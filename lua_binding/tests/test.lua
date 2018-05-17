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
    rv = stat:usd()
    assert(math.abs(2.87228 - rv) < 0.00001 , string.format("received %g", rv))
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


-- ##########################
local time_series = require "streaming_algorithms.time_series"

local errors = {
    function() local cb = time_series.new(2) end, -- new() incorrect # args
    function() local cb = time_series.new(nil, 1) end, -- new() non numeric row
    function() local cb = time_series.new(1, 1) end, -- new() 1 row
    function() local cb = time_series.new(2, nil) end, -- new() non numeric seconds_per_row
    function() local cb = time_series.new(2, 0) end, -- new() zero seconds_per_row
    function() local cb = time_series.new(2, 360e9 * 360e9) end, -- out of range seconds_per_row
    function() local cb = time_series.new(2, 1) -- set() non numeric time
    cb:set(nil, 1) end,
    function() local cb = time_series.new(2, 1) -- get() invalid object
    local invalid = 1
    cb.get(invalid, 1, 1) end,
    function() local cb = time_series.new(2, 1) -- set() non numeric value
    cb:set(0, nil) end,
    function() local cb = time_series.new(2, 1) -- set() incorrect # args
    cb:set(0) end,
    function() local cb = time_series.new(2, 1) -- add() incorrect # args
    cb:add(0) end,
    function() local cb = time_series.new(2, 1) -- get() incorrect # args
    cb:get() end,
    function() local cb = time_series.new(2, 1) -- matrix_profile() incorrect # args
    cb:matrix_profile() end,
    function() local cb = time_series.new(17, 1) -- matrix_profile() invalid sequence len
    cb:matrix_profile(nil, 20, 4, 100) end,
    function() local cb = time_series.new(17, 1) -- matrix_profile() invalid sub sequence len
    cb:matrix_profile(nil, 16, 5, 100) end,
    function() local cb = time_series.new(17, 1) -- matrix_profile() invalid percent
    cb:matrix_profile(nil, 16, 4, 0) end,
    function() local cb = time_series.new(17, 1) -- matrix_profile() invalid percent
    cb:matrix_profile(nil, 16, 4, 100.1) end,
    function() local cb = time_series.new(17, 1) -- matrix_profile() invalid result
    cb:matrix_profile(nil, 16, 4, 100, "foo") end,
    function() local cb = time_series.new(2, 1) -- get_range() incorrect # args
    cb:get_range() end,
    function() local cb = time_series.new(2, 1) -- get_range() invalid ns
    cb:get_range(-1, 2) end,
    function() local cb = time_series.new(2, 1) -- get_range() invalid length
    cb:get_range(nil, 3) end,
    function() local cb = time_series.new(2, 1) -- merge() invalid ts
    cb:stats(true) end,
    function() local cb = time_series.new(2, 1) -- merge() invalid op
    cb:stats(cp, "foo") end,
    function() local cb = time_series.new(2, 1) -- stats() invalid ns
    cb:stats(-1, 2) end,
    function() local cb = time_series.new(2, 1) -- stats() invalid length
    cb:stats(nil, 3) end,
    function() local cb = time_series.new(2, 1) -- stats() invalid length
    cb:stats(nil, 2, "foo") end,                -- stats() invalid type
}

for i, v in ipairs(errors) do
    local ok = pcall(v)
    if ok then error(string.format("error test %d failed\n", i)) end
end

local data = { 132, 161, 144, 145, 31, 44, 47, 26, 232, 236, 254, 262, 339, 360,
    313, 340, 1 }

local tests = {
    function()
        local stats = time_series.new(2, 1)
        local v = stats:get(0)
        if v ~= 0 then
            error(string.format("initial value is not zero %d", v))
        end

        local v = stats:set(0, 1)
        if v ~= 1 then
            error(string.format("set failed = %d", v))
        end
        local rows, nspr = stats:get_configuration()
        assert(rows == 2)
        assert(nspr == 1)
        end,
    function()
        local stats = time_series.new(2, 1)
        local cbuf_time = stats:current_time()
        if cbuf_time ~= 1 then
            error(string.format("current_time = %d", cbuf_time))
        end
        local v = stats:set(0, 1)
        if stats:get(0) ~= 1 then
            error(string.format("set failed = %d", v))
        end
        end,
    function()
        local cb = time_series.new(10, 1)
        assert(not cb:get(10), "value found beyond the end of the buffer")
        cb:set(20, 1)
        assert(not cb:get(10), "value found beyond the start of the buffer")
        end,
    function()
        local cb = time_series.new(2, 1)
        assert(1 == cb:current_time(), "current time not 1")
        local v = cb:set(3, 0)
        assert(3 == cb:current_time(), "current time not 3")
        local v = cb:add(4, 0)
        assert(4 == cb:current_time(), "current time not 4")
        end,
    function()
        local cb = time_series.new(17, 1)
        for i,v in ipairs(data) do
            cb:add(i - 1, v)
        end
        local ts, rp, dist = cb:matrix_profile(nil, 16, 4, 100)
        assert(ts == 3, ts)
        assert(math.abs(rp - 68.356354) < .000001, rp)
        assert(math.abs(dist - 1.078937) < .000001, dist)

        local ts, rp, dist = cb:matrix_profile(nil, 16, 4, 100, "anomaly_current")
        assert(ts == 12, ts)
        assert(rp ~= rp, rp)
        assert(dist ~= dist, dist)

        local mp = cb:matrix_profile(nil, 16, 4, 100, "mp")
        local mp_len = #mp
        assert(mp_len == 13, mp_len)
        local mpi = cb:matrix_profile(nil, 16, 4, 100, "mpi")
        mp_len = #mpi
        assert(mp_len == 13, mp_len)

        local mp_ev = { 1.5010956572519172, 1.7133271671869412,
          1.4465117438199946, 2.2386180615118265, 1.4207401525040495,
          0.62038241908389491, 0.39903111714324457, 1.0783010406460811,
          0.17635816443144478, 0.62038241908389491, 0.17635816443144478,
          1.0783010406460811, 1.446511743819994 }
        local mpi_ev = { 7, 4, 12, 1, 11, 9, 10, 11, 10, 5, 8, 7, 2 }
        for i=0, mp_len - 1 do
            assert(math.abs(mp[i+1] - mp_ev[i+1]) < .000001, mp[i+1])
            assert(mpi[i+1] == mpi_ev[i+1], mpi[i+1])
        end
        end,
    function()
        local cb = time_series.new(17, 1)
        local ts, rp = cb:matrix_profile(nil, 16, 4, 100)
        assert(not ts, ts)
        assert(not rp, rp)
        end,
    function()
        local cb = time_series.new(6, 1)
        for i=0, 5 do
            cb:add(i, i)
        end
        local range = cb:get_range(1, 2)
        assert(#range == 2)
        assert(range[1] == 1)
        assert(range[2] == 2)
        end,
    function()
        local cb = time_series.new(6, 1)
        for i=0, 5 do
            cb:add(i, i)
        end
        local cb1 = time_series.new(10, 1)
        cb1:merge(cb, "set")
        local range = cb1:get_range(1, 2)
        assert(#range == 2)
        assert(range[1] == 1)
        assert(range[2] == 2)
        cb1:merge(cb1, "add")
        range = cb1:get_range(1, 2)
        assert(#range == 2)
        assert(range[1] == 2)
        assert(range[2] == 4)
        cb1:merge(cb1)
        range = cb1:get_range(1, 2)
        assert(#range == 2)
        assert(range[1] == 4)
        assert(range[2] == 8)
        end,
    function()
        local cb = time_series.new(6, 1)
        for i,v in ipairs{1,2,3,0,5,6} do
            cb:add(i, v)
        end
        local stat, rows = cb:stats(nil, 6, "sum")
        assert(stat == 17, stat)
        assert(rows == 5, rows)
        stat, rows = cb:stats(nil, 6, "sum", true)
        assert(stat == 17, stat)
        assert(rows == 6, rows)

        stat, rows = cb:stats(nil, 6, "min")
        assert(stat == 1, stat)
        assert(rows == 5, rows)
        stat, rows = cb:stats(nil, 6, "min", true)
        assert(stat == 0, stat)
        assert(rows == 6, rows)

        stat, rows = cb:stats(nil, 6, "max")
        assert(stat == 6, stat)
        assert(rows == 5, rows)
        stat, rows = cb:stats(nil, 6, "max", true)
        assert(stat == 6, stat)
        assert(rows == 6, rows)

        stat, rows = cb:stats(nil, 6, "avg")
        assert(stat == 3.4, stat)
        assert(rows == 5, rows)
        stat, rows = cb:stats(nil, 6, "avg", true)
        assert(math.abs(2.83333 - stat) < .00001, stat)
        assert(rows == 6, rows)

        stat, rows = cb:stats(nil, 6, "sd")
        assert(math.abs(2.07364 - stat) < .00001, stat)
        assert(rows == 5, rows)
        stat, rows = cb:stats(nil, 6, "sd", true)
        assert(math.abs(2.31660 - stat) < .00001, stat)
        assert(rows == 6, rows)

        stat, rows = cb:stats(nil, 6, "usd")
        assert(math.abs(1.85472 - stat) < .00001, stat)
        assert(rows == 5, rows)
        stat, rows = cb:stats(nil, 6, "usd", true)
        assert(math.abs(2.11476 - stat) < .00001, stat)
        assert(rows == 6, rows)
        end,
}

for i, v in ipairs(tests) do
  v()
end
