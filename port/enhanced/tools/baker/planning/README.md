# Baker planning

Builds local GI regions from gameplay evidence, collision coverage and lighting transitions.
`automatic_plan.h` assembles the plan; `region_plan.h` owns partitioning and `floor_matching.h`
retains deterministic matching against the playable floor graph.

See the [baker overview](../README.md) for flow, ownership, formats and verification.
