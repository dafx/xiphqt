/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS BEFORE DISTRIBUTING.                     *
 *                                                                  *
 * THE Ogg123 SOURCE CODE IS (C) COPYRIGHT 2000-2001                *
 * by Kenneth C. Arnold <ogg@arnoldnet.net> AND OTHER CONTRIBUTORS  *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 last mod: $Id: status.c,v 1.1.2.7.2.5 2001/12/11 15:05:56 volsung Exp $

 ********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "buffer.h"
#include "status.h"

char temp_buffer[200];
int last_line_len = 0;
int max_verbosity = 0;

pthread_mutex_t output_lock = PTHREAD_MUTEX_INITIALIZER;


/* ------------------- Private functions ------------------ */

void write_buffer_state_string (char *dest, buffer_stats_t *buf_stats)
{
  char *cur = dest;
  char *comma = ", ";
  char *sep = "(";

  if (buf_stats->prebuffering) {
    cur += sprintf (cur, "%sPrebuf", sep);
    sep = comma;
  }
  if (buf_stats->paused) {
    cur += sprintf (cur, "%sPaused", sep);
    sep = comma;
  }
  if (buf_stats->eos) {
    cur += sprintf (cur, "%sEOS", sep);
    sep = comma;
  }
  if (cur != dest)
    cur += sprintf (cur, ")");
  else
    *cur = '\0';
}


/* Write a min:sec.msec style string to dest corresponding to time.
   The time parameter is in seconds.  Returns the number of characters
   written */
int write_time_string (char *dest, double time)
{
  long min = (long) time / (long) 60;
  double sec = time - 60.0f * min;

  return sprintf (dest, "%02li:%05.2f", min, sec);
}

#if 0
void SetTime (stat_format_t stats[], ogg_int64_t sample)
{
  double CurTime = (double) sample / (double) Options.outputOpts.rate;
  long c_min = (long) CurTime / (long) 60;
  double c_sec = CurTime - 60.0f * c_min;
  long r_min, t_min;
  double r_sec, t_sec;

  if (stats[2].enabled && Options.inputOpts.seekable) {
    if (sample > Options.inputOpts.totalSamples) {
      /* file probably grew while playing; update total time */
      Options.inputOpts.totalSamples = sample;
      Options.inputOpts.totalTime = CurTime;
      stats[3].arg.stringarg[0] = '\0';
      r_min = 0;
      r_sec = 0.0f;
    } else {
      r_min = (long) (Options.inputOpts.totalTime - CurTime) / (long) 60;
      r_sec = ((double) Options.inputOpts.totalTime - CurTime) - 60.0f * (double) r_min;
    }
    sprintf (stats[2].arg.stringarg, "[%02li:%05.2f]", r_min, r_sec);
    if (stats[3].arg.stringarg[0] == '\0') {
      t_min = (long) Options.inputOpts.totalTime / (long) 60;
      t_sec = Options.inputOpts.totalTime - 60.0f * t_min;
      sprintf (stats[3].arg.stringarg, "%02li:%05.2f", t_min, t_sec);
    }    
  }
  sprintf (stats[1].arg.stringarg, "%02li:%05.2f", c_min, c_sec);
}
#endif


void clear_line (int len)
{
  fputc('\r', stderr);

  while (len > 0) {
    fputc (' ', stderr);
    len--;
  }

  fputc ('\r', stderr);
}


int sprintf_clear_line(int len, char *buf)
{
  int i = 0;

  buf[i] = '\r';
  i++;

  while (len > 0) {
    buf[i] = ' ';
    len--;
    i++;
  }

  buf[i] = '\r';
  i++;

  /* Null terminate just in case */
  buf[i] = '\0';

  return i;
}

int print_statistics_line (stat_format_t stats[])
{
  int len = 0;
  char *str = temp_buffer;

  /* Put the clear line text into the same string buffer so that the
     line is cleared and redrawn all at once.  This reduces
     flickering.  Don't count characters used to clear line in len */
  str += sprintf_clear_line(last_line_len, str); 

  while (stats->formatstr != NULL) {
    
    if (stats->verbosity > max_verbosity || !stats->enabled) {
      stats++;
      continue;
    }

    if (len != 0)
      len += sprintf(str+len, " ");

    switch (stats->type) {
    case stat_noarg:
      len += sprintf(str+len, stats->formatstr);
      break;
    case stat_intarg:
      len += sprintf(str+len, stats->formatstr, stats->arg.intarg);
      break;
    case stat_stringarg:
      len += sprintf(str+len, stats->formatstr, stats->arg.stringarg);
      break;
    case stat_floatarg:
      len += sprintf(str+len, stats->formatstr, stats->arg.floatarg);
      break;
    case stat_doublearg:
      len += sprintf(str+len, stats->formatstr, stats->arg.doublearg);
      break;
    }

    stats++;
  }

  len += sprintf(str+len, "\r");

  fprintf(stderr, "%s", temp_buffer);

  return len;
}


void vstatus_print_nolock (const char *fmt, va_list ap)
{
  if (last_line_len != 0)
    fputc ('\n', stderr);

  vfprintf (stderr, fmt, ap);

  fputc ('\n', stderr);

  last_line_len = 0;
}


/* ------------------- Public interface -------------------- */

#define TIME_STR_SIZE 20
#define STATE_STR_SIZE 25
#define NUM_STATS 10

stat_format_t *stat_format_create ()
{
  stat_format_t *stats;
  stat_format_t *cur;

  stats = calloc(NUM_STATS + 1, sizeof(stat_format_t));  /* One extra for end flag */
  if (stats == NULL) {
    fprintf(stderr, "Memory allocation error in stats_init()\n");
    exit(1);
  }

  cur = stats + 0; /* currently playing file / stream */
  cur->verbosity = 3; 
  cur->enabled = 0;
  cur->formatstr = "File: %s"; 
  cur->type = stat_stringarg;
  
  cur = stats + 1; /* current playback time (preformatted) */
  cur->verbosity = 1;
  cur->enabled = 1;
  cur->formatstr = "Time: %s"; 
  cur->type = stat_stringarg;
  cur->arg.stringarg = calloc(TIME_STR_SIZE, sizeof(char));

  if (cur->arg.stringarg == NULL) {
    fprintf(stderr, "Memory allocation error in stats_init()\n");
    exit(1);
  }

    
  cur = stats + 2; /* remaining playback time (preformatted) */
  cur->verbosity = 1;
  cur->enabled = 1;
  cur->formatstr = "[%s]";
  cur->type = stat_stringarg;
  cur->arg.stringarg = calloc(TIME_STR_SIZE, sizeof(char));

  if (cur->arg.stringarg == NULL) {
    fprintf(stderr, "Memory allocation error in stats_init()\n");
    exit(1);
  }


  cur = stats + 3; /* total playback time (preformatted) */
  cur->verbosity = 1;
  cur->enabled = 1;
  cur->formatstr = "of %s";
  cur->type = stat_stringarg;
  cur->arg.stringarg = calloc(TIME_STR_SIZE, sizeof(char));

  if (cur->arg.stringarg == NULL) {
    fprintf(stderr, "Memory allocation error in stats_init()\n");
    exit(1);
  }


  cur = stats + 4; /* instantaneous bitrate */
  cur->verbosity = 2;
  cur->enabled = 1;
  cur->formatstr = "Bitrate: %5.1f";
  cur->type = stat_doublearg;

  cur = stats + 5; /* average bitrate (not yet implemented) */
  cur->verbosity = 2;
  cur->enabled = 0;
  cur->formatstr = "Avg bitrate: %5.1f";
  cur->type = stat_doublearg;

  cur = stats + 6; /* input buffer fill % */
  cur->verbosity = 2;
  cur->enabled = 0;
  cur->formatstr = " Input Buffer %5.1f%%";
  cur->type = stat_doublearg;

  cur = stats + 7; /* input buffer status */
  cur->verbosity = 2;
  cur->enabled = 0;
  cur->formatstr = "%s";
  cur->type = stat_stringarg;
  cur->arg.stringarg = calloc(STATE_STR_SIZE, sizeof(char));

  if (cur->arg.stringarg == NULL) {
    fprintf(stderr, "Memory allocation error in stats_init()\n");
    exit(1);
  }


  cur = stats + 8; /* output buffer fill % */
  cur->verbosity = 2;
  cur->enabled = 0;
  cur->formatstr = " Output Buffer %5.1f%%"; 
  cur->type = stat_doublearg;

  cur = stats + 9; /* output buffer status */
  cur->verbosity = 1;
  cur->enabled = 0;
  cur->formatstr = "%s";
  cur->type = stat_stringarg;
  cur->arg.stringarg = calloc(STATE_STR_SIZE, sizeof(char));

  if (cur->arg.stringarg == NULL) {
    fprintf(stderr, "Memory allocation error in stats_init()\n");
    exit(1);
  }


  cur = stats + 10; /* End flag */
  cur->formatstr = NULL;

  return stats;
}


void stat_format_cleanup (stat_format_t *stats)
{
  free(stats[1].arg.stringarg);
  free(stats[2].arg.stringarg);
  free(stats[3].arg.stringarg);
  free(stats[7].arg.stringarg);
  free(stats[9].arg.stringarg);
  free(stats);
}


void status_set_verbosity (int verbosity)
{
  max_verbosity = verbosity;
}


void status_reset_output_lock ()
{
  pthread_mutex_unlock(&output_lock);
}


void status_clear_line ()
{
  pthread_mutex_lock(&output_lock);

  clear_line(last_line_len);

  pthread_mutex_unlock(&output_lock);
}

void status_print_statistics (stat_format_t *stats,
			      buffer_stats_t *audio_statistics,
			      data_source_stats_t *transport_statistics,
			      decoder_stats_t *decoder_statistics)
{

  /* Updating statistics is not critical.  If another thread is
     already doing output, we skip it. */
  if (pthread_mutex_trylock(&output_lock) == 0) {

    if (decoder_statistics) {
      /* Current playback time */
      write_time_string(stats[1].arg.stringarg,
			decoder_statistics->current_time);
	
      /* Remaining playback time */
      write_time_string(stats[2].arg.stringarg,
			decoder_statistics->total_time - 
			decoder_statistics->current_time);
      
      /* Total playback time */
      write_time_string(stats[3].arg.stringarg,
			decoder_statistics->total_time);

      /* Instantaneous bitrate */
      stats[4].arg.doublearg = decoder_statistics->instant_bitrate / 1000.0f;

      /* Instantaneous bitrate */
      stats[5].arg.doublearg = decoder_statistics->avg_bitrate / 1000.0f;
    }


    if (transport_statistics != NULL && 
	transport_statistics->input_buffer_used) {
      
      /* Input buffer fill % */
      stats[6].arg.doublearg = transport_statistics->input_buffer.fill;

      /* Input buffer state */
      write_buffer_state_string(stats[7].arg.stringarg,
				&transport_statistics->input_buffer);
    }


    if (audio_statistics != NULL) {
      
      /* Output buffer fill % */
      stats[8].arg.doublearg = audio_statistics->fill;
      
      /* Output buffer state */
      write_buffer_state_string(stats[9].arg.stringarg, audio_statistics);
    }
    
    last_line_len = print_statistics_line(stats);

    pthread_mutex_unlock(&output_lock);
  }
}


void status_message (int verbosity, const char *fmt, ...)
{
  va_list ap;

  if (verbosity > max_verbosity)
    return;

  pthread_mutex_lock(&output_lock);

  va_start (ap, fmt);
  vstatus_print_nolock(fmt, ap);
  va_end (ap);

  pthread_mutex_unlock(&output_lock);
}


void vstatus_message (int verbosity, const char *fmt, va_list ap)
{
  if (verbosity > max_verbosity)
    return;

  pthread_mutex_lock(&output_lock);

  vstatus_print_nolock(fmt, ap);

  pthread_mutex_unlock(&output_lock);
}


void status_error (const char *fmt, ...)
{
  va_list ap;

  pthread_mutex_lock(&output_lock);

  va_start (ap, fmt);
  vstatus_print_nolock (fmt, ap);
  va_end (ap);

  pthread_mutex_unlock(&output_lock);
}


void vstatus_error (const char *fmt, va_list ap)
{
  pthread_mutex_lock(&output_lock);

  vstatus_print_nolock (fmt, ap);

  pthread_mutex_unlock(&output_lock);
}


void print_statistics_callback (buf_t *buf, void *arg)
{
  print_statistics_arg_t *stats_arg = (print_statistics_arg_t *) arg;
  buffer_stats_t *buffer_stats;

  if (buf != NULL)
    buffer_stats = buffer_statistics(buf);
  else
    buffer_stats = NULL;

  status_print_statistics(stats_arg->stat_format,
			  buffer_stats,
			  stats_arg->transport_statistics,
			  stats_arg->decoder_statistics);

  free(stats_arg->transport_statistics);
  free(stats_arg->decoder_statistics);
  free(stats_arg);
}


print_statistics_arg_t *new_print_statistics_arg (
			       stat_format_t *stat_format,
			       data_source_stats_t *transport_statistics,
			       decoder_stats_t *decoder_statistics)
{
  print_statistics_arg_t *arg;

  if ( (arg = malloc(sizeof(print_statistics_arg_t))) == NULL ) {
    status_error("Error: Out of memory in new_print_statistics_arg().\n");
    exit(1);
  }  
  
  arg->stat_format = stat_format;
  arg->transport_statistics = transport_statistics;
  arg->decoder_statistics = decoder_statistics;

  return arg;
}
