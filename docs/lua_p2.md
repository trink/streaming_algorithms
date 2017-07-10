# Lua Piecewise Parabolic Prediction (P2) Module

## Overview
Dynamic calculations of quatiles and histograms without storing observations.
The module is globally registered and returned by the require function.

## Example Usage
```lua
require "streaming_algorithms.p2"
local q = streaming_algorithms.p2.quantile(0.5)
for i = 1, 100 do
    q:add(i)
end
local cnt = q:count(2)
-- cnt == 50
```

## Module

### Functions

#### quantile
```lua
local p2 = require "streaming_algorithms.p2"
local q = p2.quantile(0.5)
```

Creates a new quantile userdata object.

*Arguments*
- p (number) p_quantile to calculate (e.g. 0.5 == median)

*Return*
- quantile userdata object

#### histogram
```lua
local p2 = require "streaming_algorithms.p2"
local h = p2.histogram(4)
```

Creates a new histogram userdata object.

*Arguments*
- buckets (integer) Number of histogram buckets (4-65534)

*Return*
- histogram userdata object

### Quantile Methods

#### add
```lua
q:add(1.3243)
```

Add the value to the quantile.

*Arguments*
- value (number)

*Return*
- quantile (number) NaN until five samples have been added

#### clear
```lua
q:clear()
```

Resets the quantile to its initial state.

*Arguments*
- none

*Return*
- none

#### count
```lua
local count = q:count(marker)
```

Returns the number of observations that are less than or equal to the specified
marker.

*Arguments*
- marker (integer) Selects the percentile
    * 0 = min
    * 1 = p/2
    * 2 = p
    * 3 = (1+p)/2
    * 4 = max

*Return*
- count (integer)

#### estimate
```lua
local estimate = q:estimate(marker)
```

Returns the estimated quantile value for the specified marker.

*Arguments*
- marker (integer) 0-4 see `count`

*Return*
- estimate (number)


#### fromstring
```lua
q:fromstring(tostring(q1))
```

Restores the quantile to the previously serialized state.

*Arguments*
- serialization (string) tostring output

*Return*
- none or throws an error


### Histogram Methods

#### add
```lua
d:add(1.3243)
```

Add the value to the histogram.

*Arguments*
- value (number)

*Return*
- none

#### clear
```lua
h:clear()
```

Resets the histogram to its initial state.

*Arguments*
- none

*Return*
- none

#### count
```lua
local count = h:count(marker)
```

Returns the number of observations that are less than or equal to the specified
marker.

*Arguments*
- marker (integer) Selects the percentile (marker/buckets)

*Return*
- count (integer)

#### estimate
```lua
local estimate = q:estimate(marker)
```

Returns the estimated quantile value for the specified marker.

*Arguments*
- marker (integer) Selects the percentile (marker/buckets)

*Return*
- estimate (number)

#### fromstring
```lua
h:fromstring(tostring(h1))
```

Restores the histogram to the previously serialized state.

*Arguments*
- serialization (string) tostring output

*Return*
- none or throws an error
