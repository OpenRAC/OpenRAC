/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 the OpenRAC contributors
 *
 * Up Your Arsenal's VU0 microprograms as native functions (trig.c): what the program the game
 * loads into VU0 at 0xC80 computes, with its arguments and results in plain C. */
#ifndef RAC3_TRIG_H
#define RAC3_TRIG_H

/* Entries 0xDA0 and 0xDB0: the cosine and the sine of an angle in radians. */
float rac3_cos(float angle);
float rac3_sin(float angle);

/* Entry 0xC80: the rotation rows of Euler angles (x, y, z), R = Rz.Ry.Rx; each row's w is 0. */
void rac3_euler_rows(float rows[3][4], const float angles[3]);

/* Entry 0xE98: the rotation rows of a unit quaternion (x, y, z, w); each row's w is 0. */
void rac3_quaternion_rows(float rows[3][4], const float q[4]);

/* func_0038DC70: `v` turned by `angle` radians about `axis` (any length), through the quaternion
 * of half the angle; w is v's. An angle below 2^-14 leaves v as it is. */
void rac3_turn_about_axis(float out[4], const float v[4], const float axis[3], float angle);

/* func_003D1DD8's rows: the turn by `angle` about the unit `axis`. */
void rac3_axis_angle_rows(float rows[3][3], const float axis[3], float angle);

#endif
