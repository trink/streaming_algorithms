-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

--[[
# Heka Message Volume Monitor

Analyzes the count of each unique message in a data stream based on the
specified hierarchy. This schema is used for automated monitoring of all 'set'
message attributes for any change in behavior.

## Sample Configuration

```lua
filename = "heka_message_monitor.lua"
ticker_interval = 360
preserve_data = true
message_matcher = "Uuid < '\003'" -- slightly greater than a 1% sample

hierarchy = {"Logger", "Type", "EnvVersion"} -- default
max_set_size = 255 -- default
time_series_length = 240 * 9 + 1 -- default (9 days plus the active aggregation window)
time_series_resolution = 360 -- seconds default (6 minutes)
time_series_sample_length = 10 -- default used to determine if a attribute has non sparse data

mp_window = 240 -- default (1 day)
mp_percent_data = 10 -- default (used for an estimate, if an alert would be trigger the exact value is verified)

exclude = nil -- default table of field names to exclude from monitoring/analyis e.g. `{"Pid", "Fields[Date]"}`

alert = {
  disabled = false,
  prefix = true,
  throttle = 90,
  modules = {
    email = {recipients = {"trink@mozilla.com"}},
  },
  thresholds = {
    mp_percent = 50, -- default percentage of matrix profile range the top 5% of discords must cover before alerting (max - median)
    mp_mindist = 10, -- default minimum distance change before alerting (max - median)
    mp_suppress = false, -- suppress alerts until the matrix profile has been active for time_series_length
  }
}

```

## Analysis Behavior (based on subtype)
- `unknown` - no analysis
- `unique`  - min/max lengths
- `range`   - min/max values
- `set`     - matrix profile analysis for frequency of each item
- `sparse`  - weights of each of the most frequent items are computed
--]]
_PRESERVATION_VERSION = read_config("preservation_version") or 0
schema = {}

require "math"
require "os"
require "string"
require "table"
local alert = require "heka.alert"
local sats  = require "streaming_algorithms.time_series"
local escape_json = require "lpeg.escape_sequences".escape_json
local escape_html = require "lpeg.escape_sequences".escape_html

local time_series_length        = read_config("time_series_length") or 240 * 9 + 1
local time_series_resolution    = read_config("time_series_resolution") or 360
time_series_resolution = time_series_resolution * 1e9
local time_series_sample_length = read_config("time_series_sample_length") or 10
time_series_length  = time_series_length + 1 -- add an extra interval for the current/incomplete data

local mp_window         = read_config("mp_window") or 240
local mp_percent_data   = read_config("mp_percent_data") or 10
local max_set_size      = read_config("max_set_size") or 255
local hierarchy         = read_config("hierarchy") or {"Logger", "Type", "EnvVersion"}
local levels            = #hierarchy - 1
local leaf              = hierarchy[levels + 1];

local alert_mp_percent  = alert.get_threshold("mp_percent") or 50
local alert_mp_mindist  = alert.get_threshold("mp_mindist") or 10
local alert_mp_suppress = alert.get_threshold("mp_suppress")

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

local function debug_graph(ts, st, title, stats)
    local raw = ts:get_range(nil, time_series_length - 1)
    local mp  = ts:matrix_profile(nil, time_series_length - 1, mp_window, 100, "mp") -- todo avoid recalculating it again just to get the array
    local dgraw = {}
    local dgmp = {}
    for i,v in ipairs(raw) do
        local date = os.date("%Y/%m/%d %H:%M:%S", (st + i * time_series_resolution)/1e9)
        dgraw[i] = string.format("%s,%d\\n", date, v)
        dgmp[i]  = string.format("%s,%g\\n", date, mp[i] or 0/0) -- fill with NaN to keep the graphs aligned
    end
    local i = stats.alerts_cnt
    stats.alerts[i] = string.format(dygraphs_entry, title, i, i, i, table.concat(dgraw), i, i, i, table.concat(dgmp));
end


local function compute_mp(ts, created, st, key, ks, stats)
    stats.mp_cnt = stats.mp_cnt + 1
    add_to_payload(string.format(',"created":%d', created))
    local ats, rp, dist = ts:matrix_profile(nil, time_series_length - 1, mp_window, mp_percent_data, "anomaly_current")
    if ats and rp == rp then
        if rp > alert_mp_percent and dist > alert_mp_mindist and (created <= st or not alert_mp_suppress) then
            -- re-check using the exact calculation
            ats, rp, dist = ts:matrix_profile(nil, time_series_length - 1, mp_window, 100, "anomaly_current")
            if rp > alert_mp_percent and dist > alert_mp_mindist then
                if stats.alerts_cnt < 25 then
                    stats.alerts_cnt = stats.alerts_cnt + 1
                    add_to_payload(string.format(',"alert":%d', ats))
                    local title = escape_html(string.format("%s->%s[%s]", table.concat(stats.path, "->"), tostring(key), ks))
                    debug_graph(ts, st, title, stats)
                end
            end
        end
        local full = (stats.ns - created) / (time_series_length * time_series_resolution)
        if full > 1 then full = 1 end
        add_to_payload(string.format(',"rp":%g,"dist":%g,"full":%g', rp, dist, full * 100))
    end
end


local function output_subtype(key, v, stats)
    local sep = ""
    add_to_payload(string.format(',"subtype":"%s"', v.subtype))
    if v.subtype == "set" then
        add_to_payload(string.format(',"values_cnt":%d,"values":{', v.values_cnt))
        for k, t in pairs(v.values) do
            stats.var_cnt = stats.var_cnt + 1
            local ks = escape_json(k)
            add_to_payload(string.format('%s"%s":{"cnt":%d', sep, ks, t[1]))
            if t[2] then
                local ct = t[2]:current_time()
                local st = ct - (time_series_length * time_series_resolution - 1)
                if stats.ns - ct < mp_window * time_series_resolution  then
                    compute_mp(t[2], t[3], st, key, ks, stats)
                else
                    local t = v.removed
                    if not t then
                        t = {}
                        v.removed = t
                    end
                    t[k] = stats.ns
                    v.values[k] = nil -- remove inactive matrix profile
                    v.values_cnt = v.values_cnt - 1
               end
            end
            add_to_payload("}")
            sep = ","
        end
        add_to_payload("}")
        if v.removed then
            sep = ""
            add_to_payload(string.format(',"removed":{'))
            for k, t in pairs(v.removed) do
                add_to_payload(string.format('%s"%s":%d', sep, escape_json(k), t))
                sep = ","
                if stats.ns - t >=  time_series_length * time_series_resolution then
                    v.removed[k] = nil
                end
            end
            add_to_payload("}")
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
                    ts = sats.new(time_series_length, time_series_resolution)
                    ts:merge(sts, "set")
                    v[2] = ts
                    v[3] = ns -- creation time of the matrix profile
                end
            end
        else
            if entry.values_cnt == max_set_size then
                entry.subtype = "sparse"
                entry.removed = nil
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


local viewer
local viewer1
local alert_name = "alerts"
local alert_ext  = "html"
local schema_name = "message_schema"
local schema_ext = "json"
local dygraphs_template = [[
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<script type="text/javascript" src="graphs/js/dygraph-combined.js">
</script>
</head>
<body>
%s
</body>
</html>
]]
function timer_event(ns, shutdown)
    if shutdown then return end

    local stats = {
        ns          = ns,
        path        = {},
        alerts      = {},
        alerts_cnt  = 0,
        mp_cnt      = 0,
        var_cnt     = 0
    }

    add_to_payload("{\n")
    output_table(schema, 0, stats)
    add_to_payload(string.format(',"variable_cnt":%d,\n"mp_cnt":%d\n,\n"hierarchy_levels":%d\n}',
                                 stats.var_cnt, stats.mp_cnt, levels + 1))
    inject_payload(schema_ext, schema_name)

    if viewer then
        inject_payload("html", "viewer", viewer, alert.get_dashboard_uri(schema_name, schema_ext), viewer1)
        viewer = nil
        viewer1 = nil
    end

    if stats.alerts_cnt > 0 then
        inject_payload(alert_ext, alert_name, string.format(dygraphs_template, table.concat(stats.alerts, "\n")))
        -- only one throttled alert for all hierarchys (the graph output contains all the issues)
        alert.send(alert_name, "matrix profile",
        string.format("graphs: %s\n", alert.get_dashboard_uri(alert_name, alert_ext)))
    end
end

viewer = [[
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<link rel="stylesheet" type="text/css" href="https://cdn.datatables.net/1.10.16/css/jquery.dataTables.css">
<script type="text/javascript" charset="utf8" src="https://code.jquery.com/jquery-3.3.1.js">
</script>
<script type="text/javascript" charset="utf8" src="https://cdn.datatables.net/1.10.16/js/jquery.dataTables.min.js">
</script>
<script type="text/javascript">
function fetch(url, callback) {
  var req = new XMLHttpRequest();
  var caller = this;
  req.onreadystatechange = function() {
    if (req.readyState == 4) {
      if (req.status == 200 ||
        req.status == 0) {
        callback(JSON.parse(req.responseText));
      } else {
        var p = document.createElement('p');
        p.innerHTML = "data retrieval failed: " + url;
        document.body.appendChild(p);
      }
    }
  };
  req.open("GET", url, true);
  req.send(null);
}

function get_datestring(ns) {
  var d = new Date(ns / 1000000);
  var ds = d.getFullYear() + "/"
    + ("0" + (d.getMonth() + 1)).slice(-2) + "/"
    + ("0" + d.getDate()).slice(-2);
  return ds;
}

function traverse(path, object, ta, cl) {
  for (key in object) {
    value = object[key];
    if (typeof value === 'object') {
      if (value.subtype === 'set') {
        path.push(key);
        var hierarchy = path.join("->");
        for (k in value.values) {
          var v = value.values[k];
          if (v.created) {
            console.log(v.created)
            cl.push([get_datestring(v.created), "add", hierarchy, k]);
          }
          if (v.rp) {
            ta.push([hierarchy, k, v.cnt, v.rp, v.dist, v.full]);
          }
        }
        for (k in value.removed) {
          var v = value.removed[k];
          cl.push([get_datestring(v), "delete", hierarchy, k])
        }
        path.pop();
      } else {
        var item = typeof value.cnt === 'number';
        if (item) path.push(key);
        traverse(path, value, ta, cl)
        if (item) path.pop();
      }
    }
  }
}

function load(schema) {
  ta = [ ];
  cl = [ ];
  path = [ ];
  traverse(path, schema, ta, cl);

  $('#anomalies').DataTable( {
    data: ta,
    order: [ [4, "desc"] ],
    columns: [
    { title: "Hierarchy" },
    { title: "Value" },
    { title: "Count" },
    { title: "Relative Percentage" },
    { title: "Distance" },
    { title: "%Full" }
      ]
  });

  $('#changelog').DataTable( {
    data: cl,
    order: [ [0, "desc"] ],
    columns: [
    { title: "Date" },
    { title: "Action" },
    { title: "Hierarchy" },
    { title: "Value" }
      ]
  });

  var url = new URL(window.location);
  select_view(url.hash)
}

function select_view(hash) {
  var ta = document.getElementById("ta");
  var cl = document.getElementById("cl");
  if (hash == '#changelog') {
    ta.style = "display:none";
    cl.style = "display:block";
  } else {
    ta.style = "display:block";
    cl.style = "display:none";
  }
}
</script>
</head>
<body onload="fetch(']]

viewer1 =
[[', load);">
<p>
<a href="#anomalies" onclick="select_view(); return true;">Top Anomalies</a>
|
<a href="#changelog" onclick="select_view('#changelog'); return true;">Change Log</a>
</p>
<div id="ta" style="display:none">
    <h1>Top Anomalies</h1>
    <table id="anomalies" class="display" width="100%">
    </table>
</div>
<div id="cl" style="display:none">
    <h1>Change Log</h1>
    <table id="changelog" class="display" width="100%">
    </table>
</div>
</body>
</html>
]]
