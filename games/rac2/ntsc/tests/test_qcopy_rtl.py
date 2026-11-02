"""Execute the real qcopy expansion against a small authored RTL API model."""
import importlib.util
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location("qcopy_pattern", ROOT / "tools/compiler/transformers/qcopy_pattern.py")
transformer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(transformer)


class ExpansionContractTests(unittest.TestCase):
    def test_memory_attributes_addresses_and_fallback_guards(self):
        compiler = shutil.which("cc") or shutil.which("gcc")
        if compiler is None:
            self.skipTest("requires a host C compiler; CI and the build image provide one")
        # The generated insn is an external API to the expansion. Its fixed scratch
        # is checked against the real pattern; allocation is tested by test_qcopy.py.
        self.assertRegex(transformer.PATTERN, r"\(clobber\s+\(reg:TI\s+2\)\)")
        source = r'''
#include <assert.h>
#include <string.h>

enum { ADDR, REG, MEM, SET, PARALLEL, CLOBBER };
enum { SImode, TImode, DImode };
struct node {
    int code, mode, tag;
    struct node *exp[2], *vec[2];
    int alias_set, is_volatile, in_struct, scalar, readonly_p, unaligned;
};
typedef struct node *rtx;
#define GET_CODE(X) ((X)->code)
#define XEXP(X,I) ((X)->exp[I])
#define XVECEXP(X,V,I) ((X)->vec[I])
#define SET_DEST(X) ((X)->exp[0])
#define SET_SRC(X) ((X)->exp[1])
#define MEM_ALIAS_SET(X) ((X)->alias_set)
#define MEM_UNALIGNED_P(X) ((X)->unaligned)
#define DONE return 1

static int flag_rac2_qcopy, Pmode, reload_in_progress, reload_completed;
static struct node pool[32], hard_scratch, scratch_clobber;
static int used, force_count, emit_count;
static rtx forced[2], emitted;

static rtx node(int code, int mode) {
    rtx n;
    assert(used < 32);
    n = &pool[used++];
    memset(n, 0, sizeof(*n));
    n->code = code;
    n->mode = mode;
    return n;
}
static rtx force_reg(int mode, rtx address) {
    rtx n;
    assert(mode == SImode && force_count < 2);
    forced[force_count] = address;
    n = node(REG, mode);
    n->tag = 50 + force_count++;
    n->exp[0] = address;
    return n;
}
static rtx gen_rac2_qcopy(rtx dest, rtx src) {
    rtx copy = node(PARALLEL, 0);
    rtx set = node(SET, 0);
    copy->vec[0] = set;
    copy->vec[1] = &scratch_clobber;
    set->exp[0] = node(MEM, TImode);
    set->exp[1] = node(MEM, TImode);
    set->exp[0]->exp[0] = dest;
    set->exp[1]->exp[0] = src;
    return copy;
}
static rtx change_address(rtx memory, int mode, rtx address) {
    rtx n = node(MEM, mode);
    assert(memory->code == MEM);
    *n = *memory;
    n->mode = mode;
    n->exp[0] = address;
    /* This GCC's change_address preserves flags but starts a new alias set at0.
       Reproduce that API contract so this test catches losing the explicit copy. */
    n->alias_set = 0;
    return n;
}
static void emit_insn(rtx copy) {
    assert(emit_count++ == 0);
    emitted = copy;
}
static int expand(rtx *operands) {
''' + transformer.EXPANSION + r'''
    return 0;
}
static void reset(void) {
    used = force_count = emit_count = 0;
    emitted = 0;
    memset(forced, 0, sizeof(forced));
    memset(&hard_scratch, 0, sizeof(hard_scratch));
    hard_scratch.code = REG;
    hard_scratch.mode = TImode;
    hard_scratch.tag = 2;
    memset(&scratch_clobber, 0, sizeof(scratch_clobber));
    scratch_clobber.code = CLOBBER;
    scratch_clobber.exp[0] = &hard_scratch;
    flag_rac2_qcopy = 1;
    Pmode = SImode;
    reload_in_progress = reload_completed = 0;
}
static void same_attributes(rtx actual, rtx before) {
    assert(actual->code == MEM && actual->mode == TImode);
    assert(actual->alias_set == before->alias_set);
    assert(actual->is_volatile == before->is_volatile);
    assert(actual->in_struct == before->in_struct);
    assert(actual->scalar == before->scalar);
    assert(actual->readonly_p == before->readonly_p);
    assert(actual->unaligned == before->unaligned);
}
int main(void) {
    struct node dst, src, da, sa, before_dst, before_src;
    rtx operands[2], set;
    int variant;
    for (variant = 0; variant < 4; ++variant) {
        reset();
        memset(&dst, 0, sizeof(dst)); memset(&src, 0, sizeof(src));
        memset(&da, 0, sizeof(da)); memset(&sa, 0, sizeof(sa));
        da.code = sa.code = ADDR; da.tag = 101; sa.tag = 202;
        dst.code = src.code = MEM; dst.mode = src.mode = TImode;
        dst.exp[0] = &da; src.exp[0] = &sa;
        dst.alias_set = 17 + variant; src.alias_set = 47 + variant;
        dst.is_volatile = variant & 1; src.is_volatile = (variant >> 1) & 1;
        dst.scalar = 1; src.in_struct = 1; src.readonly_p = 1;
        before_dst = dst; before_src = src;
        operands[0] = &dst; operands[1] = &src;
        assert(expand(operands) == 1);
        assert(force_count == 2 && emit_count == 1);
        assert(forced[0] == &da && forced[1] == &sa);
        assert(emitted->code == PARALLEL);
        assert(emitted->vec[1] == &scratch_clobber);
        assert(scratch_clobber.exp[0] == &hard_scratch);
        set = emitted->vec[0]; assert(set->code == SET);
        same_attributes(set->exp[0], &before_dst);
        same_attributes(set->exp[1], &before_src);
        assert(set->exp[0]->exp[0]->exp[0] == &da);
        assert(set->exp[1]->exp[0]->exp[0] == &sa);
        assert(memcmp(&dst, &before_dst, sizeof(dst)) == 0);
        assert(memcmp(&src, &before_src, sizeof(src)) == 0);
    }
    for (variant = 0; variant < 8; ++variant) {
        reset();
        memset(&dst, 0, sizeof(dst)); memset(&src, 0, sizeof(src));
        dst.code = src.code = MEM; dst.mode = src.mode = TImode;
        dst.exp[0] = &da; src.exp[0] = &sa;
        operands[0] = &dst; operands[1] = &src;
        switch (variant) {
        case 0: flag_rac2_qcopy = 0; break;
        case 1: Pmode = DImode; break;
        case 2: reload_in_progress = 1; break;
        case 3: reload_completed = 1; break;
        case 4: dst.code = REG; break;
        case 5: src.code = REG; break;
        case 6: dst.unaligned = 1; break;
        case 7: src.unaligned = 1; break;
        }
        assert(expand(operands) == 0);
        assert(force_count == 0 && emit_count == 0 && emitted == 0);
    }
    return 0;
}
'''
        with tempfile.TemporaryDirectory() as directory:
            work = Path(directory)
            source_path, executable = work / "expansion.c", work / "expansion"
            source_path.write_text(source)
            subprocess.run([compiler, "-std=c89", "-Wall", "-Wextra", str(source_path), "-o", str(executable)],
                           check=True, capture_output=True, text=True)
            subprocess.run([str(executable)], check=True, capture_output=True, text=True)


if __name__ == "__main__":
    unittest.main()
