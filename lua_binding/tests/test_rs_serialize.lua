-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local rs = require "streaming_algorithms.running_stats"

stat = rs.new()

function process()
    for i=1, 10 do
        stat:add(i)
    end
    return 0
end

function report(tc)
    write_output(stat:count())
end
