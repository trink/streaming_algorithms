-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local time_series = require "streaming_algorithms.time_series"

ts = time_series.new(2, 1)

function process()
    ts:set(3, 33)
    ts:set(4, 44)
    return 0
end

function report(tc)
    write_output(ts:get(3), " ", ts:get(4))
end
