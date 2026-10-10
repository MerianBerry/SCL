# SCL

Stormground C/C++ (utility) Library

## Goals

Provide general and fast utilities for games, and any project that wishes to use this project.

### libscl (core/C api)
* [x] Extended string manipulation functions
* [x] Filepath manipulation functions
* [x] Filesystem access via libuv
* [x] Filesystem glob-ing functions 
* [x] Swiss-table implementation
* [x] High resolution clock/sleep functions
* [ ] Child process creation/manipulation via libuv
* [ ] Asset packager

### C++ bindings & additions
* [ ] C++ bindings for the above, minus the swiss table implementation
* [ ] Extended string class from std::string, with some string processing additions
* [x] IO/memory stream
* [x] Fast XML parser & printer

## Documentation

- [Manual](docs/manual.md)
- [Full API Documentation](https://merianberry.github.io/SCL/)
