-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local p2 = require "streaming_algorithms.p2"

q = p2.quantile(0.5)
h = p2.histogram(4)

local data = { 0.02, 0.15, 0.74, 3.39, 0.83, 22.37, 10.15, 15.43, 38.62, 15.92,
    34.60, 10.28, 1.47, 0.40, 0.05, 11.39, 0.27, 0.42, 0.09, 11.37}

function process(ts)
    for i,v in ipairs(data) do
        q:add(v)
        h:add(v)
    end
    return 0
end

function report(tc)
    write_output(q:count(4), " ", h:count(4))
end
