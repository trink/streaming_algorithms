# Lua Time Series Module

## Overview
A circular buffer library for an in-memory sliding window time series data
store.

## Module

### Example Usage
```lua
local sats = require "streaming_algorithms.time_series"

local ts = sats.new(1440, 60e9)
ts:add(1e9, 1)
ts:add(1e9, 7)
local val = ts:get(1e9)
-- val == 8
```
### Functions

#### new
```lua
require "streaming_algorithms.time_series"
local ts = streaming_algorithms.time_series.new(1440, 60e9)
```

Import the _time_series_ via the Lua 'require' function. The module is globally
registered and returned by the require function.

*Arguments*
- rows (unsigned) The number of rows in the buffer (must be > 1).
- ns_per_row (unsigned) The number of nanoseconds each row represents
  (must be > 0).

*Return*
- time_series userdata object.

### Methods

#### add
```lua
v = ts:add(1e9, 1)
-- v == 1
v = ts:add(1e9, 99)
-- v == 100
```

Adds a value to the specified row in the time series.

*Arguments*
- nanosecond (unsigned) The number of nanosecond since the UNIX epoch. The value
  is used to determine which row is being operated on.
- value (number) The value to be added to the specified row/column.

*Return*
- The value of the updated row or nil if the time was outside the range
  of the window.

#### set
```lua
v = ts:set(1e9, 1)
-- v == 1
v = ts:set(1e9, 99)
-- v == 99
```

Overwrites the value at a specific row in the time series.

*Arguments*
- nanosecond (unsigned) The number of nanosecond since the UNIX epoch. The value
  is used to determine which row is being operated on.
- value (number) The value to be overwritten at the specified row.

*Return*
- The resulting value of the row or nil if the time was outside the range
  of the window.

#### get
```lua
v = ts:get(1e9)
-- v == 99
```

Fetches the value at a specific row in the time series.

*Arguments*
- nanosecond (unsigned) The number of nanosecond since the UNIX epoch. The value
  is used to determine which row is being operated on.

*Return*
- The value at the specifed row or nil if the time was outside the range
  of the window.

#### current_time
```lua
t = ts:current_time()
-- t == 86340000000000

```

Returns the timestamp of the newest row.

*Arguments*
- none

*Return*
- The time of the most current row in the time series (nanoseconds).

#### fromstring
```lua
ts:fromstring(tostring(ts1))
```

Restores the time series to the previously serialized state.

*Arguments*
- serialization (string) The tostring output.

*Return*
- none or throws an error.
