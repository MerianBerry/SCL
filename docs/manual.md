# SCL Usage Manual

Version 1.0

## Including

SCL provides a single header form as **miniscl.hpp**.

All you need to do to use this header, is to include it, and define `SCL_IMPL` in ONE .cpp file.

Any source file that does not need to implement SCL, can simply include **miniscl.hpp**.

### Examples

**miniscl.hpp** example

```cpp
#define SCL_IMPL
#include "miniscl.hpp"
```

## Initializing

SCL has some structures that need to be initialized before they are used, namely scl::pack.

### Example

SCL initialization example

```cpp
int main() {
  scl::init();

  // Your code here

  scl::terminate();
}
```
