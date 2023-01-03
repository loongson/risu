/******************************************************************************
 * Copyright (c) 2022 Loongson Technology Corporation Limited
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     based on Peter Maydell's risu_reginfo_arm.c
 *****************************************************************************/

#include <stdio.h>
#include <asm/types.h>
#include <signal.h>
#include <asm/ucontext.h>

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>
#include <assert.h>
#include <sys/prctl.h>

#include "risu.h"
#include "risu_reginfo_loongarch64.h"

const struct option * const arch_long_opts;
const char * const arch_extra_help;

void process_arch_opt(int opt, const char *arg)
{
    abort();
}

void arch_init(void)
{
}

int reginfo_size(struct reginfo *ri)
{
    return sizeof(*ri);
}

/* reginfo_init: initialize with a ucontext */
void reginfo_init(struct reginfo *ri, ucontext_t *context)
{
    int i, j, k;
    struct ucontext *uc = (struct ucontext *)context;

    memset(ri, 0, sizeof(*ri));

    for (i = 1; i < 32; i++) {
        ri->regs[i] = uc->uc_mcontext.sc_regs[i]; //sp:r3, tp:r2
    }

    ri->regs[2] = 0xdeadbeefdeadbeef;
    ri->pc = (unsigned long)uc->uc_mcontext.sc_pc - (unsigned long)image_start_address;
    ri->flags = uc->uc_mcontext.sc_flags;
    ri->faulting_insn = *((uint32_t *) uc->uc_mcontext.sc_pc);
    ri->fcc = uc->uc_mcontext.sc_fcc;
    ri->fcsr = uc->uc_mcontext.sc_fcsr;

    for (j = 1; j < 32; j++) {
        /* Save LSX registers */
        for (k = 0; k < 2; k++) {
            ri->fpregs[j].val64[k] = uc->uc_mcontext.sc_fpregs[j].val64[k];
        }
    }
}

/* reginfo_is_eq: compare the reginfo structs, returns nonzero if equal */
int reginfo_is_eq(struct reginfo *r1, struct reginfo *r2)
{
    return !memcmp(r1, r2, sizeof(*r1));
}

/* reginfo_dump: print state to a stream, returns nonzero on success */
int reginfo_dump(struct reginfo *ri, FILE * f)
{
    int i;
    fprintf(f, "  faulting insn %08x\n", ri->faulting_insn);

    for (i = 0; i < 32; i++) {
        fprintf(f, "  r%-2d    : %016" PRIx64 "\n", i, ri->regs[i]);
    }

    fprintf(f, "  pc     : %016" PRIx64 "\n", ri->pc);
    fprintf(f, "  flags  : %08x\n", ri->flags);
    fprintf(f, "  fcc    : %016" PRIx64 "\n", ri->fcc);
    fprintf(f, "  fcsr   : %08x\n", ri->fcsr);

    for (i = 0; i < 32; i++) {
        fprintf(f, "  f%-2d    : %016lx\n", i, ri->fpregs[i].val64[0]);
    }

    return !ferror(f);
}

/* reginfo_dump_mismatch: print mismatch details to a stream, ret nonzero=ok */
int reginfo_dump_mismatch(struct reginfo *m, struct reginfo *a, FILE * f)
{
    int i;
    fprintf(f, "mismatch detail (master : apprentice):\n");
    if (m->faulting_insn != a->faulting_insn) {
        fprintf(f, "  faulting insn mismatch %08x vs %08x\n",
                m->faulting_insn, a->faulting_insn);
    }
    /* r2:tp, r3:sp */
    for (i = 0; i < 32; i++) {
        if (m->regs[i] != a->regs[i]) {
            fprintf(f, "  r%-2d    : %016" PRIx64 " vs %016" PRIx64 "\n",
                    i, m->regs[i], a->regs[i]);
        }
    }

    if (m->pc != a->pc) {
        fprintf(f, "  pc     : %016" PRIx64 " vs %016" PRIx64 "\n",
                m->pc, a->pc);
    }
    if (m->flags != a->flags) {
        fprintf(f, "  flags  : %08x vs %08x\n", m->flags, a->flags);
    }
    if (m->fcc != a->fcc) {
        fprintf(f, "  fcc    : %016" PRIx64 " vs %016" PRIx64 "\n",
                m->fcc, a->fcc);
    }
    if (m->fcsr != a->fcsr) {
        fprintf(f, "  fcsr   : %08x vs %08x\n", m->fcsr, a->fcsr);
    }

    for (i = 0; i < 32; i++) {
        if (m->fpregs[i].val64[0]!= a->fpregs[i].val64[0]) {
            fprintf(f, "  f%-2d    : %016lx vs %016lx\n",
                    i, m->fpregs[i].val64[0], a->fpregs[i].val64[0]);
        }
    }

    return !ferror(f);
}
