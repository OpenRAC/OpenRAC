typedef int TI128 __attribute__((mode(TI)));

extern int func_002AB1F0(char *);
extern int func_002ABB78(char *);
extern float func_00283248(float, float);
extern float func_00283C80(float, float);
extern void func_00282D30(void *, void *, float);
extern void func_00282CF0(void *, void *, void *);
extern float func_00282DF0(void *);
extern float func_00282DB0(void *, void *);
extern float func_002A7AA8(float, float, float, float, float);
extern void func_00282CC0(void *, void *, void *);
extern float func_00282E20(void *);
extern float func_002831B0(float);
extern int func_002761F8(void *, void *, int, void *, int);
extern int func_003004E0(int);
extern void func_002AFFD8(void *, void *, int);

struct SelectionState {
    char pad[0x18];
    char *target;
};

extern int D_001A8CA0 __attribute__((sda));
extern short D_0018A168;
extern int D_001A79F0 __attribute__((sda));
extern struct SelectionState D_001BAF80;
extern char *ref_002AF460_00[];

/* Walks the object list, rejects entries by type, range and the two angle limits around
   `direction`, and returns the entry with the lowest score. Accepted entries go to `out`
   with the best one moved to the front. */
char *func_002AF580(float *origin, float *direction, float angle, float vertical, int *count, char **out,
                    int capacity, char **exclude, float range, float verticalMin, float angleMin,
                    int allowSpecial, int *types)
{
    float pos[4], offset[4], delta[4], axis[4], projection[4], side[4], component[4], other[4], combined[4];
    char *best = 0;
    char *node;
    float bestScore = 999999.0f;
    int index;
    int special;
    int i;
    int j;
    int k;
    float *info;
    float zero;
    float lenCombined, scaled, dot2, dist, fraction, horizontal, elevation, limit, dot, lenSide, lenOther, score, temp;

    *count = 0;
    if (angle == 0.0f || vertical == 0.0f || range == 0.0f)
        return 0;
    index = 0;
    node = ref_002AF460_00[0];
    while (node) {
        if (!((unsigned char *)node)[0x31])
            goto next;
        special = func_002ABB78(node);
        if (special && !allowSpecial)
            goto next;
        info = (float *)func_002AB1F0(node);
        if (!info)
            goto next;
        if (*info <= 0.0f && !special)
            goto next;
        if (!(node && *(char **)(node + 0x24) && *(short *)(*(char **)(node + 0x24) + 0x46) == 5) && !special)
            goto next;
        if (exclude) {
            for (k = 0; exclude[k] && node != exclude[k]; k++)
                ;
            if (exclude[k])
                goto next;
        }
        if (types) {
            for (j = 0; types[j] != -1 && *(short *)(node + 0xAA) != types[j]; j++)
                ;
            if (types[j] != -1)
                goto next;
        }
        *(TI128 *)pos = *(TI128 *)(node + 0x10);
        if (D_001A8CA0 || D_0018A168) {
            func_00282D30(offset, node + 0xE0, info[4]);
            func_00282CC0(pos, pos, offset);
        } else {
            pos[2] += info[4];
        }
        func_00282CF0(delta, pos, origin);
        dist = func_00282DF0(delta);
        if (range < dist || dist < 0.1f)
            goto next;
        zero = 0.0f;
        fraction = func_002A7AA8(-2.0f, zero, 1.0f, zero, dist / range);
        horizontal = zero;
        elevation = horizontal;
        if (angle < 3.141593f || vertical < 3.141593f) {
            if (!D_001A8CA0) {
                temp = func_00283248(delta[0], delta[1]);
                horizontal = func_00283C80(temp, func_00283248(direction[0], direction[1]));
                limit = angle;
                if (zero < angleMin)
                    limit = angleMin + (angle - angleMin) * fraction;
                if (limit <= horizontal)
                    goto next;
                temp = func_00283248(func_00282E20(delta), delta[2]);
                elevation = func_00283C80(temp, func_00283248(func_00282E20(direction), direction[2]));
                limit = vertical;
                if (zero < verticalMin)
                    limit = verticalMin + (vertical - verticalMin) * fraction;
                if (limit <= elevation)
                    goto next;
            } else {
                func_002AFFD8(origin, axis, 1);
                func_00282D30(projection, axis, func_00282DB0(axis, delta));
                func_00282CF0(side, delta, projection);
                temp = func_00282DB0(axis, direction);
                func_00282D30(component, axis, temp);
                func_00282CF0(other, direction, component);
                dot = func_00282DB0(other, side);
                if (dot < zero)
                    goto next;
                lenSide = func_00282DF0(side);
                if (lenSide == zero)
                    goto next;
                lenOther = func_00282DF0(other);
                if (lenOther == zero)
                    goto next;
                horizontal = 1.57079637f - func_002831B0(dot / (lenSide * lenOther));
                limit = angle;
                if (zero < angleMin)
                    limit = angleMin + (angle - angleMin) * fraction;
                if (limit <= horizontal)
                    goto next;
                func_00282D30(combined, side, lenOther / lenSide);
                func_00282CC0(combined, combined, component);
                lenCombined = func_00282DF0(combined);
                dot2 = func_00282DB0(combined, delta);
                elevation = 1.57079637f - func_002831B0(dot2 / (lenCombined * dist));
                limit = vertical;
                if (zero < verticalMin)
                    limit = verticalMin + (vertical - verticalMin) * fraction;
                if (limit <= elevation)
                    goto next;
            }
        }
        score = horizontal * 3.0f + elevation * 0.2f * dist + dist * 0.1f;
        if (special) {
            scaled = score * 0.8f;
            score = scaled * range;
        }
        if (score < bestScore) {
            if (!func_002761F8(origin, pos, 18, node, 0))
                goto found;
            if (D_001A79F0 == 11) {
                char *target = D_001BAF80.target;
                if (!target)
                    goto next;
                if (*(short *)(target + 0xAA) == 0xC20)
                    goto found;
            }
            {
                char *target = D_001BAF80.target;
                if (!target)
                    goto next;
                if (!func_003004E0(*(short *)(target + 0xAA)))
                    goto next;
            }
        found:
            bestScore = score;
            best = node;
        }
        if (out && *count < capacity)
            out[(*count)++] = node;
    next:
        node = ref_002AF460_00[++index];
    }
    if (out) {
        if (best) {
            for (i = 0; i < capacity; i++) {
                if (out[i] == best) {
                    if (i != 0) {
                        char *first = out[0];
                        out[0] = best;
                        out[i] = first;
                    }
                    break;
                }
            }
            if (i == capacity)
                out[0] = best;
        }
        if (*count < capacity)
            out[*count] = 0;
    }
    return best;
}
