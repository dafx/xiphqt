#ifndef __BITCODER_H
#define __BITCODER_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>


#if defined(BITCODER)

#define OUTPUT_BIT(coder,bit)             bitcoder_write_bit(coder,bit)
#define INPUT_BIT(coder)                  bitcoder_read_bit(coder)
#define OUTPUT_BIT_DIRECT(coder,bit)      bitcoder_write_bit(coder,bit)
#define INPUT_BIT_DIRECT(coder)           bitcoder_read_bit(coder)
#define ENTROPY_CODER                     BitCoderState
#define ENTROPY_ENCODER_INIT(coder,limit) bitcoder_encoder_init(coder,limit)
#define ENTROPY_ENCODER_DONE(coder)       bitcoder_encoder_done(coder)
#define ENTROPY_ENCODER_FLUSH(coder)      bitcoder_flush(coder)
#define ENTROPY_DECODER_INIT(coder,bitstream,limit) \
   bitcoder_decoder_init(coder,bitstream,limit)
#define ENTROPY_DECODER_DONE(coder)       /* nothing to do ... */
#define ENTROPY_CODER_IS_EMPTY(coder)     bitcoder_is_empty (coder)
#define ENTROPY_CODER_BITSTREAM(coder)    (coder)->bitstream

#endif


typedef struct {
   uint32_t  bit_count;          /*  number of valid bits in byte    */
   uint8_t   byte;               /*  buffer to save bits             */
   uint32_t  byte_count;         /*  number of bytes written         */
   uint8_t  *bitstream;
   uint32_t  limit;              /*  don't write more bytes to bitstream ... */
} BitCoderState;



static inline
void bitcoder_encoder_init (BitCoderState *s, uint32_t limit)
{
   s->bit_count = 0;
   s->byte = 0;
   s->byte_count = 0;
   s->bitstream = (uint8_t*) malloc (limit);
   s->limit = limit;
}


static inline
void bitcoder_encoder_done (BitCoderState *s)
{
//  XXX FIXME !!!
//   free (s->bitstream);
}


static inline
void bitcoder_decoder_init (BitCoderState *s, uint8_t *bitstream, uint32_t limit)
{
   s->bit_count = 0;
   s->byte = 0;
   s->byte_count = 0;
   s->bitstream = bitstream;
   s->limit = limit;
}


static inline
uint32_t bitcoder_flush (BitCoderState *s)
{
   if (s->bit_count > 0 && s->byte_count < s->limit)
      s->bitstream [s->byte_count++] = s->byte << (8 - s->bit_count);

//printf ("%s: %i bytes written.\n", __FUNCTION__, s->byte_count);
   return s->byte_count;
}


static inline
int bitcoder_is_empty (BitCoderState *s)
{
   if (!s->bitstream || s->byte_count >= s->limit)
      return 1;

   return 0;
}


static inline
void bitcoder_write_bit (BitCoderState *s, int bit)
{
   s->byte <<= 1;
   s->byte |= bit & 1;

   s->bit_count++;

   if (s->bit_count == 8 && s->byte_count < s->limit) {
      s->bitstream [s->byte_count++] = s->byte;
      s->bit_count = 0;
   }
}


static inline
int bitcoder_read_bit (BitCoderState *s)
{
   int ret;

   if (s->bit_count == 0 && s->byte_count < s->limit) {
      if (!s->bitstream)
         return 0;

      s->byte = s->bitstream [s->byte_count++];
      s->bit_count = 8;
   }

   ret = s->byte >> 7;
   s->byte <<= 1; 
   s->bit_count--;

   return ret;
}






static inline
void bit_print (TYPE byte)
{
   int bit = 8*sizeof(TYPE);

   do {
      bit--;
      printf ((byte & (1 << bit)) ? "1" : "0");
   } while (bit);
   printf ("\n");
}

#endif

