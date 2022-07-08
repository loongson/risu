/*******************************************************************************
 * Copyright (c) 2010 Linaro Limited
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Peter Maydell (Linaro) - initial implementation
 ******************************************************************************/

#ifndef RISU_H
#define RISU_H

#include <inttypes.h>
#include <stdint.h>
#include <ucontext.h>
#include <stdio.h>
#include <getopt.h>
#include <stdbool.h>

/* Extra option processing for architectures */
extern const struct option * const arch_long_opts;
extern const char * const arch_extra_help;
void process_arch_opt(int opt, const char *arg);
void arch_init(void);
#define FIRST_ARCH_OPT   0x100

/* GCC computed include to pull in the correct risu_reginfo_*.h for
 * the architecture.
 */
#define REGINFO_HEADER2(X) #X
#define REGINFO_HEADER1(ARCHNAME) REGINFO_HEADER2(risu_reginfo_ ## ARCHNAME.h)
#define REGINFO_HEADER(ARCH) REGINFO_HEADER1(ARCH)

#include REGINFO_HEADER(ARCH)

extern uintptr_t image_start_address;

/* Ops code under test can request from risu: */
typedef enum {
    /* Any other sigill besides the destignated undefined insn.  */
    OP_SIGILL = -1,

    /* These are generated by the designated undefined insn. */
    OP_COMPARE = 0,
    OP_TESTEND = 1,
    OP_SETMEMBLOCK = 2,
    OP_GETMEMBLOCK = 3,
    OP_COMPAREMEM = 4,
} RisuOp;

/* Result of operation */
typedef enum {
    RES_OK = 0,
    RES_END,
    RES_MISMATCH_REG,
    RES_MISMATCH_MEM,
    RES_MISMATCH_OP,
    RES_BAD_IO,
    RES_BAD_MAGIC,
    RES_BAD_SIZE,
    RES_BAD_OP,
} RisuResult;

/* The memory block should be this long */
#define MEMBLOCKLEN 8192

/* This is the data structure we pass over the socket for OP_COMPARE
 * and OP_TESTEND. It is a simplified and reduced subset of what can
 * be obtained with a ucontext_t*, and is architecture specific
 * (defined in risu_reginfo_*.h).
 */
struct reginfo;

typedef struct {
   uint32_t magic;
   uint32_t size;
   uint32_t risu_op;
   uintptr_t pc;
} trace_header_t;

#define RISU_MAGIC  (('R' << 24) | ('I' << 16) | ('S' << 8) | 'U')

/* Socket related routines */
int master_connect(int port);
int apprentice_connect(const char *hostname, int port);
RisuResult send_data_pkt(int sock, void *pkt, int pktlen);
RisuResult recv_data_pkt(int sock, void *pkt, int pktlen);
void send_response_byte(int sock, int resp);

/* Functions operating on reginfo */

/* Interface provided by CPU-specific code: */

/* Move the PC past this faulting insn by adjusting ucontext. */
void advance_pc(void *uc);

/* Set the parameter register in a ucontext_t to the specified value.
 * (32-bit targets can ignore high 32 bits.)
 * vuc is a ucontext_t* cast to void*.
 */
void set_ucontext_paramreg(void *vuc, uint64_t value);

/* Return the value of the parameter register from a reginfo. */
uint64_t get_reginfo_paramreg(struct reginfo *ri);

/*
 * Return the risu operation number we have been asked to do,
 * or OP_SIGILL if this was a SIGILL for a non-risuop insn.
 */
RisuOp get_risuop(struct reginfo *ri);

/* Return the PC from a reginfo */
uintptr_t get_pc(struct reginfo *ri);

/* initialize structure from a ucontext */
void reginfo_init(struct reginfo *ri, ucontext_t *uc);

/* return 1 if structs are equal, 0 otherwise. */
int reginfo_is_eq(struct reginfo *r1, struct reginfo *r2);

/* print reginfo state to a stream, returns 1 on success, 0 on failure */
int reginfo_dump(struct reginfo *ri, FILE * f);

/* reginfo_dump_mismatch: print mismatch details to a stream, ret nonzero=ok */
int reginfo_dump_mismatch(struct reginfo *m, struct reginfo *a, FILE *f);

/* return size of reginfo */
int reginfo_size(struct reginfo *ri);

#endif /* RISU_H */
