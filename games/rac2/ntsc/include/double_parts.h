#ifndef RAC2_DOUBLE_PARTS_H
#define RAC2_DOUBLE_PARTS_H

/* The shared software-double representation passed to pack/unpack helpers.
   The fraction is at offset 16 in the measured 24-byte EE record. */
typedef struct DoubleParts {
    int kind;
    unsigned int sign;
    int exponent;
    /* Keep the public fp-bit.c union: its aliasing rules affect scheduling. */
    union {
        unsigned long long ll;
        unsigned int l[2];
    } fraction;
} DoubleParts;

typedef char DoublePartsSizeMustBe24[(sizeof(DoubleParts) == 24) ? 1 : -1];

#endif
