# Lua Matrix Module

## Overview
The matrix data structure module is globally registered and returned by the
Lua require function.

## Module

### Example Usage
```lua
local matrix = require "streaming_algorithms.matrix"

local m = matrix.new(20, 10)
m:add(1, 1, 99)
m:add(20, 10, 100)
local val = m:get(1, 1)
-- val == 99
```
### Functions

#### new
```lua
require "streaming_algorithms.matrix"
local m = streaming_algorithms.matrix.new(20, 10)
```

*Arguments*
- rows (unsigned) The number of rows in the matrix (must be > 1).
- cols (unsigned) The number of columns in the matrix (must be > 1).
- type (nil/string) int|float (default int)

*Return*
- matrix userdata object.

### Methods

#### get_configuration
```lua
local rows, columns = m:get_configuration()
```

Returns the configuration of the matrix structure.

*Arguments*
- none
-
*Return*
- rows (unsigned) The number of rows in the matrix.
- columns (unsigned) The number of columns in the matrix.

#### add
```lua
v = m:add(1, 1, 1)
-- v == 1
v = m:add(1, 1, 99)
-- v == 100
```

Adds a value to the specified matrix cell.

*Arguments*
- row (unsigned)
- column (unsigned)
- value (number) The value to be added to the specified row/column.

*Return*
- The resulting value at the specified cell or nil if the request was out of
  bounds.

#### set
```lua
v = m:set(1, 1, 1)
-- v == 1
v = m:set(1, 1, 99)
-- v == 99
```

Overwrites the value at a specific cell.

*Arguments*
- row (unsigned)
- column (unsigned)
- value (number) The value to be overwritten at the specified row.

*Return*
- The value at the specified cell or nil if the request was out of bounds.

#### get
```lua
v = m:get(1, 1)
-- v == 99
```

Fetches the value at a specific cell.

*Arguments*
- row (unsigned)
- column (unsigned)

*Return*
- The value at the specified cell or nil if the request was out of bounds.

#### get_row
```lua
a = m:get_row(1)
-- a == {98, 99}
```

Returns an array of values containing column entries for the row.

*Arguments*
- row (unsigned)

*Returns*
- Array of values or nil if the request was out of bounds.


#### clear_row
```lua
m:clear_row(1)
```

Zeros out the specified row in the matrix.

*Arguments*
- row (unsigned)

*Returns*
- none


#### pcc
```lua
pcc, index = m:pcc(1)
```

Returns Pearson's correlation coefficient of the most similar/different row in
the matrix.

*Arguments*
- row (unsigned)
- match (string) "max|min", default "max".

*Returns*
- pcc (number/nil) Pearson's correlation coefficient.
- index (int/nil) Row index of the best match.


#### fromstring
```lua
m:fromstring(tostring(m1))
```

Restores a matrix to the previously serialized state.

*Arguments*
- serialization (string) The tostring output.

*Return*
- none or throws an error.
