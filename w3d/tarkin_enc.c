#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "tarkin.h"
#include "ppm.h"


static
void usage (const char *program_name)
{
   printf ("\n"
     " usage: %s <input filename format string> <bitrate> <a_m> <s_m>\n"
     "\n"
     "   input ppm filename format:  optional, \"%%i.ppm\" by default\n"
     "   bitrate:                    cut Y/U/V bitstream after limit bits (!!!)/frame\n"
     "                               (something like 1228800 makes sense here)\n" 
     "   a_m, s_m:                   number of vanishing moments of the\n"
     "                               analysis/synthesis filter, (2,2) by default\n"
     "\n", program_name);
   exit (-1);
}



int main (int argc, char **argv)
{
   char *fmt = "%i.ppm";
   char fname[256];
   uint32_t frame = 0;
   uint8_t *rgb;
   int a_moments, s_moments;
   int fd;
   TarkinStream *tarkin_stream;
   TarkinVideoLayerDesc layer [] = { { 0, 0, 1, 5000, TARKIN_RGB24 } };

   if (argc == 1) {
      layer[0].bitrate = 1000;
      a_moments = 2;
      s_moments = 2;
   } else if (argc == 5) {
      fmt = argv[1];
      layer[0].bitrate = strtol (argv[2], 0, 0);
      a_moments = strtol (argv[3], 0, 0);
      s_moments = strtol (argv[4], 0, 0);
   } else {
      usage (argv[0]);
   }

   snprintf (fname, 256, fmt, 0);
   if (read_ppm_info (fname, &layer[0].width, &layer[0].height) < 0)
      exit (-1);

   rgb  = malloc (layer[0].width * layer[0].height * 3);

   if ((fd = open ("stream.tarkin", O_CREAT | O_RDWR | O_TRUNC, 0644)) < 0) {
      printf ("error opening '%s' for writing !\n", "stream.tarkin");
      usage (argv[0]);
   }

   tarkin_stream = tarkin_stream_new (fd);
   tarkin_stream_write_layer_descs (tarkin_stream, 1, layer);

   do {
      snprintf (fname, 256, fmt, frame);
      printf ("read '");
      printf (fname, frame);
      printf ("'");

      if (read_ppm (fname, rgb, layer[0].width, layer[0].height) < 0)
      {
         printf (" failed.\n");
         break;
      }
      printf ("\n");

      tarkin_stream_write_frame (tarkin_stream, &rgb);
      frame++;
   } while (1);

   free (rgb);
   tarkin_stream_destroy (tarkin_stream);
   close (fd);

   return 0;
}

