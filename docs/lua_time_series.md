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

#### get_configuration
```lua
local rows, ns_per_row = ts:get_configuration()
```

Returns the configuration of the time series structure.

*Arguments*
- none
-
*Return*
- rows (unsigned) The number of rows in the buffer.
- ns_per_row (unsigned) The number of nanoseconds each row represents.

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

#### merge
```lua
ts:merge(ts1, op)
```

Merges one time series into another based on the specified operation. The
resolution of time series being merged must be smaller or equal to the
destination resolution.

*Arguments*
- ts1 (userdata) Time series userdata to merge.
- op (string/nil) One of the following entries:
    - add (default)
    - set

*Return*
- none

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

#### get_range
```lua
a = ts:get_range(nil, 2)
-- a == {98, 99}
```

Returns an array of values spanning the specified range.

*Arguments*
- nanoseconds (unsigned/nil) The start of the interval to return, nil starts
  from the beginning.
- sequence_length (unsigned) Time series length (<= rows).

*Returns*
- Array of values or nil if the range fell outside of the buffer.

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

#### stats
```lua
sum, cnt = ts:stats(nil, 10, "sum")
-- sum == 23, cnt = 10
```

Returns the requested type of stats specified range.

*Arguments*
- nanoseconds (unsigned/nil) The start of the interval to return, nil starts
  from the beginning.
- sequence_length (unsigned) Time series length (<= rows).
- type (string/nil) One of the following entries:
    - sum (default)
    - min
    - max
    - avg
    - sd    - corrected standard deviation
    - usd   - uncorrected standard deviation
- include_zero (bool/nil) - Treat a zero value as a value instead of unitialized
  (default: false, integer time series only).

*Returns*
- stat (number) Resulting `type` output
- rows (number) Number of rows used in the computation.

#### matrix_profile
```lua
local ts, rp, dist = ts:matrix_profile(nil, 16, 4, 100, "anomaly")
-- ts == 1525465122000000000
-- rp == 35.762825
-- dist == 1.078937

local mp = ts:matrix_profile(1e9, 120, 10, 1.0, "mp")
-- mp == {1.5010956572519172, 1.7133271671869412, ... }

local mpi = ts:matrix_profile(1e9, 120, 10, 1.0, "mpi")
-- mpi == {7, 4, ... }

```

Returns the requested information from the
[matrix profile](http://www.cs.ucr.edu/~eamonn/MatrixProfile.html) calculation.

*Arguments*
- nanoseconds (unsigned/nil) The start of the interval to analyze, nil starts
  from the beginning.
- sequence_length (unsigned) Time series length (<= rows).
- subsequence_length (unsigned) (sequence_length / 4 >= subsequence_length > 3).
- percent (number) Percentage of data to base the calculation on
  (0.0 < percent <= 100). Use less than 100 to produce an estimate of the
  matrix profile trading accuracy for speed.
- result (string/nil) One of the following (anomaly|anomaly_current|mp|mpi)
  - `anomaly` (default) Returns the timestamp, the percentage of the range
  represented by the top 5% of discords and the distance between the top discord
  and the median.
  - `anomaly_current` Same as `anomaly` but only over the last
  `subsequence_length` of matrix profiles.
  - `mp` Returns the matrix profile array.
  - `mpi` Returns the matrix profile index array.

*Return*
- The specified result described above, nil (out of range) or throws an error.

#### fromstring
```lua
ts:fromstring(tostring(ts1))
```

Restores the time series to the previously serialized state.

*Arguments*
- serialization (string) The tostring output.

*Return*
- none or throws an error.
