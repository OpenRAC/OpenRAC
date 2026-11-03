/* Catalogue review pending: the 76-byte USA v1.01 extent at 0x002B80B8 is
 * only positive stack-pointer adjustments and `nop`s; it contains no branch
 * or return before func_002B8108. Verify surrounding control flow and catalogue
 * ownership before changing generated data. */
