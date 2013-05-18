/*  $Id: wavsilence.h,v 1.2 2003/08/08 14:47:08 dsmith Exp $

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

/*
    tblough 5/23/04 - Added override option to override minimum track length
	for silence gaps greater than specified amount.  Assumed override time
	is greater than gap time.
	
	tblough 5/25/04 - Modified the name parameter to provide additional
	flexibility on where and how many digits are used for the segment portion.
	Added (N)atural option to allow segment portion to be 0 or 1 based.
*/


#ifndef _WAVSILENCE_H
#define _WAVSILENCE_H

#define VERBOSE         1
#define VERYVERBOSE     2
#define INSANELYVERBOSE 3

// These should be fine for all WAV files
#define CHUNK0_OFFSET   4
#define CHUNK1_OFFSET   16
#define CHUNK2_OFFSET   40

#define FILEN_LENGTH    256
#define SIZE_LENGTH     256
#define DATE_LENGTH     256


#define PROG_MULT       700

#define DEFAULT_NAME	"piece-%03i.wav"

/* GAP is the calculated number of samples for opts.gap seconds */
#define GAP ((int)(wav_headers->fmt.SampleRate * opts.gap * wav_headers->fmt.NumChannels))
#define OVERRIDE ((int)(wav_headers->fmt.SampleRate * opts.override * wav_headers->fmt.NumChannels))

struct ws_opts {

  float threshold;
  double gap;
  double override;	/* tblough 5/23/04 */
  int sample_width;
  int show_file_info;
  int read_from_file;
  char input_file[FILEN_LENGTH];
  char output_prefix[FILEN_LENGTH];
  int exec_enabled;
  char exec_cmd[FILEN_LENGTH];
  int remove_after_exec;
  int show_progress;
  int log_enabled;
  char log_file[FILEN_LENGTH];
  int buffer_amt;
  char pipe_cmd[FILEN_LENGTH];
  int pipe_enabled;
  char piece_name[FILEN_LENGTH];
  float min_track_length;
  int natural;	/* tblough 5/25/04 */
  int skip_silence; // loescher 06/06/04
  int counter_start; // loescher 07/06/04

} opts;

#endif
