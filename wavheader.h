/*  $Id: wavheader.h,v 1.1 2003/08/07 19:58:05 dsmith Exp dsmith $

  wavsilence - A program to split a WAV file into smaller pieces, based on
               detected silence

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


#ifndef WAV_HEADER_H
#define WAV_HEADER_H

#define WAVSILENCE_VERSION "wavsilence 0.45 " __DATE__ " " __TIME__

#define RIFF_CHUNK_ID 0x46464952 // "RIFF"
#define FMT_CHUNK_ID  0x20746d66 // "fmt "
#define DATA_CHUNK_ID 0x61746164 // "DATA"

#pragma pack(push, 1)

struct chunk_header {
   unsigned int id;

   // Size of the chunk's content in bytes, excluding the potential padding byte
   unsigned int size;
};

struct riff_header {

   struct chunk_header header;
  unsigned int Format;

};

struct fmt_header {

   struct chunk_header header;

  short    int AudioFormat;
  short    int NumChannels;
  unsigned int SampleRate;
  unsigned int ByteRate;
  short    int BlockAlign;
  short    int BitsPerSample;

};

#pragma pack(pop)

struct wav_file_headers {

  struct riff_header  riff;
  struct fmt_header   fmt;
  struct chunk_header data;

};

// Functions

void print_riff_info(struct riff_header* h);
void print_format_info(struct fmt_header* h);
void print_data_info(struct chunk_header* h);

int process_headers(int fd, struct wav_file_headers *h);

int write_headers(int fd, struct wav_file_headers* h);
int fp_write_headers(FILE* fp, struct wav_file_headers* h);

#endif
