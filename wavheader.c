/*  $Id: wavheader.c,v 1.3 2003/08/08 20:09:38 dsmith Exp $

  wavsilence - A program to split a WAV file into smaller pieces, based on
               detected silence

  Copyright 2003 Daniel Smith (dsmith@danplanet.com)
  
  The most recent revision of this program may be found at:
      http://danplanet.com/wav/


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
#include <stdlib.h>


#include "wavheader.h"

unsigned int endian_flip(unsigned int word) {

  char hbyte, lbyte, hhbyte, llbyte;
  short result;

  llbyte = word & 0xFF000000;
  lbyte  = word & 0x00FF0000;
  hbyte  = word & 0x0000FF00;
  hhbyte = word & 0x000000FF;
  
  result = 0;

  result = (hhbyte << 24) & result;
  result = (hbyte << 8) & result;
  result = (lbyte >> 8) & result;
  result = (llbyte >>24) & result;

  printf("BEFORE: %x  AFTER: %x\n", word, result);

}

struct riff_header* process_riff_header(void* chunk) {

  struct riff_header* h;
  h = malloc(sizeof(struct riff_header));

  memcpy(h, chunk, sizeof(struct riff_header));

  if(h->ChunkID == RIFF_CHUNK_ID)
    return h;
  else
    return NULL;

}

struct fmt_header* process_fmt_header(void* chunk) {

  struct fmt_header* h;
  h = malloc(sizeof(struct fmt_header));

  memcpy(h, chunk, sizeof(struct fmt_header));

  if(h->Subchunk1ID == FMT_CHUNK_ID)
    return h;
  else
    return NULL;

}

struct data_header* process_data_header(void* chunk) {

  struct data_header* h;
  h = malloc(sizeof(struct data_header));

  memcpy(h, chunk, sizeof(struct data_header));

  if(h->Subchunk2ID == DATA_CHUNK_ID)
    return h;
  else
    return NULL;

}

void print_riff_info(struct riff_header* h) {

  char id_string[5];
  char fmt_string[5];

  memcpy(id_string, &h->ChunkID, 4);
  id_string[4] = '\0';

  memcpy(fmt_string, &h->Format, 4);
  fmt_string[4] = '\0';

  printf("-- Riff Header --\n");
  printf("  Chunk ID: 0x%x (%s)\n", h->ChunkID, id_string);
  printf("  Size: %.2f KB\n", h->ChunkSize / 1024.0);
  printf("  Format: 0x%x (%s)\n", h->Format, fmt_string);

}

void print_format_info(struct fmt_header* h) {

  char id_string[5];

  memcpy(id_string, &h->Subchunk1ID, 4);
  id_string[4] = '\0';

  printf("-- Format Header --\n");
  printf("  Chunk ID: 0x%x (%s)\n", h->Subchunk1ID, id_string);
  printf("  Size: %i bytes\n", h->Subchunk1Size);
  printf("  Audio Format: %i (1=PCM)\n", h->AudioFormat);
  printf("  Num Channels: %i (2=Stereo)\n", h->NumChannels);
  printf("  Sampling Rate: %i Hz\n", h->SampleRate);
  printf("  ByteRate: %i Bps\n", h->ByteRate);
  printf("  BlockAlign: %i\n", h->BlockAlign);
  printf("  Bits/Sample: %i\n", h->BitsPerSample);

}

void print_data_info(struct data_header* h) {

  char id_string[5];
  memcpy(id_string, &h->Subchunk2ID, 4);
  id_string[4] = '\0';

  printf("-- Data Header --\n");
  printf("  Chunk ID: 0x%x (%s)\n", h->Subchunk2ID, id_string);
  printf("  Size: %.2f KB\n", h->Subchunk2Size / 1024.0);

}


struct wav_file_headers* process_headers(int fd) {

  struct wav_file_headers* h;
  void *riff_h, *fmt_h, *data_h;
  
  riff_h = malloc(sizeof(struct riff_header));
  fmt_h  = malloc(sizeof(struct fmt_header));
  data_h = malloc(sizeof(struct data_header));
  h = malloc(sizeof(struct wav_file_headers));
  
  read(fd, riff_h, sizeof(struct riff_header));
  read(fd, fmt_h,  sizeof(struct fmt_header));
  read(fd, data_h, sizeof(struct data_header));

  h->riff = process_riff_header(riff_h);
  h->fmt  = process_fmt_header(fmt_h);
  h->data = process_data_header(data_h);

  free(riff_h);
  free(fmt_h);
  free(data_h);

  return h;

}

void write_headers(int fd, struct wav_file_headers* h) {

  // RIFF Header
  write(fd, h->riff, sizeof(struct riff_header));
  
  // FMT Header
  write(fd, h->fmt, sizeof(struct fmt_header));
  
  // DATA Header
  write(fd, h->data, sizeof(struct data_header));

}

void fp_write_headers(FILE* fp, struct wav_file_headers* h) {

  // RIFF Header
  fwrite(h->riff, sizeof(struct riff_header), 1, fp);
  
  // FMT Header
  fwrite(h->fmt, sizeof(struct fmt_header), 1, fp);
  
  // DATA Header
  fwrite(h->data, sizeof(struct data_header), 1, fp);


}
