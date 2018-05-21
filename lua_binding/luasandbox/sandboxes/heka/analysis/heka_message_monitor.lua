-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

--[[
# Heka Message Monitor

Generates documentation for each unique message in a data stream based on the
specified hierarchy. This schema is used for automated monitoring of all message
attributes for any change in behavior.

## Sample Configuration

```lua
filename = "heka_message_monitor.lua"
ticker_interval = 360
preserve_data = true
message_matcher = "Uuid < '\003'" -- slightly greater than a 1% sample

hierarchy = {"Logger", "Type", "EnvVersion"} -- default
max_set_size = 255 -- default
time_series_length = 240 * 8 + 10 -- default (8 days + 1 hour) -- weekly pattern allowing for some skew
time_series_resolution = 360 -- seconds default (6 minutes)
time_series_sample_length = 10 -- default used to determine if a attribute has non sparse data

mp_window = 240 -- default (1 day)
mp_percent_data = 10 -- default

exclude = nil -- default table of field names to exclude from monitoring/analyis e.g. `{"EnvVersion", "Fields[Date]"}`

alert = {
  disabled = false,
  prefix = true,
  throttle = 90,
  modules = {
    email = {recipients = {"trink@mozilla.com"}},
  },
  thresholds = {
    mp_percent = 50, -- default percentage of matrix profile range the top 5% of discords must cover before alerting (max - median)
    mp_mindist = 5, -- default minimum distance change before alerting
  }
}

```

## Sample Output
```
# Fields[normalizedChannel]
# The number in brackets is the number of messages occuring at each level of the hierarchy.
Other [738]
  Logger // subtype: set [1]
    telemetry [738]

  Type // subtype: set [2]
    telemetry [727]
    telemetry.error [11]

  Pid // subtype: set [2]
    2615 [138]
    2623 [135]

  Hostname // subtype: set [2]
    ip-172-31-10-108 [261]
    ip-172-31-30-139 [249]

  Severity - optional [0] // subtype: unknown
  Payload - optional [0] // subtype: unique min:inf max:-inf
  EnvVersion - optional [0] // subtype: unknown
  -Fields-
    payload.childPayloads (binary (json) - optional [80]) // subtype: unique min:137 max:1835
    sourceVersion (mismatch - optional [727]) // subtype: set [4]
      8 [398]
      4 [181]
      9 [135]
      7 [13]
```

## Analysis Behavior (based on subtype)
- `unknown` - no analysis
- `unique`  - no matrix profile analysis (just min/max lengths)
- `range`   - no matrix profile analysis (just min/max values)
- `set`     - counts of each item are analyzed
- `sparse`  - weights of each of the most frequent items are computed
--]]
_PRESERVATION_VERSION = read_config("preservation_version") or 0

require "math"
require "os"
require "string"
require "table"
local alert = require "heka.alert"
local sats  = require "streaming_algorithms.time_series"

schema = {}
report = {}
report_size = 0
day = math.floor(os.time() / 86400)

local time_series_length        = read_config("time_series_length") or 240 * 8 + 10
local time_series_resolution    = read_config("time_series_resolution") or 360
time_series_resolution = time_series_resolution * 1e9
local time_series_sample_length = read_config("time_series_sample_length") or 10
time_series_length  = time_series_length + 1 -- add an extra interval for the current/incomplete data

local mp_window         = read_config("mp_window") or 240
local mp_percent_data   = read_config("mp_percent_data") or 10
local max_set_size      = read_config("max_set_size") or 255
local hierarchy         = read_config("hierarchy") or {"Logger", "Type", "EnvVersion"}

local alert_mp_percent  = alert.get_threshold("mp_percent") or 50
local alert_mp_mindist  = alert.get_threshold("mp_mindist") or 5

local levels    = #hierarchy - 1
local leaf      = hierarchy[levels + 1];
local path      = {}

local exclude = read_config("exclude") or {}
local exclude_headers
local exclude_fields
for i,v in ipairs(exclude) do
    local fn = v:match("^Fields%[([^]]+)%]$")
    if fn then
        if not exclude_fields then exclude_fields = {} end
        exclude_fields[#exclude_fields + 1] = fn
    else
        if not exclude_headers then exclude_headers = {} end
        exclude_headers[#exclude_headers + 1] = v
    end
end
exclude = nil

local dygraphs_entry = [[
<h1>%s</h1>
<div id="raw%d" style="height: 200px; width: 100%%;"</div>
<script type="text/javascript">
  gr%d = new Dygraph(document.getElementById("raw%d"), "Date,raw\n" + "%s");
</script>
<div id="mp%d" style="height: 100px; width: 100%%;"</div>
<script type="text/javascript">
  gm%d = new Dygraph(document.getElementById("mp%d"), "Date,mp\n" + "%s");
</script>
]]

local alerts = {}
local function debug_graph(ts, key)
    local idx = #alerts + 1
    if idx > 50 then return end

    local rows, ns_per_row = ts:get_configuration()
    local st = ts:current_time() - (rows * ns_per_row - 1)
    local raw = ts:get_range(nil,  time_series_length - 1)
    local mp = ts:matrix_profile(nil, time_series_length - 1, mp_window, mp_percent_data, "mp")
    local dgraw = {}
    local dgmp = {}
    for i,v in ipairs(raw) do
        dgraw[i] = string.format("%s,%d\\n", os.date("%Y/%m/%d %H:%M:%S", (st + i * ns_per_row)/1e9), v)
        dgmp[i] = string.format("%s,%g\\n", os.date("%Y/%m/%d %H:%M:%S", (st + i * ns_per_row)/1e9), mp[i] or 0) -- fill with zero to keep the graphs aligned
    end
    alerts[idx] = string.format(dygraphs_entry, key, idx, idx, idx, table.concat(dgraw), idx, idx, idx, table.concat(dgmp));
end


local function get_type(t)
    if t == -1 then
        return "mismatch"
    elseif t == 1 then
        return "binary"
    elseif t == 2 then
        return "integer"
    elseif t == 3 then
        return "double"
    elseif t == 4 then
       return "bool"
    end
    return "string" -- default
end


local exclude_table = {exclude = true}
local function get_entry_table(t, key)
    local v = t[key]
    if not v then
        v = {}
        v.cnt = 0
        v.headers = {
--            Uuid        = {cnt = 0, values_cnt = 0, values = {}, subtype = "unique",
--                min = math.huge, max = -math.huge},
--            Timestamp   = {cnt = 0, values_cnt = 0, values = {}, subtype = "range",
--                min = math.huge, max = -math.huge},
            Logger      = {cnt = 0, values_cnt = 0, values = {}, subtype = "unknown",
                min = math.huge, max = -math.huge},
            Hostname    = {cnt = 0, values_cnt = 0, values = {}, subtype = "unknown",
                min = math.huge, max = -math.huge},
            Type        = {cnt = 0, values_cnt = 0, values = {}, subtype = "unknown",
                min = math.huge, max = -math.huge},
            Payload     = {cnt = 0, values_cnt = 0, values = {}, subtype = "unique",
                min = math.huge, max = -math.huge},
            EnvVersion  = {cnt = 0, values_cnt = 0, values = {}, subtype = "unknown",
                min = math.huge, max = -math.huge},
            Pid         = {cnt = 0, values_cnt = 0, values = {}, subtype = "unknown",
                min = math.huge, max = -math.huge},
            Severity    = {cnt = 0, values_cnt = 0, values = {}, subtype = "unknown",
                min = math.huge, max = -math.huge},
        }
        if exclude_headers then
            for i,h in ipairs(exclude_headers) do
                v.headers[h] = nil
            end
        end
        v.fields = {}
        if exclude_fields then
            for i,f in ipairs(exclude_fields) do
                v.fields[f] = exclude_table
            end
        end
        t[key] = v
    end
    v.cnt = v.cnt + 1
    return v
end


local function get_table(t, key)
    local v = t[key]
    if not v then
        v = {}
        v.cnt = 0
        t[key] = v
    end
    v.cnt = v.cnt + 1
    return v
end


function order_by_value(t, f)
    local a = {}
    for k, v in pairs(t) do table.insert(a, {k, v}) end
    table.sort(a, f)
    local i = 0
    local iter = function ()
        i = i + 1
        if a[i] == nil then
            return nil
        else
            return a[i][1], a[i][2]
        end
    end
    return iter
end

local variable_cnt = 0
local mp_cnt = 0
local function output_subtype(key, v, indent)
    add_to_payload(" // subtype: ", v.subtype)
    if v.subtype == "set" then
        add_to_payload(" [", v.values_cnt, "]\n")
        for k, t in order_by_value(v.values, function(a,b) return a[2][1] > b[2][1] end) do
            variable_cnt = variable_cnt + 1
            add_to_payload(indent, k, " [", t[1],"]")
            if t[2] then
                mp_cnt = mp_cnt + 1
                local ats, rp, dist = t[2]:matrix_profile(nil, time_series_length - 1,
                                                        mp_window, mp_percent_data, "anomaly_current")
                add_to_payload(" rp:", rp, " dist:", dist)
                if ats then
                    if rp > alert_mp_percent and dist > alert_mp_mindist then
                        add_to_payload(" ### ALERT:", os.date("%Y-%m-%d %H:%M:%S", ats/1e9))
                        debug_graph(t[2], string.format("%s->%s[%s]", table.concat(path, "->"),
                                                        tostring(key), tostring(k)))
                    end
                else
                    local rows, npr = t[2]:get_configuration()
                    if os.time() * 1e9 - t[2]:current_time() >= rows * npr then
                        report[report_size] = string.format(
                            "%s %s removed empty matrix profile field: %s item: %s",
                            os.date("%Y/%m/%d %H:%M:%S"), table.concat(path, "->"),
                            tostring(key), tostring(k))
                        report_size = report_size + 1
                        v.values[k] = nil
                        v.values_cnt = v.values_cnt - 1
                    end
                end
            end
            add_to_payload("\n")
        end
    elseif v.subtype == "sparse" then
        add_to_payload(" [", v.values_cnt, "]\n")
        for k, w in order_by_value(v.values, function(a,b) return a[2] > b[2] end) do
            add_to_payload(indent, k, " {", w,"}\n")
        end
    elseif v.subtype == "unique" or v.subtype == "range" then
        add_to_payload(" min:", v.min, " max:", v.max)
    end
    add_to_payload("\n")
end


local function output_headers(t, cnt, indent)
    for k, v in pairs(t) do
        add_to_payload(indent, k)
        if cnt ~= v.cnt then
            add_to_payload(" - optional [", v.cnt, "]")
        end
        output_subtype(k, v, indent .. "  ")
    end
end


local function output_fields(t, indent)
    output_headers(t.headers, t.cnt, indent)
    add_to_payload(indent, "-Fields-\n")
    indent = indent .. "  "
    for k,v in pairs(t.fields) do
        if not v.exclude then
            add_to_payload(indent, k, " (", get_type(v.type))
            if v.repetition then
                add_to_payload("[]")
            end
            if v.representation then
                add_to_payload(" (", v.representation, ")")
            end
            if t.cnt ~= v.cnt then
                add_to_payload(" - optional [", v.cnt, "])")
            else
                add_to_payload(")")
            end
            output_subtype(k, v, indent .. "  ")
        end
    end
end


local function output_table(t, clevel, indent)
    for k,v in pairs(t) do
        if type(v) == "table" then
            add_to_payload(indent, k, " [", v.cnt, "]\n")
            if clevel < levels then
                path[clevel + 1] = tostring(k)
                output_table(v, clevel + 1, indent .. "  ")
            else
                output_fields(v, indent .. "  ")
            end
            add_to_payload("\n")
        end
    end
end


local function process_entry(ns, f, value, entry)
    if entry.exclude then return end

    entry.cnt = entry.cnt + 1
    if f.value_type ~= entry.type then
        entry.type = -1 -- mis-matched types
    end

    if entry.subtype == "unknown" then
        local v = entry.values[value]
        if v then
            v[1] = v[1] + 1
            v[3]:add(ns, 1)
        else
            local sts = sats.new(time_series_sample_length, time_series_resolution)
            sts:add(ns, 1)
            entry.values[value] = {1, nil, sts}
            entry.values_cnt = entry.values_cnt + 1
        end

        if type(value) == "string" then
            local len = #value
            if len < entry.min then entry.min = len end
            if len > entry.max then entry.max = len end
        elseif type(value) == "number" then
            if value < entry.min then entry.min = value end
            if value > entry.max then entry.max = value end
        end

        if entry.values_cnt == max_set_size or entry.cnt == max_set_size then
            local ratio = entry.cnt / entry.values_cnt
            if entry.type == 2 or entry.type == 3 then -- numeric
                if ratio < 2 then
                    entry.subtype = "range"
                    entry.values = nil
                    entry.values_cnt = nil
                    -- no time series analysis
                else
                    entry.subtype = "set"
                end
            else
                if ratio == 1 then
                    entry.subtype = "unique"
                    entry.values = nil
                    entry.values_cnt = nil
                    -- no time series analysis
                else
                    entry.subtype = "set"
                end
            end
        end
    elseif entry.subtype == "set" then
        local v = entry.values[value]
        if v then
            local ts, sts = v[2], v[3]
            v[1] = v[1] + 1
            if ts then
                ts:add(ns, 1)
            elseif sts then
                sts:add(ns, 1)
                local sum, rows = sts:stats(nil, time_series_sample_length, "sum")
                if rows == time_series_sample_length then  -- non sparse data
                    report[report_size] = string.format(
                        "%s %s added matrix a profile to field: %s item: %s",
                        os.date("%Y/%m/%d %H:%M:%S"), table.concat(path, "->"),
                        tostring(f.name), tostring(value))
                    report_size = report_size + 1
                    ts = sats.new(time_series_length, time_series_resolution)
                    ts:merge(sts, "set")
                    v[2] = ts
                    v[3] = nil
                end
            end
        else
            if entry.values_cnt == max_set_size then
                entry.subtype = "sparse"
                for k,v in pairs(entry.values) do
                    entry.values[k] = v[1];
                end
            else
                local sts = sats.new(time_series_sample_length, time_series_resolution)
                sts:add(ns, 1)
                entry.values[value] = {1, nil, sts}
                entry.values_cnt = entry.values_cnt + 1
            end
        end
    elseif entry.subtype == "sparse" then
        local w = entry.values[value]
        if w then
            entry.values[value] = w + 1
        else
            if entry.values_cnt == max_set_size then
                for k,w in pairs(entry.values) do
                    local weight = w
                    if weight == 1 then
                        entry.values[k] = nil
                        entry.values_cnt = entry.values_cnt - 1
                    else
                        entry.values[k] = weight - 1
                    end
                end
                if entry.values_cnt == 0 then
                    if entry.type == 2 or entry.type == 3 then
                        entry.subtype = "range"
                    else
                        entry.subtype = "unique"
                    end
                    report[report_size] = string.format(
                        "%s %s demoted sparse field: %s to %s",
                        os.date("%Y/%m/%d %H:%M:%S"), table.concat(path, "->"),
                        tostring(f.name), entry.subtype)
                    report_size = report_size + 1
                    entry.values = nil
                    entry.values_cnt = nil
                end
            else
                entry.values[value] = 1
                entry.values_cnt = entry.values_cnt + 1
            end
        end
-- todo ideally we would look at something like the average of the size/value for unique/range
    elseif entry.subtype == "unique" then
        if type(value) == "string" then
            local len = #value
            if len < entry.min then entry.min = len end
            if len > entry.max then entry.max = len end
        end
    elseif entry.subtype == "range" then
        if type(value) == "number" then
        if value < entry.min then entry.min = value end
        if value > entry.max then entry.max = value end
        end
    end
end


local header = {name = nil}
function process_message()
    local msg = decode_message(read_message("raw"))
    local t = schema
    for i = 1, levels do
        local k = read_message(hierarchy[i]) or "NIL"
        path[i] = tostring(k)
        t = get_table(t, k)
    end
    local k = read_message(leaf) or "NIL"
    path[levels + 1] = tostring(k)
    local v = get_entry_table(t, k)

    local ns = read_message("Timestamp")
    for k, entry in pairs(v.headers) do
        local value = msg[k]
        if value then
            header.name = k
            process_entry(ns, header, value, entry)
        end
    end

    if not msg.Fields then return 0 end

    for i, f in ipairs(msg.Fields) do
        local value = f.value[1]
        local entry = v.fields[f.name]
        if not entry then
            entry = {
                cnt = 0,
                type = f.value_type,
                representation = f.representation,
                repetition = #f.value > 1,
                values_cnt = 0,
                values = {},
                min = math.huge,
                max = -math.huge,
                subtype = "unknown"
            }
            if f.representation == "json" then
                entry.subtype = "unique"
            elseif f.value_type == 4 then -- boolean
                entry.subtype = "set"
            end
            v.fields[f.name] = entry
        end
        process_entry(ns, f, value, entry)
    end
    return 0
end


local alert_name = "alerts"
local alert_ext  = "html"
local dygraphs_template = [[
<!DOCTYPE html>
<html>
<head>
<script type="text/javascript"
  src="graphs/js/dygraph-combined.js">
</script>
</head>
<body>
%s
</body>
</html>
]]
function timer_event(ns, shutdown)
    if shutdown then return end

    inject_payload("txt", "report", table.concat(report, "\n"))
    local cday = math.floor(os.time() / 86400)
    if day ~= cday then
        report = {}
        report_size = 0
        day = cday
    end

    variable_cnt = 0
    mp_cnt = 0
    add_to_payload("# ", table.concat(hierarchy, " -> "),
    "\n# The number in brackets is the number of messages occuring at each level of the hierarchy.\n")
    output_table(schema, 0, "")
    add_to_payload("\nvariable_cnt:", variable_cnt, " mp_cnt:", mp_cnt, "\n")
    inject_payload("txt", "message_schema")
    if alerts[1] then
        inject_payload(alert_ext, alert_name, string.format(dygraphs_template, table.concat(alerts, "\n")))
        -- only one throttled alert for all hierarchys (the graph output contains all the issues)
        alert.send(alert_name, "matrix profile",
        string.format("graphs: %s\n", alert.get_dashboard_uri(alert_name, alert_ext)))
        alerts = {}
    end
end
