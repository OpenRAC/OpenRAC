/* func_00282C68 - 16 bytes, 28 places (boot + 27 levels). Retail is
   `max.s $f0,$f12,$f13; nop; jr $ra; min.s $f0,$f0,$f14`. The clamp below
   reaches the same operations but not the retail FPR allocation or delay-slot
   schedule. Under -O1 -G0 -ffunction-sections -ffast-math it is 12 bytes
   (max.s into $f12; jr; min.s in the non-delay return position), 0/28. Under
   -O2 -G0 -ffunction-sections -ffast-math it is 16 bytes, 0/28: a move of
   $f14 into $f0 is inserted and the final min.s uses $f12 rather than $f14.
   Nested conditional, if-chain, and builtin fminf/fmaxf forms were also tried;
   none matched. A 29 October -fno-schedule-insns replay under the O2 fast-math
   profile produced `mov.s $f0,$f14; max.s $f12,$f12,$f13; jr $ra;
   min.s $f0,$f0,$f12`, still 0/28 and differing in all three FPR operands/
   positions. An isolated -fno-schedule-insns2 profile is 16/16 and 0/28:
   max.s moves first and min.s fills the jr delay slot, but the max destination
   and min source remain the wrong FPRs. Reversing both comparison operand
   orders is byte-identical; the fast-math if-chain is 44/16. No register pins
   or artificial barriers were used. */
float func_00282C68(float a0, float a1, float a2) {
    float upper = a0 < a1 ? a1 : a0;
    return upper < a2 ? upper : a2;
}
