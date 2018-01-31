require "string"
require "math"

-- ##########################
local rs = require "streaming_algorithms.running_stats"

local rs_errors = {
    {function() local s = rs.new(2) end                     , "test.lua:8: bad argument #0 to 'new' (this function takes no arguments)"},
    {function() local s = rs.new(); s:add("foo") end        , "test.lua:9: bad argument #1 to 'add' (number expected, got string)"},
    {function() local s = rs.new(); s:avg("foo") end        , "test.lua:10: bad argument #-1 to 'avg' (incorrect number of arguments)"},
    {function() local s = rs.new(); s:clear("foo") end      , "test.lua:11: bad argument #-1 to 'clear' (incorrect number of arguments)"},
    {function() local s = rs.new(); s:count("foo") end      , "test.lua:12: bad argument #-1 to 'count' (incorrect number of arguments)"},
    {function() local s = rs.new(); s:fromstring(nil) end   , "test.lua:13: bad argument #1 to 'fromstring' (string expected, got nil)"},
    {function() local s = rs.new(); s:__tostring(nil) end   , "test.lua:14: bad argument #-1 to '__tostring' (incorrect number of arguments)"},
    {function() local s = rs.new(); s:sd(nil) end           , "test.lua:15: bad argument #-1 to 'sd' (incorrect number of arguments)"},
    {function() local s = rs.new(); s:variance(nil) end     , "test.lua:16: bad argument #-1 to 'variance' (incorrect number of arguments)"},
    {function() local s = rs.new(); s:fromstring("foo") end , "test.lua:17: invalid serialization"},
}

for i, v in ipairs(rs_errors) do
    local ok, err = pcall(v[1])
    if ok or err ~= v[2]then
        error(tostring(err))
    end
end


local function verify_stat(stat)
    local rv = stat:count()
    assert(rv == 10, string.format("received %d", rv))
    rv = stat:avg()
    assert(rv == 5.5, string.format("received %g", rv))
    rv = stat:sd()
    assert(math.abs(3.02765 - rv) < 0.00001 , string.format("received %g", rv))
    rv = stat:variance()
    assert(math.abs(9.16667 - rv) < 0.00001 , string.format("received %g", rv))
end

local stat = rs.new()
local stat1 = rs.new()
for i = 1, 10 do
    stat:add(i)
end
verify_stat(stat)
stat1:fromstring(tostring(stat))
stat:clear()
rv = stat:count()
assert(rv == 0, string.format("received %d", rv))
verify_stat(stat1)






-- ##########################
local p2 = require "streaming_algorithms.p2"

local p2_errors = {
    {function() local s = p2.quantile("string") end     , "test.lua:60: bad argument #1 to 'quantile' (number expected, got string)"},
    {function() local s = p2.quantile(-1) end           , "test.lua:61: bad argument #1 to 'quantile' (0 < quantile < 1)"},
    {function() local s = p2.quantile(1.1) end          , "test.lua:62: bad argument #1 to 'quantile' (0 < quantile < 1)"},
    {function() local s = p2.histogram("string") end    , "test.lua:63: bad argument #1 to 'histogram' (number expected, got string)"},
    {function() local s = p2.histogram(3) end           , "test.lua:64: bad argument #1 to 'histogram' (4 <= buckets < 65535)"},
    {function() local s = p2.histogram(65535) end       , "test.lua:65: bad argument #1 to 'histogram' (4 <= buckets < 65535)"},
}

for i, v in ipairs(p2_errors) do
    local ok, err = pcall(v[1])
    if ok or err ~= v[2]then
        error(tostring(err))
    end
end


local p2_method_errors = {
    {function(ud) local rv = ud:add(nil) end        , "test.lua:77: bad argument #1 to 'add' (number expected, got nil)"},
    {function(ud) ud:clear(nil) end                 , "test.lua:78: bad argument #-1 to 'clear' (incorrect number of arguments)"},
    {function(ud) local rv = ud:count(-1) end       , "test.lua:79: bad argument #1 to 'count' (marker out of range)"},
    {function(ud) local rv = ud:count(5) end        , "test.lua:80: bad argument #1 to 'count' (marker out of range)"},
    {function(ud) local rv = ud:estimate(-1) end    , "test.lua:81: bad argument #1 to 'estimate' (marker out of range)"},
    {function(ud) local rv = ud:estimate(5) end     , "test.lua:82: bad argument #1 to 'estimate' (marker out of range)"},
    {function(ud) local rv = ud:__tostring(nil) end , "test.lua:83: bad argument #-1 to '__tostring' (incorrect number of arguments)"},
    {function(ud) ud:fromstring(nil) end      , "test.lua:84: bad argument #1 to 'fromstring' (string expected, got nil)"},
    {function(ud) ud:fromstring("foo") end    , "test.lua:85: invalid serialization"},
}

for i, v in ipairs(p2_method_errors) do
    local objects = {p2.quantile(0.5), p2.histogram(4)}
    for i,obj in ipairs(objects) do
        local ok, err = pcall(v[1], obj)
        if ok or err ~= v[2]then
            error(string.format("obj: %d error: %s", i, tostring(err)))
        end
    end
end


local data = { 0.02, 0.15, 0.74, 3.39, 0.83, 22.37, 10.15, 15.43, 38.62, 15.92,
    34.60, 10.28, 1.47, 0.40, 0.05, 11.39, 0.27, 0.42, 0.09, 11.37}

local q = p2.quantile(0.5)
local h = p2.histogram(4)
for i,v in ipairs(data) do
    q:add(v)
    h:add(v)
end

local function verify_results(ud)
    local cnts = {1, 6, 10, 16, 20}
    local quantiles = {0.02, 0.493895, 4.44063, 17.2039, 38.62}
    for i=0, 4 do
        local cnt = ud:count(i)
        if cnts[i+1] ~= cnt then
            error(string.format("count marker %d expected %d received %d", i, cnts[i+1], cnt))
        end
        local e = ud:estimate(i)
        if math.abs(quantiles[i+1] - e) > 0.00001 then
            error(string.format("estimate marker %d expected %g received %g", i, quantiles[i+1], e))
        end
    end
end

verify_results(q)
verify_results(h)





-- ##########################
local cm_sketch = require "streaming_algorithms.cm_sketch"

local cms_errors = {
    {function() local s = cm_sketch.new() end, "test.lua:135: bad argument #0 to 'new' (incorrect number of arguments)"},
    {function() local s = cm_sketch.new(0.1, "string") end, "test.lua:136: bad argument #2 to 'new' (number expected, got string)"},
    {function() local s = cm_sketch.new(-1, 0.1) end, "test.lua:137: bad argument #1 to 'new' (0 < epsilon < 1)"},
    {function() local s = cm_sketch.new(0.1, -1) end, "test.lua:138: bad argument #2 to 'new' (0 < delta < 1)"},
    {function() local s = cm_sketch.new("string", -1) end, "test.lua:139: bad argument #1 to 'new' (number expected, got string)"},
}

for i, v in ipairs(cms_errors) do
    local ok, err = pcall(v[1])
    if ok or err ~= v[2]then
        error(tostring(err))
    end
end


local cms_method_errors = {
    {function(ud) local rv = ud:update() end, "test.lua:151: bad argument #-1 to 'update' (incorrect number of arguments)"},
    {function(ud) local rv = ud:update(true) end, "test.lua:152: bad argument #1 to 'update' (must be a string or number)"},
    {function(ud) local rv = ud:update("a", "a") end, "test.lua:153: bad argument #2 to 'update' (number expected, got string)"},
    {function(ud) local rv = ud:item_count(6) end, "test.lua:154: bad argument #-1 to 'item_count' (incorrect number of arguments)"},
    {function(ud) local rv = ud:unique_count(6) end, "test.lua:155: bad argument #-1 to 'unique_count' (incorrect number of arguments)"},
    {function(ud) local rv = ud:clear(6) end, "test.lua:156: bad argument #-1 to 'clear' (incorrect number of arguments)"},
    {function(ud) ud:fromstring(nil) end, "test.lua:157: bad argument #1 to 'fromstring' (string expected, got nil)"},
    {function(ud) ud:fromstring("foo") end, "test.lua:158: invalid serialization"},
    {function(ud) ud:point_query() end, "test.lua:159: bad argument #-1 to 'point_query' (incorrect number of arguments)"},
    {function(ud) ud:point_query(true) end, "test.lua:160: bad argument #1 to 'point_query' (must be a string or number)"},
}

for i, v in ipairs(cms_method_errors) do
    local cms = cm_sketch.new(0.1, 0.1)
    local ok, err = pcall(v[1], cms)
    if ok or err ~= v[2]then
        error(string.format("test: %d error: %s", i, tostring(err)))
    end
end

local cms = cm_sketch.new(0.1, 0.1)
assert(cms:item_count() == 0)
assert(cms:unique_count() == 0)
assert(cms:point_query("a") == 0)
assert(cms:update("a", -10) == 0)
assert(cms:item_count() == 0)
assert(cms:unique_count() == 0)
cms:update("c", 6)
cms:update("a")
cms:update("b", 2)
cms:update("c", -3)
cms:update(2)
assert(cms:item_count() == 7)
assert(cms:unique_count() == 4)
assert(cms:point_query("a") == 1)
assert(cms:point_query("b") == 2)
assert(cms:point_query("c") == 3)
assert(cms:point_query(2) == 1)
assert(cms:update("c", -4) == 0)
assert(cms:point_query("c") == 0)
assert(cms:item_count() == 4)
assert(cms:unique_count() == 3)
