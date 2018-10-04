-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

--[[
# Heka Message Monitor

Analyzes each unique message in a data stream based on the specified hierarchy.
This schema is used for automated monitoring of all message attributes for any
change in behavior.

## Sample Configuration

```lua
filename        = "heka_message_monitor.lua"
ticker_interval = 360
preserve_data   = true
message_matcher = "Uuid < '\003'" -- slightly greater than a 1% sample

hierarchy       = {"Logger", "Type", "EnvVersion"} -- default
max_set_size    = 255 -- default
samples         = 25
sample_interval = 3600

exclude = nil -- default table of field names to exclude from monitoring/analyis e.g. `{"Pid", "Fields[Date]"}`
-- telemetry
-- exclude = {"Hostname", "Pid", "Severity", "Payload", "EnvVersion", "Fields[content]", "Fields[Date]", "Fields[sampleId]", "Fields[DecodeErrorDetail]"}


alert = {
  disabled  = false,
  prefix    = true,
  throttle  = 90,
  modules   = {
    email = {recipients = {"trink@mozilla.com"}},
  },
  thresholds = {
    -- pcc          = 0.3,  -- default minimum correlation coefficient (less than or equal alerts)
    -- submissions  = 1000, -- default minimum number of submissions before alerting in at least the current and one previous interval
    active          = sample_interval  * 5, -- number of seconds after field creation before alerting
  }}

```

## Analysis Behavior (based on subtype)
- `unknown` - no analysis
- `unique`  - min/max lengths, hyperloglog percent duplicate calculation
- `range`   - histogram analysis of the range
- `set`     - histogram analysis of enumerated set
- `sparse`  - weights of each of the most frequent items are computed
--]]
_PRESERVATION_VERSION = read_config("preservation_version") or 1
schema = {}

require "math"
require "os"
require "string"
require "table"
local alert  = require "heka.alert"
local matrix = require "streaming_algorithms.matrix"
local escape_json = require "lpeg.escape_sequences".escape_json
local escape_html = require "lpeg.escape_sequences".escape_html

local hierarchy         = read_config("hierarchy") or {"Logger", "Type", "EnvVersion"}
local levels            = #hierarchy - 1
local leaf              = hierarchy[levels + 1];

local max_set_size      = read_config("max_set_size") or 255
local samples           = read_config("samples") or 25
local sample_interval   = read_config("sample_interval") or 3600
sample_interval         = sample_interval * 1e9

local alert_pcc         = alert.get_threshold("pcc") or 0.3
local alert_submissions = alert.get_threshold("submissions") or 1000
local alert_active      = alert.get_threshold("active") or 3600

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


local function init_header()
    return {
        created     = 0,
        updated     = 0,
        alerted     = false,
        cnt         = 0,
        values_cnt  = 0,
        values      = {},
        subtype     = "unknown",
        min         = math.huge,
        max         = -math.huge
    }
end


local exclude_table = {exclude = true}
local function get_entry_table(t, key)
    local v = t[key]
    if not v then
        v = {}
        v.cnt = 0
        v.headers = {
            -- Uuid and Timestamp are not tracked
            Logger      = init_header(),
            Hostname    = init_header(),
            Type        = init_header(),
            Payload     = init_header(),
            EnvVersion  = init_header(),
            Pid         = init_header(),
            Severity    = init_header(),
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

local bar_div = [[
<div id="%s""</div>
<script type="text/javascript">
MG.data_graphic({
    title: '%s',
    data: [%s],
    chart_type: 'bar',
    width: %d,
    height: 320,
    bottom: 60,
    target: '#%s',
    x_accessor: 'x',
    y_accessor: 'y',
    rotate_x_labels: 45
});</script>
]]
local function debug_alert(v, pcc, row, closest, title, alerts)
    local cnt = #alerts
    local curr = v.data:get_row(row)
    local cdata = {}
    local labels = {}
    for k,e in pairs(v.values) do
        labels[e.idx] = k
    end
    for x,y in ipairs(curr) do
        cdata[x] = string.format('{x:"%s",y:%d}', labels[x], y)
    end

    local width = math.floor(1000 / v.values_cnt)
    if width < 25 then width = 25 end
    width = width * v.values_cnt

    local div = string.format("current%d", cnt)
    local c = string.format(bar_div, div, string.format("current = %d", row), table.concat(cdata, ","), width, div);

    local prev = v.data:get_row(closest)
    local pdata = {}
    for x,y in ipairs(prev) do
        pdata[x] = string.format('{x:"%s",y:%d}', labels[x], y)
    end

    div = string.format("closest%d", cnt)
    local p = string.format(bar_div, div, string.format("closest index = %d", closest), table.concat(pdata, ","), width, div);
    alerts[cnt + 1] = string.format("<h1>%s</h1>\n<span>Pearson's Correlation Coefficient:%g</span>\n%s%s", title, pcc, c, p)
end


local function output_subtype(key, v, stats)
    local sep = ""
    add_to_payload(string.format(',"subtype":"%s"', v.subtype))
    if v.subtype == "set" then
        add_to_payload(string.format(',"created":%d,"updated":%d,"alerted":%s,"values_cnt":%d,"cint":%d,"values":{', v.created, v.updated, tostring(v.alerted), v.values_cnt, v.cint))
        for k, t in pairs(v.values) do
            local ks = escape_json(k)
            add_to_payload(string.format('%s"%s":{"idx":%d,"cnt":%d}', sep, ks, t.idx, t.cnt))
            sep = ","
        end
        add_to_payload("}")

        local pcc, closest
        if v.values_cnt > 2 then pcc, closest = v.data:pcc(v.cint) end
        if pcc then
            add_to_payload(string.format(',"pcc":%g,"closest_row":%d', pcc, closest))
            local submissions = v.data:sum(v.cint)
            if submissions >= alert_submissions
            and pcc <= alert_pcc
            and not v.alerted
            and v.updated - v.created > alert_active
            and #stats.alerts < 25 then
                local alert = false
                for i=1, samples do
                    -- confirm there is at least one other row with the minimum number of submissions
                    if i ~= v.cint and v.data:sum(i) >= alert_submissions then
                        alert = true
                        break
                    end
                end
                if alert then
                    v.alerted = true
                    debug_alert(v, pcc, v.cint, closest, escape_html(string.format("%s->%s", table.concat(stats.path, "->"), tostring(key))), stats.alerts)
                end
            end
        end
    elseif v.subtype == "sparse" then
        add_to_payload(string.format(',"values_cnt":%d,"values":{', v.values_cnt))
        for k, w in pairs(v.values) do
            local ks = escape_json(k)
            add_to_payload(string.format('%s"%s":{"weight":%d}', sep, ks, w))
            sep = ","
        end
        add_to_payload("}")
    elseif v.subtype == "unique" or v.subtype == "range" then
        if v.min ~= 1/0 then
            add_to_payload(string.format(',"min":%g,"max":%g', v.min, v.max))
        end
    end
end


local function output_headers(t, stats)
    local sep = ""
    add_to_payload(string.format('"headers":{'))
    for k, v in pairs(t) do
        local ks = escape_json(k)
        add_to_payload(string.format('%s"%s":{"cnt":%d', sep, ks, v.cnt))
        output_subtype(k, v, stats)
        add_to_payload("}")
        sep = ","
    end
    add_to_payload("},")
end


local function output_fields(t, stats)
    local sep = ""
    add_to_payload(string.format('"fields":{'))
    for k,v in pairs(t) do
        if not v.exclude then
            local ks = escape_json(k)
            add_to_payload(string.format(
                '%s"%s":{"cnt":%d,"type":"%s","representation":"%s","array":%s,"optional":%s',
                sep, ks, v.cnt, get_type(v.type),
                escape_json(v.representation),
                tostring(v.repetition == true),
                tostring(t.cnt ~= v.cnt)))
            output_subtype(k, v, stats)
            add_to_payload("}")
            sep = ","
        end
    end
    add_to_payload("}")
end


local function output_table(t, clevel, stats)
    local sep = ""
    add_to_payload(string.format('"%s":{\n', escape_json(hierarchy[clevel + 1])))
    for k,v in pairs(t) do
        if type(v) == "table" then
            local ks = escape_json(k)
            add_to_payload(string.format('%s"%s":{"cnt":%d,', sep, ks, v.cnt))
            stats.path[clevel + 1] = ks
            if clevel < levels then
                output_table(v, clevel + 1, stats)
            else
                output_headers(v.headers, stats)
                output_fields(v.fields, stats)
            end
            add_to_payload("}\n\n")
            sep = ","
        end
    end
    add_to_payload('}\n')
end


local function process_entry(ns, f, value, entry)
    if entry.exclude then return end

    if ns > entry.updated then entry.updated = ns end
    entry.cnt = entry.cnt + 1
    if f.value_type ~= entry.type then
        entry.type = -1 -- mis-matched types
    end

    if entry.subtype == "unknown" then
        local v = entry.values[value]
        if v then
            v.cnt = v.cnt + 1
        else
            entry.values_cnt = entry.values_cnt + 1
            v = {idx = entry.values_cnt, cnt = 1}
            entry.values[value] = v
        end

        if type(value) == "string" then
            local len = #value
            if len < entry.min then entry.min = len end
            if len > entry.max then entry.max = len end
        elseif type(value) == "number" then
            if value < entry.min then entry.min = value end
            if value > entry.max then entry.max = value end
        end

        if entry.cnt == max_set_size then
            local ratio = entry.cnt / entry.values_cnt
            if entry.type == 2 or entry.type == 3 then -- numeric
                if ratio < 2 then
                    entry.subtype = "range"
                    entry.values = nil
                    entry.values_cnt = nil
                else
                    entry.subtype = "set"
                end
            else
                if ratio == 1 then
                    entry.subtype = "unique"
                    entry.values = nil
                    entry.values_cnt = nil
                else
                    entry.subtype = "set"
                end
            end
            if entry.subtype == "set" then
                entry.data = matrix.new(samples, entry.values_cnt)
                entry.cint = math.floor(ns / sample_interval) % samples + 1
            end
        end
    elseif entry.subtype == "set" then
        local v = entry.values[value]
        local int = math.floor(ns / sample_interval) % samples + 1
        if ns == entry.updated and entry.cint ~= int then
            entry.cint = int
            entry.data:clear_row(int) -- don't worry about any intervals we may have skipped
        end
        if v then
            entry.data:add(int, v.idx, 1)
            v.cnt = v.cnt + 1
        else
            if entry.values_cnt < max_set_size then
                entry.values_cnt = entry.values_cnt + 1
                local m = matrix.new(samples, entry.values_cnt)
                m:merge(entry.data)
                m:add(int, entry.values_cnt, 1)
                entry.data = m
                entry.values[value] = {idx = entry.values_cnt, cnt = 1}
            else
                entry.subtype = "sparse"
                for k,v in pairs(entry.values) do
                    entry.values[k] = v.cnt;
                end
                entry.data = nil
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
        t = get_table(t, k)
    end
    local k = read_message(leaf) or "NIL"
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
                created = ns,
                updated = 0,
                type = f.value_type,
                representation = f.representation,
                repetition = #f.value > 1,
                values_cnt = 0,
                values = {},
                min = math.huge,
                max = -math.huge,
                subtype = "unknown",
                alerted = false
            }
            if f.representation == "json" then
                entry.subtype = "unique"
            end
            v.fields[f.name] = entry
        end
        process_entry(ns, f, value, entry)
    end
    return 0
end

local schema_name   = "message_schema"
local schema_ext    = "json"
local alert_name    = "alerts"
local alert_ext     = "html"
local html_fmt = [[
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<link href='css/metricsgraphics.css' rel='stylesheet' type='text/css' id='light'>
<script src="https://d3js.org/d3.v4.min.js"></script>
<script src='graphs/js/metricsgraphics.min.js'></script>
<body>
%s
</body>
</html>
]]
function timer_event(ns, shutdown)
    if shutdown then return end

    local stats = {
        path        = {},
        alerts      = {},
    }

    add_to_payload("{\n")
    output_table(schema, 0, stats)
    add_to_payload(string.format(',"hierarchy_levels":%d\n}', levels + 1))
    inject_payload(schema_ext, schema_name)

    if #stats.alerts > 0 then
        inject_payload(alert_ext, alert_name, string.format(html_fmt, table.concat(stats.alerts, "\n")))
        alert.send(alert_name, "pcc", string.format("graphs: %s\n", alert.get_dashboard_uri(alert_name, alert_ext)))
    end
end
