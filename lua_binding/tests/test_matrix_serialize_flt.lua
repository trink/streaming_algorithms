-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local matrix = require "streaming_algorithms.matrix"

m = matrix.new(3, 1, "float")

function process()
    m:set(1, 1, 33)
    m:set(2, 1, 1/0)
    m:set(3, 1, 44)
    return 0
end

function report(tc)
    write_output(m:get(1, 1), " ", m:get(2, 1), " ", m:get(3, 1))
end
