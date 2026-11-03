# Baker pipeline

Owns automatic level admission, shared worker scheduling, cache identity, logs and publication.
The entry point is `pipeline.h`; `pipeline_internal.h` describes controller-owned state.
JSON helpers and staged file replacement are shared with the preparation/output stages.

See the [baker overview](../README.md) for flow, ownership, formats and verification.
