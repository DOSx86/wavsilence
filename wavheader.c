/*  $Id: wavheader.c,v 1.3 2003/08/08 20:09:38 dsmith Exp $

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


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "wavheader.h"

static void skip_chunk_error(const char *which, unsigned int target_id) {
   fprintf(stderr,
           "%s while searching for chunk ID 0x%08X\n",
           which,
           target_id);
}

// Attempts to read the next chunk, verifying that the ID matches and that
// the data in the chunk can be read completely.
//
// If the ID does not match, header is NULL, or if data_size is greater than the
// number of bytes in the chunk, this function will fail and return 0.
//
// chunk_data is optional. If chunk_data is NULL or data_size is 0, then only
// the header is read.
//
// If skip_data is 0 and chunk data is being read (chunk_data and data_size are
// both non-zero), then the read position does not skip the rest of the chunk
// data, if any is remaining. Note: If the data is not skipped, then the padding
// byte at the the end of the chunk data must be taken into account.
//
// If skip_data is 1 and chunk data is being read, then data_size bytes are read
// into chunk_data, then the read position skips to the beginning of the next
// chunk header.
//
// Returns a non-zero value on success
static int read_chunk(int fd, unsigned int id, struct chunk_header *header, void *chunk_data, size_t data_size, int skip_data) {
   if (!header) {
      fprintf(stderr, "read_chunk: NULL header for chunk ID 0x%08X\n", id);
      return 0;
   }

   if (!chunk_data || !data_size) {
      chunk_data  = NULL;
      data_size   = 0;
   }

   if (read(fd, header, sizeof(*header)) != sizeof(*header)) {
      fprintf(stderr,
              "Error reading the header for chunk ID 0x%08X. Error = %d. "
                 "Offset = %d.\n",
              id,
              errno,
              lseek(fd, 0, SEEK_CUR));
      return 0;
   }

   if (header->id != id) {
      fprintf(stderr,
              "Expected chunk ID = 0x%08x. Read chunk ID = 0x%08X. Offset = "
                 "%d.\n",
              id,
              header->id,
              lseek(fd, 0, SEEK_CUR));
      return 0;
   }

   // Copy data in the chunk into chunk_data
   if (chunk_data) {
      assert(data_size > 0);

      if (data_size > header->size) {
         fprintf(stderr,
                 "Tried to read more data than the chunk contains for ID "
                    "0x%08X. Chunk data size = %d bytes, read size = %d "
                    "bytes. Offset = %d.\n",
                 id,
                 header->size,
                 data_size,
                 lseek(fd, 0, SEEK_CUR));
         return 0;         
      }

      if (read(fd, chunk_data, data_size) != data_size) {
         fprintf(stderr,
                 "Error while reading chunk data for chunk ID 0x%08X. "
                    "Error = %d. Offset = %d.\n",
                 id,
                 errno,
                 lseek(fd, 0, SEEK_CUR));
         return 0;
      }

      // data_size now contains the number of bytes of chunk data to skip
      data_size = header->size - data_size;
   } else
     data_size = header->size;

   if (skip_data) {
      // Add a padding byte if necessary
      data_size += header->size % 2;

      // Skip the rest of the chunk's data
      if (lseek(fd, data_size, SEEK_CUR)==-1) {
         fprintf(stderr,
                 "Error while seeking to the end of chunk ID 0x%08X. Error = "
                    "%d.\n",
                 id,
                 errno);
         return 0;
      }
   }

   return 1;
}

// Skips every chunk in the file until it finds one with an ID matching
// target_id. On success, the matching chunk header is copied into header and a
// non-zero value is returned.
static int skip_chunks(int fd, unsigned int target_id, struct chunk_header *header) {
   if (!header) {
      fprintf(stderr,
              "skip_chunks: NULL header while looking for chunk ID 0x%08X\n",
              target_id);
      return 0;
   }

   do {
      if (read(fd, header, sizeof(*header)) != sizeof(*header)) {
         skip_chunk_error("Read error", target_id);
         return 0;
      }

      if (header->id != target_id) {
         // Skip to the next chunk
         if (lseek(fd, header->size + (header->size % 2), SEEK_CUR)==-1) {
            skip_chunk_error("Unexpected EOF", target_id);
            return 0;
         }
      }
   } while (header->id != target_id);

   return 1;
}

void print_riff_info(struct riff_header* h) {

  char id_string[5];
  char fmt_string[5];

  memcpy(id_string, &h->header.id, 4);
  id_string[4] = '\0';

  memcpy(fmt_string, &h->Format, 4);
  fmt_string[4] = '\0';

  printf("-- Riff Header --\n");
  printf("  Chunk ID: 0x%x (%s)\n", h->header.id, id_string);
  printf("  Size: %.2f KiB\n", h->header.size / 1024.0);
  printf("  Format: 0x%x (%s)\n", h->Format, fmt_string);

}

void print_format_info(struct fmt_header* h) {

  char id_string[5];

  memcpy(id_string, &h->header.id, 4);
  id_string[4] = '\0';

  printf("-- Format Header --\n");
  printf("  Chunk ID: 0x%x (%s)\n", h->header.id, id_string);
  printf("  Size: %i bytes\n", h->header.size);
  printf("  Audio Format: %i (1=PCM)\n", h->AudioFormat);
  printf("  Num Channels: %i (2=Stereo)\n", h->NumChannels);
  printf("  Sampling Rate: %i Hz\n", h->SampleRate);
  printf("  ByteRate: %i Bps\n", h->ByteRate);
  printf("  BlockAlign: %i\n", h->BlockAlign);
  printf("  Bits/Sample: %i\n", h->BitsPerSample);

}

void print_data_info(struct chunk_header* h) {

  char id_string[5];
  memcpy(id_string, &h->id, 4);
  id_string[4] = '\0';

  printf("-- Data Header --\n");
  printf("  Chunk ID: 0x%x (%s)\n", h->id, id_string);
  printf("  Size: %.2f KiB\n", h->size / 1024.0);

}

int process_headers(int fd, struct wav_file_headers *h) {

  if (!h) {
     fprintf(stderr, "process_headers: wav_file_header is NULL\n");
     return 0;
  }

  if (!read_chunk(fd, RIFF_CHUNK_ID, &h->riff.header, (unsigned char *)&h->riff + sizeof(h->riff.header), sizeof(h->riff) - sizeof(h->riff.header), 0))
     return 0;

  if (!read_chunk(fd, FMT_CHUNK_ID, &h->fmt.header, (unsigned char *)&h->fmt + sizeof(h->fmt.header), sizeof(h->fmt) - sizeof(h->fmt.header), 1))
     return 0;

  // Skip optional chunks
  if (!skip_chunks(fd, DATA_CHUNK_ID, &h->data))
     return 0;

  return 1;
}

int write_headers(int fd, struct wav_file_headers* h) {

   off_t offset = lseek(fd, 0, SEEK_CUR);

   if (offset==-1) {
      fprintf(stderr,
              "write_headers: Could not get the file offset. Error = %d.\n",
              errno);
      return 0;
   }

   if (!h) {
      fprintf(stderr, "write_headers: wav_file_headers is NULL\n");
      return 0;
   }

  // RIFF Header
  if (write(fd, &h->riff, sizeof(h->riff)) != sizeof(h->riff)) {
     fprintf(stderr,
             "write_headers: Error while writing RIFF header. Start offset = "
                "%d. Error = %d.\n",
             offset,
             errno);
     return 0;
   }

  offset += sizeof(h->riff);

  // FMT Header
  if (write(fd, &h->fmt, sizeof(h->fmt)) != sizeof(h->fmt)) {
     fprintf(stderr,
             "write_headers: Error while writing format header. Start offset = "
                "%d. Error = %d.\n",
             offset,
             errno);
     return 0;
  }

  offset += sizeof(h->fmt);

  // DATA Header
  if (write(fd, &h->data, sizeof(h->data)) != sizeof(h->data)) {
     fprintf(stderr,
             "write_headers: Error while writing data header. Start offset = "
               "%d. Error = %d.\n",
             offset,
             errno);
     return 0;
  }

  return 1;
}

int fp_write_headers(FILE* fp, struct wav_file_headers* h) {

   int fd = fileno(fp);

   if (fd==-1) {
      fprintf(stderr, "fp_write_headers: Could not get file descriptor\n");
      return 0;
   }

   return write_headers(fd, h);
}
