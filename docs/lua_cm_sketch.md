# Lua Count-min Sketch Module

## Overview
Calculates the frequency of an item in a stream. The module is globally
registered and returned by the require function.

## Example Usage
```lua
require "streaming_algorithms.cm_sketch"
local cms = streaming_algorithms.cm_sketch.new(0.01, 0.01)
for i = 1, 100 do
    cms:update(i)
end
local cnt = cms:point_query(1)
-- cnt == 1
```

## Module

### Functions

#### new
```lua
local cm_sketch = require "streaming_algorithms.cm_sketch"
local cms = cm_sketch.new(0.01, 0.01)
```

Creates a new cm_sketch userdata object.

*Arguments*
- epsilon (number) approximation factor
- delta (number) probability of failure

*Return*
- cm_sketch userdata object

### Methods

#### update
```lua
local estimate = cms:update("foo")
```

Update the count for the specified item in the sketch.

*Arguments*
- key (string/number) item identifier
- n (number/nil/none) number of items (default 1), a negative value removes
  items

*Return*
- estimate (integer) estimated frequency count

#### point_query
```lua
local estimate = cms:point_query("foo")
```

Retruns the frequency for the specified item in the sketch.

*Arguments*
- key (string/number) item identifier

*Return*
- estimate (integer) estimated frequency count

#### clear
```lua
cms:clear()
```

Resets the count-min sketch.

*Arguments*
- none

*Return*
- none

#### item_count
```lua
local count = cms:item_count()
```

Returns the total number items in the sketch.

*Arguments*
- none

*Return*
- count (number)

#### unique_count
```lua
local count = cms:unique_count()
```

Returns the total number unique items in the sketch.

*Arguments*
- none

*Return*
- count (number)

#### fromstring
```lua
cms1:fromstring(tostring(cms))
```

Restores the sketch to the previously serialized state (must have a compatible
epsilon and delta).

*Arguments*
- serialization (string) tostring output

*Return*
- none or throws an error
