/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2003                *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

  function:
  last mod: $Id: recon_mmx.c 11441 2006-05-27 17:09:08Z giles $

 ********************************************************************/

#include "codec_internal.h"

static const __attribute__ ((aligned(8),used)) ogg_int64_t V128 = 0x8080808080808080LL;

static void copy8x8__mmx (unsigned char *src,
                          unsigned char *dest,
                          ogg_uint32_t stride)
{
  __asm__ __volatile__ (
    "  .balign 16                      \n\t"

    "  lea         (%2, %2, 2), %%rdi  \n\t"

    "  movq        (%1), %%mm0         \n\t"
    "  movq        (%1, %2), %%mm1     \n\t"
    "  movq        (%1, %2, 2), %%mm2  \n\t"
    "  movq        (%1, %%rdi), %%mm3  \n\t"

    "  lea         (%1, %2, 4), %1     \n\t" 

    "  movq        %%mm0, (%0)         \n\t"
    "  movq        %%mm1, (%0, %2)     \n\t"
    "  movq        %%mm2, (%0, %2, 2)  \n\t"
    "  movq        %%mm3, (%0, %%rdi)  \n\t"

    "  lea         (%0, %2, 4), %0     \n\t" 

    "  movq        (%1), %%mm0         \n\t"
    "  movq        (%1, %2), %%mm1     \n\t"
    "  movq        (%1, %2, 2), %%mm2  \n\t"
    "  movq        (%1, %%rdi), %%mm3  \n\t"

    "  movq        %%mm0, (%0)         \n\t"
    "  movq        %%mm1, (%0, %2)     \n\t"
    "  movq        %%mm2, (%0, %2, 2)  \n\t"
    "  movq        %%mm3, (%0, %%rdi)  \n\t"
      : "+a" (dest)
      : "c" (src),
        "d" ((ogg_uint64_t)stride)
      : "memory", "rdi"
  );
}

static void recon_intra8x8__mmx (unsigned char *ReconPtr, ogg_int16_t *ChangePtr,
                                 ogg_uint32_t LineStep)
{
  __asm__ __volatile__ (
    "  .balign 16                      \n\t"

    "  movq        %[V128], %%mm0      \n\t" /* Set mm0 to 0x8080808080808080 */

    "  lea         128(%1), %%rdi      \n\t" /* Endpoint in input buffer */
    "1:                                \n\t" 
    "  movq         (%1), %%mm2        \n\t" /* First four input values */

    "  packsswb    8(%1), %%mm2        \n\t" /* pack with next(high) four values */
    "  por         %%mm0, %%mm0        \n\t" 
    "  pxor        %%mm0, %%mm2        \n\t" /* Convert result to unsigned (same as add 128) */
    "  lea         16(%1), %1          \n\t" /* Step source buffer */
    "  cmp         %%rdi, %1           \n\t" /* are we done */

    "  movq        %%mm2, (%0)         \n\t" /* store results */

    "  lea         (%0, %2), %0        \n\t" /* Step output buffer */
    "  jc          1b                  \n\t" /* Loop back if we are not done */
      : "+r" (ReconPtr)
      : "r" (ChangePtr),
        "r" ((ogg_uint64_t)LineStep),
        [V128] "m" (V128)
      : "memory", "rdi"
  );
}

static void recon_inter8x8__mmx (unsigned char *ReconPtr, unsigned char *RefPtr,
                                 ogg_int16_t *ChangePtr, ogg_uint32_t LineStep)
{
  __asm__ __volatile__ (
    "  .balign 16                      \n\t"

    "  pxor        %%mm0, %%mm0        \n\t"
    "  lea         128(%1), %%rdi      \n\t"

    "1:                                \n\t"
    "  movq        (%2), %%mm2         \n\t" /* (+3 misaligned) 8 reference pixels */

    "  movq        (%1), %%mm4         \n\t" /* first 4 changes */
    "  movq        %%mm2, %%mm3        \n\t"
    "  movq        8(%1), %%mm5        \n\t" /* last 4 changes */
    "  punpcklbw   %%mm0, %%mm2        \n\t" /* turn first 4 refs into positive 16-bit #s */
    "  paddsw      %%mm4, %%mm2        \n\t" /* add in first 4 changes */
    "  punpckhbw   %%mm0, %%mm3        \n\t" /* turn last 4 refs into positive 16-bit #s */
    "  paddsw      %%mm5, %%mm3        \n\t" /* add in last 4 changes */
    "  add         %3, %2              \n\t" /* next row of reference pixels */
    "  packuswb    %%mm3, %%mm2        \n\t" /* pack result to unsigned 8-bit values */
    "  lea         16(%1), %1          \n\t" /* next row of changes */
    "  cmp         %%rdi, %1           \n\t" /* are we done? */

    "  movq        %%mm2, (%0)         \n\t" /* store result */

    "  lea         (%0, %3), %0        \n\t" /* next row of output */
    "  jc          1b                  \n\t"
      : "+r" (ReconPtr)
      : "r" (ChangePtr),
        "r" (RefPtr),
        "r" ((ogg_uint64_t)LineStep)
      : "memory", "rdi"
  );
}

static void recon_inter8x8_half__mmx (unsigned char *ReconPtr, unsigned char *RefPtr1,
                                      unsigned char *RefPtr2, ogg_int16_t *ChangePtr,
                                      ogg_uint32_t LineStep)
{
  __asm__ __volatile__ (
    "  .balign 16                      \n\t"

    "  pxor        %%mm0, %%mm0        \n\t"
    "  lea         128(%1), %%rdi      \n\t"

    "1:                                \n\t"
    "  movq        (%2), %%mm2         \n\t" /* (+3 misaligned) 8 reference pixels */
    "  movq        (%3), %%mm4         \n\t" /* (+3 misaligned) 8 reference pixels */

    "  movq        %%mm2, %%mm3        \n\t"
    "  punpcklbw   %%mm0, %%mm2        \n\t" /* mm2 = start ref1 as positive 16-bit #s */
    "  movq        %%mm4, %%mm5        \n\t"
    "  movq        (%1), %%mm6         \n\t" /* first 4 changes */
    "  punpckhbw   %%mm0, %%mm3        \n\t" /* mm3 = end ref1 as positive 16-bit #s */
    "  movq        8(%1), %%mm7        \n\t" /* last 4 changes */
    "  punpcklbw   %%mm0, %%mm4        \n\t" /* mm4 = start ref2 as positive 16-bit #s */
    "  punpckhbw   %%mm0, %%mm5        \n\t" /* mm5 = end ref2 as positive 16-bit #s */
    "  paddw       %%mm4, %%mm2        \n\t" /* mm2 = start (ref1 + ref2) */
    "  paddw       %%mm5, %%mm3        \n\t" /* mm3 = end (ref1 + ref2) */
    "  psrlw       $1, %%mm2           \n\t" /* mm2 = start (ref1 + ref2)/2 */
    "  psrlw       $1, %%mm3           \n\t" /* mm3 = end (ref1 + ref2)/2 */
    "  paddw       %%mm6, %%mm2        \n\t" /* add changes to start */
    "  paddw       %%mm7, %%mm3        \n\t" /* add changes to end */
    "  lea         16(%1), %1          \n\t" /* next row of changes */
    "  packuswb    %%mm3, %%mm2        \n\t" /* pack start|end to unsigned 8-bit */
    "  add         %4, %2              \n\t" /* next row of reference pixels */
    "  add         %4, %3              \n\t" /* next row of reference pixels */
    "  movq        %%mm2, (%0)         \n\t" /* store result */
    "  add         %4, %0              \n\t" /* next row of output */
    "  cmp         %%rdi, %1           \n\t" /* are we done? */
    "  jc          1b                  \n\t"
      : "+r" (ReconPtr)
      : "r" (ChangePtr),
        "r" (RefPtr1),
        "r" (RefPtr2),
        "r" ((ogg_uint64_t)LineStep)
      : "memory", "rdi"
  );
}

void dsp_mmx_recon_init(DspFunctions *funcs)
{
  TH_DEBUG("enabling accelerated x86_64 mmx recon functions.\n");
  funcs->copy8x8 = copy8x8__mmx;
  funcs->recon_intra8x8 = recon_intra8x8__mmx;
  funcs->recon_inter8x8 = recon_inter8x8__mmx;
  funcs->recon_inter8x8_half = recon_inter8x8_half__mmx;
}

