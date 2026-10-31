# Baker output

Owns encoded GI output, adaptive-density verification, reflection capture and final registration.
`batch_output.h` consumes traced grids without copying their maps. Every pending output finishes
before `batch_registration.cpp` can register the level.

See the [baker overview](../README.md) for flow, ownership, formats and verification.
