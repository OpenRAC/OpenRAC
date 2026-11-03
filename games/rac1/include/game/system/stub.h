#ifndef GAME_SYSTEM_STUB_H
#define GAME_SYSTEM_STUB_H

/* Functions defined in src/game/system/stub.c. */

#include "common.h"

extern void Profile_Mark(void *);
extern void STUB_printf(void *, ...);
extern void noop_callback_a(void *, void *, int, int, int);
extern void noop_callback_b(void *);
extern void noop_callback_c(void *);
extern void noop_callback_d(void);
extern void noop_callback_e(void *);
extern void noop_callback_f(void *);
extern void noop_callback_g(void);
extern void noop_callback_h(void);
extern void noop_callback_i(void);
extern void noop_callback_j(void);
extern void noop_callback_k(void *, void *, int, int);
extern int noop_callback_l(void *);
extern void noop_callback_m(void *, void *);
extern void noop_callback_n(int);
extern void noop_callback_o(int);
extern int noop_callback_p(int);
extern void noop_callback_q(int, int);
extern void noop_callback_r(void *, int);
extern void noop_callback_s(void *, void *, int, int, int);

#endif /* GAME_SYSTEM_STUB_H */
