/* Copyright (C) 2002 Jean-Marc Valin 
   File: speexdec.c

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   
   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include "speex.h"
#include "ogg/ogg.h"

#define MAX_FRAME_SIZE 2000
#define MAX_FRAME_BYTES 1000

void usage()
{
   fprintf (stderr, "speexenc [options] <input file> <output file>\n");
   fprintf (stderr, "options:\n");
   fprintf (stderr, "\t--help       -h    This help\n"); 
   fprintf (stderr, "\t--version    -v    Version information\n"); 
}

void version()
{
   fprintf (stderr, "Speex encoder version " VERSION "\n");
}

int main(int argc, char **argv)
{
   int c;
   int option_index = 0;
   char *inFile, *outFile;
   FILE *fin, *fout;
   short out[MAX_FRAME_SIZE];
   float output[MAX_FRAME_SIZE];
   int frame_size=0;
   SpeexMode *mode=NULL;
   void *st=NULL;
   FrameBits bits;
   int first = 1;
   struct option long_options[] =
   {
      {"help", no_argument, NULL, 0},
      {"version", no_argument, NULL, 0},
      {0, 0, 0, 0}
   };
   ogg_sync_state oy;
   ogg_page       og;
   ogg_packet     op;
   ogg_stream_state os;

   while(1)
   {
      c = getopt_long (argc, argv, "hv",
                       long_options, &option_index);
      if (c==-1)
         break;
      
      switch(c)
      {
      case 0:
         if (strcmp(long_options[option_index].name,"help")==0)
         {
            usage();
            exit(0);
         } else if (strcmp(long_options[option_index].name,"version")==0)
         {
            version();
            exit(0);
         }
         break;
      case 'h':
         usage();
         break;
      case 'v':
         version();
         exit(0);
         break;
      case '?':
         usage();
         exit(1);
         break;
      }
   }
   if (argc-optind!=2)
   {
      usage();
      exit(1);
   }
   inFile=argv[optind];
   outFile=argv[optind+1];

   if (strcmp(inFile, "-")==0)
      fin=stdin;
   else 
   {
      fin = fopen(inFile, "r");
      if (!fin)
      {
         perror(inFile);
         exit(1);
      }
   }
   if (strcmp(outFile,"-")==0)
      fout=stdout;
   else 
   {
      fout = fopen(outFile, "w");
      if (!fout)
      {
         perror(outFile);
         exit(1);
      }
   }

   ogg_sync_init(&oy);
   ogg_stream_init(&os, 0);
   
   speex_bits_init(&bits);
   while (1/*||feof(fin)*/)
   {
      char *data;
      int i, nb_read;
      data = ogg_sync_buffer(&oy, 200);
      nb_read = fread(data, sizeof(char), 200, fin);      
      ogg_sync_wrote(&oy, nb_read);
      while (ogg_sync_pageout(&oy, &og)==1)
      {
         ogg_stream_pagein(&os, &og);
         while (ogg_stream_packetout(&os, &op)==1)
         {
            if (first)
            {
               if (strncmp((char *)op.packet, "speex wideband**", 12)==0)
               {
                  fprintf (stderr, "Wideband!!\n");
                  mode = &speex_wb_mode;
               } else if (strncmp((char *)op.packet, "speex narrowband", 12)==0)
               {
                  fprintf (stderr, "Narrowband!!\n");
                  mode = &speex_nb_mode;
               } else {
                  fprintf (stderr, "This Ogg file is not a Speex bitstream\n");
                  exit(1);
               }
               st = decoder_init(mode);
               frame_size=mode->frameSize;
               first=0;
            } else {
               if (strncmp((char *)op.packet, "END OF STREAM", 13)==0)
                  break;
               speex_bits_init_from(&bits, (char*)op.packet, op.bytes);
               decode(st, &bits, output);
               
               for (i=0;i<frame_size;i++)
               {
                  if (output[i]>32000)
                     output[i]=32000;
                  else if (output[i]<-32000)
                     output[i]=-32000;
               }
               for (i=0;i<frame_size;i++)
                  out[i]=output[i];
               fwrite(out, sizeof(short), frame_size, fout);
            }
         }
      }
      if (feof(fin))
         break;

   }

   decoder_destroy(st);

   exit(0);
   return 1;
}