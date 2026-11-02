/* No C attempt: the USA v1.01 boot catalogue gives func_003356E8 a 172-byte
   extent from 0x003356E8 to 0x00335794. It begins with `sb $a0,0x88($v1)`, then
   contains nine positive stack adjustments with delay-slot nops and three
   stores, followed by further stack adjustments and nops. It has no function
   prologue, control-flow body or return. The preceding catalogued function
   func_003355A8 ends at 0x003356E4; func_00335798 begins at 0x00335798.
   A scan of USA v1.01 boot load segments found no direct branch/jump target or
   aligned literal pointer into this extent. This supports a catalogue-residue
   hypothesis, but does not establish release-independent boundaries. Leave
   the marker and catalogue row for maintainer review; no generator rule was
   added. */
