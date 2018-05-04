# Lua Running Stats Module

## Overview
Calculates the running count, mean, variance, and standard deviation. The module
is globally registered and returned by the require function.

## Example Usage
```lua
require "streaming_algorithms.running_stats"
local stat = streaming_algorithms.running_stats.new()
for i = 1, 100 do
    stat:add(i)
end
local cnt = stat:count()
-- cnt == 100
```

## Module

### Functions

#### new
```lua
local rs = require "streaming_algorithms.running_stats"
local stat = rs.new()
```

Creates a new running_stats userdata object.

*Arguments*
- none

*Return*
- running_stats userdata object

### Methods

#### add
```lua
stat:add(1.3243)
```

Add the value to the running stats.

*Arguments*
- value (number)

*Return*
- none

#### avg
```lua
local avg = stat:avg()
```

Returns the current average.

*Arguments*
- none

*Return*
- avg (number)

#### clear
```lua
stat:clear()
```

Resets the stats to zero.

*Arguments*
- none

*Return*
- none

#### count
```lua
local count = stat:count()
```

Returns the total number of values.

*Arguments*
- none

*Return*
- count (number)

#### fromstring
```lua
stat:fromstring(tostring(stat1))
```

Restores the stats to the previously serialized state.

*Arguments*
- serialization (string) tostring output

*Return*
- none or throws an error

#### sd
```lua
local sd = stat:sd()
```

Returns the current corrected sample standard deviation.

*Arguments*
- none

*Return*
- sd (number)

#### usd
```lua
local sd = stat:usd()
```

Returns the current uncorrected sample standard deviation.

*Arguments*
- none

*Return*
- sd (number)

#### variance
```lua
local variance = stat:variance()
```

Returns the current variance.

*Arguments*
- none

*Return*
- variance (number)
