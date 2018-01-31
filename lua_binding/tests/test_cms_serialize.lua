-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local cm_sketch = require "streaming_algorithms.cm_sketch"

cms = cm_sketch.new(0.01, 0.01)

function process(ts)
    cms:update("a")
    cms:update("b")
    cms:update("c", 3)
    return 0
end

function report(tc)
    write_output(cms:item_count(), " ", cms:unique_count())
end
