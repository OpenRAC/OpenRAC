/* func_0020E3D0: MobyAnimAdvance, the per-moby animation step of MobyUpdateLoop. Moves the key
   pair (frame A = m->frame, frame B = m->prevFrame, t at +0x54) on by speed * rate (+0x58, +0x5C),
   stepping keys forward or back; each key takes its rate from the sequence's override or from its
   own frame header. Then the sequence's triggers for the key times passed play (PlayClassSound),
   and the loop sound is refreshed (func_0020D790) on the frame-spread schedule of D_0015F6F0. */
extern int func_0022ED80(int, int, int);

void func_0020E3D0(Moby *m) {
    unsigned char *mp = (unsigned char *)m;
    unsigned int speedBits = *(unsigned int *)(mp + 0x58);
    unsigned int rateBits = *(unsigned int *)(mp + 0x5C);
    float speed = *(float *)(mp + 0x58);
    float rate = *(float *)(mp + 0x5C);
    float tOld = *(float *)(mp + 0x54);
    float t;
    unsigned char seqA = m->seq;
    unsigned char seqB = m->prevSeq;
    unsigned int oldA = m->frame;
    unsigned int flags = 0;
    unsigned int loop = m->unk7C;
    unsigned int voice = m->unk7D;
    unsigned int trig = m->unk7E;
    MobySeq *seq;
    int ti;
    int snap;
    int newA;
    int newB;
    int fc;
    int ovr;
    int oldBptr;
    int newBptr;
    int newAptr;

    if (rateBits == 0 || speedBits == 0) {
        goto tail;
    }

    seq = m->pClass->seqs[seqB];
    if (seqA == seqB) {
        t = (0.0f + tOld) + speed * rate;
    } else {
        t = tOld + rate;
    }

    ti = *(int *)&t;
    snap = (ti >= 0x3F7F0000 && ti <= 0x3F808000);
    if (snap || ti > 0x3F800000) {
        /* forward: the key passes the end of its interval, possibly more than once */
        if (snap) {
            t = 1.0f;
        }
        do {
            t = t - 1.0f;
            newA = m->prevFrame;
            newB = newA + 1;
            oldBptr = m->prevFrameData;
            if (seqA != seqB) {
                m->unk7E = seq->unk12;
                seqA = seqB;
                loop = 0xFF;
                voice = 0xFF;
                trig = 0;
            }
            t = t / rate;
            flags |= 1;
            fc = ((unsigned char *)seq)[0x10];
            ovr = *(int *)((char *)seq + 0x18);
            if (fc - newB <= 0) {
                newB = 0;
                flags |= 2;
            }
            newBptr = seq->frames[newB];
            rate = *(float *)&ovr;
            if (ovr == 0) {
                rate = *(float *)(oldBptr);
            }
            t = t * rate;
            mp[0x52] = seqA;
            m->frame = newA;
            m->prevFrame = newB;
            m->frameData = oldBptr;
            m->prevFrameData = newBptr;
            *(float *)(mp + 0x5C) = rate;
            *(float *)(mp + 0x54) = t;
        } while (*(int *)&t > 0x3F800000);
        mp[0x70] = flags;
        goto after;
    } else if (ti < 0) {
        /* backward: the key passes its start, possibly more than once */
        do {
            t = t / rate;
            newB = m->frame;
            newA = newB - 1;
            flags |= 1;
            ovr = *(int *)((char *)seq + 0x18);
            if (newA < 0) {
                fc = ((unsigned char *)seq)[0x10];
                newA = fc - 1;
                flags |= 2;
            }
            oldBptr = m->frameData;
            newAptr = seq->frames[newA];
            rate = *(float *)&ovr;
            if (ovr == 0) {
                rate = *(float *)(newAptr);
            }
            t = t * rate;
            m->frameData = newAptr;
            m->frame = newA;
            m->prevFrame = newB;
            t = t + 1.0f;
            m->prevFrameData = oldBptr;
            *(float *)(mp + 0x5C) = rate;
            *(float *)(mp + 0x54) = t;
        } while (*(int *)&t < 0);
        mp[0x70] = flags;
        goto after;
    } else {
        *(float *)(mp + 0x54) = t;
    }

after:
    mp[0x70] = flags;
    if (loop == 0xFF && voice == 0xFF && trig == 0) {
        return;
    }
    if (seqA != seqB) {
        goto tail;
    }
    if (trig != 0) {
        /* triggers: the first whose time falls in (old position, new position] plays */
        int curA = m->frame;
        int newPos = (curA << 4) + (int)(t * 16.0f);
        int oldPos = ((int)oldA << 4) + (int)(tOld * 16.0f);
        if (newPos - oldPos > 0) {
            unsigned int *tw;
            int k;
            fc = ((unsigned char *)seq)[0x10];
            tw = (unsigned int *)((char *)seq + 0x1C + fc * 4);
            for (k = 0; k < (int)trig; k++) {
                int time = (int)(tw[k] >> 16);
                if (newPos - time >= 0 && time - (oldPos + 1) >= 0) {
                    func_0022ED80((int)(tw[k] & 0xFFFF), 0, (int)m);
                    return;
                }
            }
        }
    }

tail:
    mp[0x70] = flags;
    if (loop == 0xFF && voice == 0xFF) {
        return;
    }
    if (voice != 0xFF) {
        func_0020D790(mp);
        return;
    }
    if ((((unsigned int)mp >> 8) & 3) == ((unsigned int)D_0015F6F0 & 3)) {
        func_0020D790(mp);
    }
}
