/*  $Id: wavsilence.c,v 1.7 2003/08/08 14:47:01 dsmith Exp $

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

/*
    tblough 5/23/04 - Added (o)verride option to override minimum track length
	for silence gaps greater than specified amount.  Assumed override time
	is greater than gap time.
	
	tblough 5/25/04 - Modified the name parameter to provide additional
	flexibility on where and how many digits are used for the segment portion.
	Added (N)atural option to allow segment portion to be 0 or 1 based.
*/


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>   /* strncpy() */
#include "wavheader.h"
#include "wavsilence.h"

// GLOBALS
int counter;
FILE* fd;
FILE* logfp;
int debug_level;

void clear_line() {

  printf("\r                                                                   \r");

}

void build_output_filename(int count, char* buffer){  /* tblough 5.25.04 */
  sprintf(buffer, opts.piece_name, (opts.natural ? count + 1 : count));
}

void exec_cmd() {

  int pid;
  int ret;
  char command[FILEN_LENGTH];

  char fname[FILEN_LENGTH];
  build_output_filename(counter-1, fname);

  pid = fork();

  if(pid == 0) {
    pid = getpid();
    // Child
    if(debug_level >= VERYVERBOSE)
      printf("--> [%i] Asyncronously running %s\n", pid, opts.pipe_cmd);
    snprintf(command, FILEN_LENGTH, "%s %s 2>&1 > /dev/null",
	     opts.exec_cmd, fname);
    ret = system(command);
    if(debug_level >= VERYVERBOSE)
      printf("--> [%i] Command exited with status %i\n", pid, ret);

    if(opts.remove_after_exec) {
      unlink( fname);
    }
    exit(0);
  } else
    return;

}

void start_log_file(struct wav_file_headers* wav_headers) {

  FILE* datefp;
  char date[DATE_LENGTH];

  logfp = fopen(opts.log_file, "w");

  if (logfp == NULL) {
    perror("log file");
    exit(1);
  }

  datefp = popen("date", "r");
  fgets(date, DATE_LENGTH, datefp);
  pclose(datefp);

  fprintf(logfp, "# Log file created by wavsilence v%.2f\n", VERSION);
  fprintf(logfp, "# Copyright Dan Smith (2003) - http://www.danplanet.com\n");
  fprintf(logfp, "# Summary generated %s\n", date);

  fprintf(logfp, "# Data read from ");
  if(opts.read_from_file)
    fprintf(logfp, "file: %s\n", opts.input_file);
  else
    fprintf(logfp, "stdin\n");

  fprintf(logfp, "# %i Channels, %i Bit, %i Hz\n\n", 
	  wav_headers->fmt->NumChannels, 
	  wav_headers->fmt->BitsPerSample,
	  wav_headers->fmt->SampleRate);

  fflush(logfp);

}

void finish_log_file(struct wav_file_headers* wav_headers,
		     unsigned int start_time, unsigned int bytecount) {

  unsigned int current_time;

  current_time = time(NULL);

  fprintf(logfp, "\n");
  fprintf(logfp, "# === Totals ===\n");
  fprintf(logfp, "# Elapsed time: %i seconds\n", current_time - start_time);
  fprintf(logfp, "# Data processed: %i KB\n", bytecount / 1024);
  fprintf(logfp, "# Average throughput: %7.1f KB/s\n", 
	  (bytecount / 1024) / (float)(current_time - start_time));

}

int is_silence(short sample) {

  int boundary;
  
  // For now, just assume 16-bit 2's compliment

  boundary = (opts.threshold * 65536) / 2;

  if((sample < boundary) && (sample > (-1 * boundary)))
    return 1;
  else
    return 0;

}

double calc_real_time(int sample_num, struct wav_file_headers* h) {

  return (double)sample_num / h->fmt->SampleRate;

}

void fix_file(int length) {

  int chunksize;

  // Position ourselves at the RIFF Header ChunkSize
  fseek(fd, CHUNK0_OFFSET, SEEK_SET);
  
  chunksize = 36 + length;

  // Write new ChunkSize
  fwrite(&chunksize, sizeof(chunksize), 1, fd);
  
  // Position ourselves at the FMT Header SubchunkSize
  fseek(fd, CHUNK1_OFFSET, SEEK_SET);

  chunksize = 16;

  // Write new SubchunkSize
  fwrite(&chunksize, sizeof(chunksize), 1, fd);

  // Position ourselves at the DATA Header SubchunkSize
  fseek(fd, CHUNK2_OFFSET, SEEK_SET);
  
  chunksize = length;

  // Write new SubchunkSize
  fwrite(&chunksize, sizeof(chunksize), 1, fd);

}

void write_log_entry(unsigned int bytecounter, unsigned int sample_c, 
		     struct wav_file_headers* wav_headers) {
  char fname[FILEN_LENGTH];
  build_output_filename(counter-1, fname);


  fprintf(logfp, "%20s: %9i bytes  %9.2f seconds\n",
	      fname, bytecounter, calc_real_time(sample_c, wav_headers));
}

void start_new_file(struct wav_file_headers* wav_headers, 
		    unsigned int bytecounter, int sample_c) {

  char fname[FILEN_LENGTH];

  if(fd != NULL) {

    if(debug_level >= VERYVERBOSE)
      printf("Wrote %i bytes\n", bytecounter);

    if(! opts.pipe_enabled) // Don't seek if we're piping
      fix_file(bytecounter); 

    fclose(fd);

    if(opts.exec_enabled)
      exec_cmd();

  }

  build_output_filename(counter++, fname);

  if(debug_level >= VERBOSE) {
    if(opts.show_progress) clear_line();
    printf("New File: %s\n", fname);
  }

  if(opts.pipe_enabled)
    fd = popen(opts.pipe_cmd, "w");
  else
    fd = fopen(fname, "w");

  fp_write_headers(fd, wav_headers);


}

void display_stats(int sample_c, int bytecount, unsigned int start_time,
		   struct wav_file_headers* wav_headers) {

  float wavtime;
  int total;
  char human[SIZE_LENGTH];
  int orig_byte_count;
  unsigned int current_time;
  float kbps;

  current_time = time(NULL);

  wavtime = calc_real_time(sample_c, wav_headers);
  orig_byte_count = bytecount;

  if((bytecount % 1024) == bytecount) {
    // Less than 1K
    sprintf(human, "%5i b ", bytecount);
    bytecount = -1;
  } else {
    bytecount = total = bytecount / 1024;
  }
  if((total % 1024) == bytecount) {
    // Less than 1M
    sprintf(human, "%5i KB", total);
    bytecount = -1;
  } else {
    total = total / 1024;
  }

  if(bytecount != -1)
    sprintf(human, "%5i MB", total);

  kbps = (float)(orig_byte_count / 1024) / (float)(current_time - start_time);

  printf("\rProcessed %.2f s - %s total - %.1f KB/s throughput\r", 
	 wavtime, human, kbps);

}

void process_data(struct wav_file_headers* wav_headers, int in_fd) {
  
  int sample_size;
  int sample_c,i;
  short *sample;
  int size, wsize;
  int silence_counter = 0;
  int silence_flag = 0;
  int min_length_flag = 1; /* Default is always split */
  int override_flag = 0;	/* tblough 5/23/04 */
  unsigned int bytecounter = 0;
  unsigned int file_bytecounter = 0;
  unsigned int start_time;
  unsigned int file_sample_c = 0;

  start_time = time(NULL);

  // Array of samples (NumChannels of them)
  sample = calloc(wav_headers->fmt->NumChannels * opts.buffer_amt, sizeof(short));

  sample_size = wav_headers->fmt->BitsPerSample / 8;

  if(debug_level >= VERYVERBOSE)
    printf("sample size: %i\n", sample_size);

  sample_c = 0;
  do {

    size = read(in_fd, sample, sample_size * wav_headers->fmt->NumChannels * opts.buffer_amt);
    for(i=0; i<wav_headers->fmt->NumChannels * opts.buffer_amt; i++) {
      //      sample[i] = 0;
      //      size = read(in_fd, &sample[i], sample_size);

      if((size == 0) && (debug_level >= VERYVERBOSE))
	printf("End of Data\n");

      if(debug_level >= INSANELYVERBOSE)
	printf("[%i,%i] 0x%hx (%hi)  %i\n", sample_c, i, sample[i], sample[i], size);

      //      if((sample[i] < 1000) && (sample[i] > -1000))
      if(is_silence(sample[i]))
	silence_counter++;
      else {
	silence_counter = 0;
      }
    }

    if(silence_counter == 0)
      silence_flag=0;

    if(debug_level >= VERYVERBOSE) {
      printf("min_track_length: %f cur_track_length: %f\n", opts.min_track_length, 
	     file_sample_c / (float)wav_headers->fmt->SampleRate);
    }

    if(opts.min_track_length > 0) {
      /* Check to make sure we've seen enough samples before splitting */
      if( opts.min_track_length <= (file_sample_c / wav_headers->fmt->SampleRate) )
	min_length_flag = 1;
      else
	min_length_flag = 0;
    }
    
    // tblough 5/23/04 - modified to provide minimum track length override
	if((opts.override > opts.gap) && (silence_counter > OVERRIDE)) {
	   if(debug_level >= VERYVERBOSE) {
          printf("Override GAP: %i  Counter: %i\n", OVERRIDE, silence_counter);
          printf("Silence Detected @ %.2fs\n", calc_real_time(sample_c, wav_headers));
       }
       override_flag = 1;
	}
	else
	   override_flag = 0;

	if((silence_counter > GAP) && (! silence_flag) && (min_length_flag || override_flag)) {

      if(debug_level >= VERYVERBOSE) {
	printf("Silence GAP: %i  Counter: %i\n", GAP, silence_counter);
	printf("Silence Detected @ %.2fs\n", calc_real_time(sample_c, wav_headers));
      }

      silence_flag=1;
      if(opts.log_enabled)
	write_log_entry(file_bytecounter, file_sample_c, wav_headers);
      start_new_file(wav_headers, file_bytecounter, sample_c);
      file_bytecounter = 0;
      file_sample_c = 0;
    }
      
	// Only write if we should not skip the silence and there is silence
	// loescher 06/06/04
	if (! (opts.skip_silence && silence_flag) ) {
	  wsize = fwrite(sample, sample_size * wav_headers->fmt->NumChannels * opts.buffer_amt, 1, fd);
	  
	  file_bytecounter += wsize * sample_size * wav_headers->fmt->NumChannels * opts.buffer_amt;
	  bytecounter += wsize * sample_size * wav_headers->fmt->NumChannels * opts.buffer_amt;
	}

    sample_c += opts.buffer_amt;
    file_sample_c += opts.buffer_amt;

    // Display stats
    if((opts.show_progress) && ((sample_c % 1000) == 0))
      display_stats(sample_c, bytecounter, start_time, wav_headers);

  } while(size > 0);

  // Fix final file and close FD
  //  file_bytecounter += opts.buffer_amt * wav_headers->fmt->NumChannels * sample_size;
  //  file_sample_c += opts.buffer_amt;

  if(! opts.pipe_enabled) // Don't seek if we're piping
    fix_file(file_bytecounter);

  if(opts.log_enabled)
    write_log_entry(file_bytecounter, file_sample_c, wav_headers);

  fclose(fd);

  if(opts.exec_enabled)
    exec_cmd();

  if(opts.log_enabled)
    finish_log_file(wav_headers, start_time, bytecounter);

}

void print_usage() {

  printf("wavsilence v%.2f - Dan Smith (dsmith@danplanet.com)\n", VERSION);
  printf("Usage: wavsilence <options>\n");
  printf("Options:\n");
  printf("  -g <gap>       Minimum gap (in seconds) to be considered silence\n");
  printf("  -t <threshold> Volume (in %% of Max) to be considered silence\n");
  printf("  -v             Verbose mode (specify multiple times to increase verbosity)\n");
  printf("  -I             Print input WAV information\n");
  printf("  -e <cmd>       Execute <cmd> when each piece is finished, with the filename\n");
  printf("                 as the last argument\n");
  printf("  -r             Remove the WAV file after <cmd> (only valid with -e)\n");
  printf("  -p             Display progress and statistics during operation\n");
  printf("  -i <file>      Read from <file> instead of stdin\n");
  printf("  -n <name>      Name output files <name>.  '%%n' can be used to locate the\n");
  printf("                 the segment number where 'n' is the number of digits\n");
  printf("                 (i.e. '-n piece-%%3' would produce piece-000, piece 001, ...)\n");
  printf("  -N             Use 1,2,3,... vs 0,1,2,... for segment numbering\n");
  printf("  -l <file>      Log summary information in <file>\n");
  printf("  -b <num>       Buffer input by <num> samples (1 is default; try 16)\n");
  printf("  -P <cmd>       Pipe output of each segment to <cmd>\n");
  printf("  -m <seconds>   Minimum track length (seconds)\n");
  printf("  -M <minutes>   Minimum track length (minutes)\n");
  printf("  -o <override>  Minimum gap (in seconds) to override minimum track length\n");
  printf("                 Longer gaps will begin a new track regardless of track length\n");
  printf("  -s             Skip silence (remove the silence between pieces)\n");
  printf("  -c <num>       Counter-start. With this option you can set the initial\n                 value of the file-number-counter.\n");
  printf("  -h             Display this message\n");
  printf("Operation:\n");
  printf("  WAV file is read via stdin, split at points of silence into files\n");
  printf("  named \"piece-N.wav\" (unless user specified)\n");
  printf("  The initial value of N defaults to 0 but can be specified by -c.\n");
  printf("Examples:\n");
  printf("  wavsilence -g 1.1 -o 3.5 -t 4 -b 64 -l log.txt -p -M 3 -i test.wav\n");
  printf("  wavsilence -v -s -b 64 -n '%2' -c 26 -i recording_of_song_26_to_31.wav\n");
  
  exit(1);
}

void set_name( char *optarg)  /* tblough 5.25.04 */
{
	char *p;
	char tmp[FILEN_LENGTH - 6];
	int width;

    /* leave enough room for the '0' and 'i' and the '.wav' we will add */
	strncpy(tmp, optarg, FILEN_LENGTH - 6);
	/* make sure there is one and only one occurance of the flag */
	if( ((p = strchr( tmp, '%')) != '\0') && (strrchr( tmp, '%') == p))
	{
		width = tmp[p - tmp + 1] - '0';  /* grab the next character, convert to int */
		if( width >= 1 && width <= 9)
		{
			strncpy( opts.piece_name, tmp, p - tmp + 1);	/* copy prepended info */
			opts.piece_name[p - tmp + 1] = '\0';
			sprintf( opts.piece_name, "%s0%ii%s.wav", opts.piece_name, width, p+2);
			return;
		}
	}
	/* more than one field marked, or not a vaild field width, so append the segment */
	strncpy( opts.piece_name, optarg, FILEN_LENGTH - 9);
	sprintf( opts.piece_name, "%s-%%03i.wav", opts.piece_name);
}

void process_args(int argc, char**argv) {
  int c;

  while((c = getopt(argc, argv, "re:n:P:b:i:Vl:psIvht:g:o:m:M:Nc:")) != -1) {
    switch (c) {
    case 't':
      opts.threshold = atof(optarg) / 100.0;
      if(opts.threshold <= 0) {
	printf("Invalid threshold value!\n");
	exit(1);
      }
      break;
    case 'g':
      opts.gap = atof(optarg);
      if(opts.gap <= 0) {
	printf("Invalid gap length!\n");
	exit(1);
      }
      break;
    case 'o':
      opts.override = atof(optarg);
      if(opts.override <= 0) {
	printf("Invalid override length!\n");
	exit(1);
      }
      break;
    case 'v':
      debug_level++;
      break;
    case 'I':
      opts.show_file_info = 1;
      break;
    case 'p':
      opts.show_progress = 1;
      break;
    case 'l':
      opts.log_enabled = 1;
      strncpy(opts.log_file, optarg, FILEN_LENGTH);
      break;
    case 'V':
      printf("RCS Tag: $Id: wavsilence.c,v 1.7 2003/08/08 14:47:01 dsmith Exp $ \n");
      exit(1);
      break;
    case 'i':
      opts.read_from_file = 1;
      strncpy(opts.input_file, optarg, FILEN_LENGTH);
      break;
    case 'b':
      opts.buffer_amt = atoi(optarg);
      if(opts.buffer_amt <= 0) {
	printf("Invalid Buffer amount!\n");
	exit(1);
      }
	
      break;
    case 'P':
      opts.pipe_enabled = 1;
      strncpy(opts.pipe_cmd, optarg, FILEN_LENGTH);
      break;
    case 'n':
      set_name(optarg);
      break;
    case 'e':
      opts.exec_enabled = 1;
      strncpy(opts.exec_cmd, optarg, FILEN_LENGTH);
      break;
    case 'r':
      opts.remove_after_exec = 1;
      break;
    case 'm':
      opts.min_track_length += atof(optarg);
      break;
    case 'M':
      opts.min_track_length += (60 * atof(optarg));
      break;
    case 'N':
      opts.natural = 1;
      break;
    case 's':
      opts.skip_silence = 1;
      break;
    case 'c':
      opts.counter_start = atoi(optarg);
      break;

      // DEFAULT
    default:
    case 'h':
      print_usage();
      break;
    }
  }

    return;    

}

void print_params() {

  printf("Minimum Gap: %.2f s\n", opts.gap);
  printf("Minimum Track Length: %.2f s\n", opts.min_track_length);
  printf("Override Gap: %.2f s\n", opts.override);
  printf("Threshold: %.0f %%\n", opts.threshold * 100);
  printf("Verbosity: %i\n", debug_level);
  printf("Buffer: %i samples\n", opts.buffer_amt);
}


int open_input_file() {

  int fd;

  if(debug_level >=VERBOSE)
    printf("Opened file %s for input\n", opts.input_file);

  fd = open(opts.input_file, O_RDONLY);

  if(fd == -1) {
    perror("input file");
    exit(1);
  }

  return fd;
}

int main(int argc, char**argv) {
  
  struct wav_file_headers* wav_headers;
  int input_fd;

  // Initialization
  //  counter = 0; // loescher 07/06/04
  fd = NULL;

  // Defaults
  opts.threshold = 0.03;
  opts.gap = 1.0;
  opts.override = 0.0;
  opts.show_file_info = 0;
  opts.show_progress = 0;
  opts.log_enabled = 0;
  opts.read_from_file = 0;
  opts.buffer_amt = 1;
  opts.pipe_enabled = 0;
  opts.exec_enabled = 0;
  opts.remove_after_exec = 0;
  strncpy(opts.piece_name, DEFAULT_NAME, FILEN_LENGTH);
  debug_level = 0;
  opts.min_track_length = 0;
  opts.natural = 0;
  opts.skip_silence = 0; // loescher 06/06/04
  opts.counter_start = 0; // loescher 07/06/04
  
  process_args(argc, argv);

  // loescher 07/06/04
  counter = opts.counter_start;

  if(debug_level >= VERBOSE)
    print_params();

  if(opts.read_from_file)
    input_fd = open_input_file();
  else
    input_fd = 0; // STDIN

  wav_headers = process_headers(input_fd);

  if(opts.log_enabled)
    start_log_file(wav_headers);

  if(opts.show_file_info)
    print_format_info(wav_headers->fmt);

  start_new_file(wav_headers, 0, 0);

  process_data(wav_headers, input_fd);
  
  exit(0);
};
