/*  $Id: wavinfo.c,v 1.2 2003/08/08 20:24:42 dsmith Exp dsmith $

  wavinfo: Display some information about a WAV file from the headers

  Note: Currently only reads "test.wav" !!

  Copyright 2003 Daniel Smith (dsmith@danplanet.com)
  
  The most recent revision of this program may be found at:
      http://danplanet.com/wav/  (404)

   New project location:
      https://github.com/DOSx86/wavsilence

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/



#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "wavheader.h"

int fd;
int length;
int verify;
int info;
int quiet;

#define BLOCK_SIZE 128

int measure_size(struct wav_file_headers* h) {

  // Read the rest of the file, counting bytes

  int count = 0;
  int total = 0;
  char buffer[BLOCK_SIZE];
  int percent;

  do {
    count = read(fd, buffer, BLOCK_SIZE);
    total += count;
    if(!quiet)
      printf("Processing file: %.0f%% (%i KB)\r", 
	     (total / (double)h->data.size) * 100,
	     total / 1024);
  } while(count == BLOCK_SIZE);

  printf("                                           \r");

  return total;

}

double calc_length(struct wav_file_headers* h) {

  int data_size;
  int num_samples;

  data_size = h->data.size;
  num_samples = data_size / ((h->fmt.NumChannels) * (h->fmt.BitsPerSample / 8));
 
  return (double)num_samples / h->fmt.SampleRate;
  

}

void print_usage() {

  printf("usage: wavinfo <options> file\n");
  printf("Options:\n");
  printf("  -v     Verify File\n");
  printf("  -q     Do not show verification progress\n");
  printf("  -i     Show file info\n");
  printf("  -l     Display file length (in seconds)\n");
  printf("  -h     Show this message\n");

  printf("\n");
}

void process_args(int argc, char**argv) {

  int c;

  while((c = getopt(argc, argv, "Vhlviq")) != -1) {
    switch(c) {
    case 'h':
      print_usage();
      exit(0);
      break;
    case 'l':
      length = 1;
      break;
    case 'v':
      verify = 1;
      break;
    case 'i':
      info = 1;
      break;
    case 'V':
      printf(WAVSILENCE_VERSION "\n");
      exit(0);
    case 'q':
      quiet = 1;
      break;

    default:
      exit(1);
    }
    
  }

}


int main(int argc, char**argv) {

  struct wav_file_headers h;
  int c;
  int size;

  length = info = verify = quiet = 0;

  if (argc < 2) {
     print_usage();
     return 1;
  }

  process_args(argc, argv);

  fd = open(argv[argc-1], O_RDONLY);

  if (fd==-1) {
     fprintf(stderr, "Could not open the file \"%s\"\n", argv[argc - 1]);
     return 1;
  }

  if (!process_headers(fd, &h)) {
     close(fd);
     return 1;
  }

  if(info) {
    print_riff_info(&h.riff);
    print_format_info(&h.fmt);
    print_data_info(&h.data);
    printf("\nTotal Length: %.2f seconds\n", calc_length(&h));
  }
  if(length) {
    printf("%.2f\n", calc_length(&h));
  }

  if(verify) {
    size = measure_size(&h);
    if(size < h.data.size)
      printf("File is truncated (%i / %i bytes)\n", size, h.data.size);
    else if (size > h.data.size)
      printf("File is too long (%i / %i bytes)\n", size, h.data.size);
    else
      printf("File is OK (%i bytes)\n", size);
  }

  if((length == 0) && (info == 0) && (verify == 0)) {
    print_usage();
    return 1;
  }

  return 0;
}
