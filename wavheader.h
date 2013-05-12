/*  $Id: wavheader.h,v 1.1 2003/08/07 19:58:05 dsmith Exp dsmith $

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


#ifndef WAV_HEADER_H
#define WAV_HEADER_H


#define RIFF_CHUNK_ID 0x46464952
#define FMT_CHUNK_ID  0x20746d66
#define DATA_CHUNK_ID 0x61746164

struct riff_header {

  unsigned int ChunkID;
  unsigned int ChunkSize;
  unsigned int Format;

};

struct fmt_header {

  unsigned int Subchunk1ID;
  unsigned int Subchunk1Size;
  short    int AudioFormat;
  short    int NumChannels;
  unsigned int SampleRate;
  unsigned int ByteRate;
  short    int BlockAlign;
  short    int BitsPerSample;

};

struct data_header {

  unsigned int Subchunk2ID;
  unsigned int Subchunk2Size;

};

struct wav_file_headers {

  struct riff_header* riff;
  struct fmt_header*  fmt;
  struct data_header* data;

};

// Functions

struct riff_header* process_riff_header(void* chunk);
struct fmt_header* process_fmt_header(void* chunk);
struct data_header* process_data_header(void* chunk);
void print_riff_info(struct riff_header* h);
void print_format_info(struct fmt_header* h);
void print_data_info(struct data_header* h);
struct wav_file_headers* process_headers(int fd);
void write_headers(int fd, struct wav_file_headers* h);

unsigned int endian_flip(unsigned int word);

#endif
