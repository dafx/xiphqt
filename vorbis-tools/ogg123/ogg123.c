/* ogg123.c by Kenneth Arnold <ogg123@arnoldnet.net> */
/* Modified to use libao by Stan Seibert <volsung@asu.edu> */

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

 last mod: $Id: ogg123.c,v 1.39.2.18 2001/08/13 17:31:46 kcarnold Exp $

 ********************************************************************/

#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>

#include "ogg123.h"
#include "ao_interface.h"
#include "curl_interface.h"
#include "buffer.h"
#include "options.h"
#include "status.h"

/* take buffer out of the data segment, not the stack */
#define BUFFER_CHUNK_SIZE 4096
char convbuffer[BUFFER_CHUNK_SIZE];
int convsize = BUFFER_CHUNK_SIZE;

/* take big options structure from the data segment */
ogg123_options_t Options;

static char skipfile_requested;
static char exit_requested;

struct {
    char *key;			/* includes the '=' for programming convenience */
    char *formatstr;		/* formatted output */
} ogg_comment_keys[] = {
  {"ARTIST=", "Artist: %s\n"},
  {"ALBUM=", "Album: %s\n"},
  {"TITLE=", "Title: %s\n"},
  {"VERSION=", "Version: %s\n"},
  {"TRACKNUMBER=", "Track number: %s\n"},
  {"ORGANIZATION=", "Organization: %s\n"},
  {"GENRE=", "Genre: %s\n"},
  {"DESCRIPTION=", "Description: %s\n"},
  {"DATE=", "Date: %s\n"},
  {"LOCATION=", "Location: %s\n"},
  {"COPYRIGHT=", "Copyright %s\n"},
  {NULL, NULL}
};

struct option long_options[] = {
  /* GNU standard options */
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'V'},
    /* ogg123-specific options */
    {"buffer", required_argument, 0, 'b'},
    {"config", optional_argument, 0, 'c'},
    {"device", required_argument, 0, 'd'},
    {"file", required_argument, 0, 'f'},
    {"skip", required_argument, 0, 'k'},
    {"delay", required_argument, 0, 'l'},
    {"device-option", required_argument, 0, 'o'},
    {"prebuffer", required_argument, 0, 'p'},
    {"quiet", no_argument, 0, 'q'},
    {"save", required_argument, 0, 's'},
    {"verbose", no_argument, 0, 'v'},
    {"nth", required_argument, 0, 'x'},
    {"ntimes", required_argument, 0, 'y'},
    {"shuffle", no_argument, 0, 'z'},
    {0, 0, 0, 0}
};

/* configuration interface */
int ConfigErrorFunc (void *arg, ParseCode pcode, int lineno, char *filename, char *line)
{
  if (pcode == parse_syserr)
    {
      if (errno != EEXIST && errno != ENOENT)
	perror ("System error");
      return -1;
    }
  else
    {
      int len = 80 + strlen(filename) + strlen(line);
      char *buf = malloc (len);
      if (!buf) { perror ("malloc"); return -1; }
      snprintf (buf, len, "Parse error: %s on line %d of %s (%s)\n", ParseErr(pcode), lineno, filename, line);
      ShowMessage (0, 0, buf);
      free(buf);
      return 0;
    }
}

ParseCode ReadConfig (Option_t opts[], char *filename)
{
  return ParseFile (opts, filename, ConfigErrorFunc, NULL);
}

void ReadStdConfigs (Option_t opts[])
{
  char filename[FILENAME_MAX];
  char *homedir = getenv("HOME");

  /* Read config from files in same order as original parser */
  if (homedir && strlen(homedir) < FILENAME_MAX - 10) {
    /* Try ~/.ogg123 */
    strncpy (filename, homedir, FILENAME_MAX);
    strcat (filename, "/.ogg123rc");
    ReadConfig (opts, filename);
  }
  ReadConfig (opts, "/etc/ogg123rc");
}
/* /configuration interface */

/* status interface */
/* string temporary data (from the data segment) */
char TwentyBytes1[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
char TwentyBytes2[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
char TwentyBytes3[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void InitOgg123Stats (Stat_t stats[])
{
  Stat_t *cur;

  cur = &stats[0]; /* currently playing file / stream */
  cur->prio = 3; cur->enabled = 0; cur->formatstr = "File: %s"; cur->type = stat_stringarg;
  cur = &stats[1]; /* current playback time (preformatted) */
  cur->prio = 1; cur->enabled = 1; cur->formatstr = "Time: %s"; cur->type = stat_stringarg; cur->arg.stringarg = TwentyBytes1;
  cur = &stats[2]; /* remaining playback time (preformatted) */
  cur->prio = 1; cur->enabled = 0; cur->formatstr = "%s"; cur->type = stat_stringarg; cur->arg.stringarg = TwentyBytes2;
  cur = &stats[3]; /* total playback time (preformatted) */
  cur->prio = 1; cur->enabled = 0; cur->formatstr = "of %s"; cur->type = stat_stringarg; cur->arg.stringarg = TwentyBytes3;
  cur = &stats[4]; /* instantaneous bitrate */
  cur->prio = 2; cur->enabled = 1; cur->formatstr = "Bitrate: %5.1f"; cur->type = stat_doublearg;
  cur = &stats[5]; /* average bitrate (not yet implemented) */
  cur->prio = 2; cur->enabled = 0; cur->formatstr = "Avg bitrate: %5.1f"; cur->type = stat_doublearg;
  cur = &stats[6]; /* input buffer fill % */
  cur->prio = 2; cur->enabled = 0; cur->formatstr = " Input Buffer %5.1f%%"; cur->type = stat_doublearg;
  cur = &stats[7]; /* input buffer status */
  cur->prio = 3; cur->enabled = 0; cur->formatstr = "(%s)"; cur->type = stat_stringarg; cur->arg.stringarg = NULL;
  cur = &stats[8]; /* output buffer fill % */
  cur->prio = 2; cur->enabled = 0; cur->formatstr = " Output Buffer %5.1f%%"; cur->type = stat_doublearg;
  cur = &stats[9]; /* output buffer status */
  cur->prio = 3; cur->enabled = 0; cur->formatstr = "%s"; cur->type = stat_stringarg; cur->arg.stringarg = NULL;
}

void SetTime (Stat_t stats[], ogg_int64_t sample)
{
  double time = (double) sample / (double) Options.outputOpts.rate;
  long c_min = (long) time / (long) 60;
  double c_sec = time - 60.0f * c_min;
  long r_min, t_min;
  double r_sec, t_sec;

  if (stats[2].enabled && Options.inputOpts.seekable) {
    if (sample > Options.inputOpts.totalSamples) {
      /* file probably grew while playing; update total time */
      Options.inputOpts.totalSamples = sample;
      Options.inputOpts.totalTime = time;
      stats[3].arg.stringarg[0] = '\0';
      r_min = 0;
      r_sec = 0.0f;
    } else {
      r_min = (long) (Options.inputOpts.totalTime - time) / (long) 60;
      r_sec = ((double) Options.inputOpts.totalTime - time) - 60.0f * (double) r_min;
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

void SetBufferStats (buf_t *buf, char *strbuf)
{
  char *cur = strbuf;
  char *comma = ", ";
  char *sep = "(";

  if (buf->StatMask & STAT_PREBUFFERING) {
    cur += sprintf (cur, "%sPrebuffering", sep);
    sep = comma;
  }
  if (!buf->StatMask & STAT_PLAYING) {
    cur += sprintf (cur, "%sPaused", sep);
    sep = comma;
  }
  if (buf->FlushPending) {
    cur += sprintf (cur, "%sFlushing", sep);
    sep = comma;
  }
  if (buf->StatMask & STAT_INACTIVE) {
    cur += sprintf (cur, "%sInactive", sep);
    sep = comma;
  }
  if (cur != strbuf)
    cur += sprintf (cur, ")");
}

void SetBuffersStats ()
{
  char strbuf[80];
  Stat_t *stats = Options.statOpts.stats;

  memset (strbuf, 0, 80);
  if (Options.inputOpts.buffer) {
    SetBufferStats (Options.inputOpts.buffer, strbuf);
    stats[6].arg.doublearg = (double) buffer_full(Options.inputOpts.buffer) / (double) Options.inputOpts.buffer->size * 100.0f;
  }
  if (stats[7].arg.stringarg)
    free (stats[7].arg.stringarg);
  stats[7].arg.stringarg = strdup (strbuf);

  memset (strbuf, 0, 80);
  if (Options.outputOpts.buffer) {
    SetBufferStats (Options.outputOpts.buffer, strbuf);
    stats[8].arg.doublearg = (double) buffer_full(Options.outputOpts.buffer) / (double) Options.outputOpts.buffer->size * 100.0f;
  }
  if (stats[9].arg.stringarg)
    free (stats[9].arg.stringarg);
  stats[9].arg.stringarg = strdup (strbuf);
}

/* /status interface */

/* buffer interface */
size_t OutBufferWrite(void *ptr, size_t size, size_t nmemb, void *arg, char iseos)
{
  static ogg_int64_t cursample = 0;
  
  SetTime (Options.statOpts.stats, cursample);
  SetBuffersStats ();
  UpdateStats (Options.statOpts.stats);
  cursample += size * nmemb / Options.outputOpts.channels / 2; /* locked to 16-bit */
  
  if (iseos)
    cursample = 0;
  return devices_write (ptr, size, nmemb, Options.outputOpts.devices);
}
/* /buffer interface */


void usage(void)
{
  FILE *o = stderr;
  int i, driver_count;
  ao_info **devices = ao_driver_info_list(&driver_count);
  
  fprintf(o,
	  "Ogg123 from " PACKAGE " " VERSION "\n"
	  " by Kenneth Arnold <kcarnold@arnoldnet.net> and others\n\n"
	  "Usage: ogg123 [<options>] <input file> ...\n\n"
	  "  -h, --help     this help\n"
	  "  -V, --version  display Ogg123 version\n"
	  "  -d, --device=d uses 'd' as an output device\n"
	  "      Possible devices are:\n"
	  "        ");
  
  for(i = 0; i < driver_count; i++)
    fprintf(o,"%s ",devices[i]->short_name);
  
  fprintf(o,"\n");
  
  fprintf(o,
	  "  -f, --file=filename  Set the output filename for a previously\n"
	  "      specified file device (with -d).\n"
	  "  -k n, --skip n  Skip the first 'n' seconds\n"
	  "  -o, --device-option=k:v passes special option k with value\n"
	  "      v to previously specified device (with -d).  See\n"
	  "      man page for more info.\n"
	  "  -b n, --buffer n  use a buffer of approximately 'n' kilobytes\n"
	  "  -p n, --prebuffer n  prebuffer n%% of the buffer before playing\n"
	  "  -v, --verbose  display progress and other status information\n"
	  "  -q, --quiet    don't display anything (no title)\n"
	  "  -z, --shuffle  shuffle play\n"
	  "\n"
	  "ogg123 will skip to the next song on SIGINT (Ctrl-C) after s seconds after\n"
	  "song start.\n"
	  "  -l, --delay=s  set s (default 1). If s=-1, disable song skip.\n");
}

int main(int argc, char **argv)
{
  int ret;
  int option_index = 1;
  ao_option *temp_options = NULL;
  ao_option ** current_options = &temp_options;
  ao_info *info;
  int temp_driver_id = -1;
  devices_t *current;

  /* data used just to initialize the pointers */
  char char_n = 'n';
  long int_10000 = 10000;
  float float_50f = 50.0f;
  float float_0f = 0.0f;
  long int_1 = 1;
  long int_0 = 0;

  /* *INDENT-OFF* */
  Option_t opts[] = {
    /* found, name, description, type, ptr, default */
    {0, "default_device", "default output device", opt_type_string, &Options.outputOpts.default_device, NULL},
    {0, "shuffle",        "shuffle playlist",      opt_type_char,   &Options.playOpts.shuffle,        &char_n},
    {0, "verbose",        "be verbose",            opt_type_int,    &Options.statOpts.verbose,        &int_0},
    {0, "quiet",          "be quiet",              opt_type_int,    &Options.statOpts.quiet,          &int_0},
    {0, "outbuffer",      "out buffer size (kB)",  opt_type_int,    &Options.outputOpts.BufferSize, &int_0},
    {0, "outprebuffer",   "out prebuffer (%)",     opt_type_float,  &Options.outputOpts.Prebuffer,   &float_0f},
    {0, "inbuffer",       "in buffer size (kB)",   opt_type_int,    &Options.inputOpts.BufferSize, &int_10000},
    {0, "inprebuffer",    "in prebuffer (%)",      opt_type_float,  &Options.inputOpts.Prebuffer, &float_50f},
    {0, "save_stream",    "append stream to file", opt_type_string, &Options.inputOpts.SaveStream, NULL},
    {0, "delay",          "skip file delay (sec)", opt_type_int,    &Options.playOpts.delay,          &int_1},
    {0, NULL,             NULL,                    0,               NULL,                NULL}
  };
  /* *INDENT-ON* */

  Options.playOpts.delay = 1;
  Options.playOpts.nth = 1;
  Options.playOpts.ntimes = 1;

  on_exit (ogg123_onexit, &Options);
  signal (SIGINT, signal_quit);
  ao_initialize();
  
  InitOgg123Stats (Options.statOpts.stats);

  InitOpts(opts);
  ReadStdConfigs (opts);

    while (-1 != (ret = getopt_long(argc, argv, "b:c::d:f:hl:k:o:p:qvVx:y:z",
				    long_options, &option_index))) {
	switch (ret) {
	case 0:
	    fprintf(stderr,
		    "Internal error: long option given when none expected.\n");
	    exit(1);
	case 'b':
	  Options.outputOpts.BufferSize = atoi(optarg);
	  break;
	case 'c':
	  if (optarg)
	    {
	      char *tmp = strdup (optarg);
	      ParseCode pcode = ParseLine (opts, tmp);
	      if (pcode != parse_ok)
		fprintf (stderr,
			 "Error parsing config option from command line.\n"
			 "Error: %s\n"
			 "Option was: %s\n", ParseErr (pcode), optarg);
	      free (tmp);
	    }
	  else {
	    fprintf (stdout, "Available options:\n");
	    DescribeOptions (opts, stdout);
	    exit (0);
	  }
	  break;
	case 'd':
	    temp_driver_id = ao_driver_id(optarg);
	    if (temp_driver_id < 0) {
		fprintf(stderr, "No such device %s.\n", optarg);
		exit(1);
	    }
	    current = append_device(Options.outputOpts.devices, temp_driver_id, 
				    NULL, NULL);
	    if(Options.outputOpts.devices == NULL)
		    Options.outputOpts.devices = current;
	    current_options = &current->options;
	    break;
	case 'f':
	  if (temp_driver_id >= 0) {
	    info = ao_driver_info(temp_driver_id);
	    if (info->type == AO_TYPE_FILE) {
	      free(current->filename);
	      current->filename = strdup(optarg);
	    } else {
	      fprintf(stderr, "Driver %s is not a file output driver.\n",
		      info->short_name);
	      exit(1);
	    }
	  } else {
	    fprintf (stderr, "Cannot specify output file without specifying a driver.\n");
	    exit (1);
	  }
	  break;
	case 'k':
	    Options.playOpts.seekpos = atof(optarg);
	    break;
	case 'l':
	    Options.playOpts.delay = atoi(optarg);
	    break;
	case 'o':
	    if (optarg && !add_option(current_options, optarg)) {
		fprintf(stderr, "Incorrect option format: %s.\n", optarg);
		exit(1);
	    }
	    break;
	case 'h':
	    usage();
	    exit(0);
	case 'p':
	  Options.outputOpts.Prebuffer = atof (optarg);
	  if (Options.outputOpts.Prebuffer < 0.0f || Options.outputOpts.Prebuffer > 100.0f)
	    {
	      fprintf (stderr, "Prebuffer value invalid. Range is 0-100, using nearest value.\n");
	      Options.outputOpts.Prebuffer = Options.outputOpts.Prebuffer < 0.0f ? 0.0f : 100.0f;
	    }
	  break;
	case 'q':
	    Options.statOpts.quiet++;
	    break;
	case 'v':
	    Options.statOpts.verbose++;
	    break;
	case 'V':
	    fprintf(stderr, "Ogg123 from " PACKAGE " " VERSION "\n");
	    exit(0);
	case 'x':
	  Options.playOpts.nth = atoi (optarg);
	  break;
	case 'y':
	  Options.playOpts.ntimes = atoi (optarg);
	  break;
	case 'z':
	    Options.playOpts.shuffle = 1;
	    break;
	case '?':
	    break;
	default:
	    usage();
	    exit(1);
	}
    }

    if (optind == argc) {
	usage();
	exit(1);
    }

    SetPriority (Options.statOpts.verbose);

    /* Add last device to device list or use the default device */
    if (temp_driver_id < 0) {
      if (Options.outputOpts.default_device) {
	temp_driver_id = ao_driver_id (Options.outputOpts.default_device);
	if (temp_driver_id < 0 && Options.statOpts.quiet < 2)
	  fprintf (stderr, "Warning: driver %s specified in configuration file invalid.\n", Options.outputOpts.default_device);
      }
      
      if (temp_driver_id < 0) {
	temp_driver_id = ao_default_driver_id();
      }
      
      if (temp_driver_id < 0) {
	fprintf(stderr,
		"Could not load default driver and no driver specified in config file. Exiting.\n");
	exit(1);
      }

      Options.outputOpts.devices = append_device(Options.outputOpts.devices, temp_driver_id, 
						 temp_options, NULL);
    }

    Options.inputOpts.BufferSize *= 1024;

    if (Options.outputOpts.BufferSize)
      {
	Options.outputOpts.BufferSize *= 1024;
	Options.outputOpts.Prebuffer = (Options.outputOpts.Prebuffer * (float) Options.outputOpts.BufferSize / 100.0f);
	Options.outputOpts.buffer = StartBuffer (Options.outputOpts.BufferSize, (int) Options.outputOpts.Prebuffer,
					     NULL, (pWriteFunc) OutBufferWrite,
					     NULL, NULL, 4096);
	Options.statOpts.stats[8].enabled = 1;
	Options.statOpts.stats[9].enabled = 1;
      }
    
    if (Options.playOpts.shuffle == 'n' || Options.playOpts.shuffle == 'N')
      Options.playOpts.shuffle = 0;
    else if (Options.playOpts.shuffle == 'y' || Options.playOpts.shuffle == 'Y')
      Options.playOpts.shuffle = 1;

    if (Options.playOpts.shuffle) {
	int i;
	int range = argc - optind;
	
	srandom(time(NULL));

	for (i = optind; i < argc; i++) {
	  int j = optind + random() % range;
	  char *temp = argv[i];
	  argv[i] = argv[j];
	  argv[j] = temp;
	}
    }

    while (optind < argc) {
	Options.playOpts.read_file = argv[optind];
	play_file();
	optind++;
    }

    if (Options.outputOpts.buffer != NULL) {
      buffer_WaitForEmpty (Options.outputOpts.buffer);
    }
    
    exit (0);
}

/* Two signal handlers, one for SIGINT, and the second for
 * SIGALRM.  They are de/activated on an as-needed basis by the
 * player to allow the user to stop ogg123 or skip songs.
 */

void signal_skipfile(int which_signal)
{
  skipfile_requested = 1;

  /* libao, when writing wav's, traps SIGINT so it correctly
   * closes things down in the event of an interrupt.  We
   * honour this.   libao will re-raise SIGINT once it cleans
   * up properly, causing the application to exit.  This is 
   * desired since we would otherwise re-open output.wav 
   * and blow away existing "output.wav" file.
   */

  signal (SIGINT, signal_quit);
}

void signal_activate_skipfile(int ignored)
{
  signal(SIGINT,signal_skipfile);
}

void signal_quit(int ignored)
{
  exit_requested = 1;
  if (Options.outputOpts.buffer)
    buffer_flush (Options.outputOpts.buffer);
}

#if 0
/* from vorbisfile.c */
static int _fseek64_wrap(FILE *f,ogg_int64_t off,int whence)
{
  if(f==NULL)return(-1);
  return fseek(f,(int)off,whence);
}
#endif

void play_file()
{
  OggVorbis_File vf;
  int current_section = -1, eof = 0, eos = 0, ret;
  int old_section = -1;
  int is_big_endian = ao_is_big_endian();
  double realseekpos = Options.playOpts.seekpos;
  int nthc = 0, ntimesc = 0;
  int tmp;
  ov_callbacks VorbisfileCallbacks;
  
  tmp = strchr(Options.playOpts.read_file, ':') - Options.playOpts.read_file;
  if (tmp < 10 && tmp + 2 < strlen(Options.playOpts.read_file) && !strncmp(Options.playOpts.read_file + tmp, "://", 3))
    {
      /* let's call this a URL. */
      if (Options.statOpts.quiet < 1)
	fprintf (stderr, "Playing from stream %s\n", Options.playOpts.read_file);
      VorbisfileCallbacks.read_func = StreamBufferRead;
      VorbisfileCallbacks.seek_func = StreamBufferSeek;
      VorbisfileCallbacks.close_func = StreamBufferClose;
      VorbisfileCallbacks.tell_func = StreamBufferTell;
      
      Options.inputOpts.URL = Options.playOpts.read_file;
      Options.inputOpts.buffer = InitStream (Options.inputOpts);
      if ((ov_open_callbacks (Options.inputOpts.buffer->data, &vf, NULL, 0, VorbisfileCallbacks)) < 0) {
	fprintf(stderr, "Error: input not an Ogg Vorbis audio stream.\n");
	return;
      }
      Options.statOpts.stats[6].enabled = 1;
      Options.statOpts.stats[7].enabled = 1;
    }
  else
    {
      FILE *InStream;
#if 0
      VorbisfileCallbacks.read_func = fread;
      VorbisfileCallbacks.seek_func = _fseek64_wrap;
      VorbisfileCallbacks.close_func = fclose;
      VorbisfileCallbacks.tell_func = ftell;
#endif
      if (strcmp(Options.playOpts.read_file, "-"))
	{
	  if (Options.statOpts.quiet < 1)
	    fprintf(stderr, "Playing from file %s.\n", Options.playOpts.read_file);
	  /* Open the file. */
	  if ((InStream = fopen(Options.playOpts.read_file, "rb")) == NULL) {
	    fprintf(stderr, "Error opening input file.\n");
	    exit(1);
	  }
	}
      else
	{
	  if (Options.statOpts.quiet < 1)
	    fprintf(stderr, "Playing from standard input.\n");
	  InStream = stdin;
	}
      if ((ov_open (InStream, &vf, NULL, 0)) < 0) {
	fprintf(stderr, "Error: input not an Ogg Vorbis audio stream.\n");
	return;
      }
    }
    
    /* Setup so that pressing ^C in the first second of playback
     * interrupts the program, but after the first second, skips
     * the song.  This functionality is similar to mpg123's abilities. */
    
    if (Options.playOpts.delay > 0) {
      skipfile_requested = 0;
      signal(SIGALRM,signal_activate_skipfile);
      alarm(Options.playOpts.delay);
    }
    
    exit_requested = 0;
    
    while (!eof && !exit_requested) {
      int i;
      vorbis_comment *vc = ov_comment(&vf, -1);
      vorbis_info *vi = ov_info(&vf, -1);
      
      Options.outputOpts.rate = vi->rate;
      Options.outputOpts.channels = vi->channels;
      if(open_audio_devices() < 0)
	exit(1);
      
      if (Options.statOpts.quiet < 1) {
	if (eos && Options.statOpts.verbose) fprintf (stderr, "\r                                                                          \r\n");
	for (i = 0; i < vc->comments; i++) {
	  char *cc = vc->user_comments[i];	/* current comment */
	  int i;
	  
	  for (i = 0; ogg_comment_keys[i].key != NULL; i++)
	    if (!strncasecmp
		(ogg_comment_keys[i].key, cc,
		 strlen(ogg_comment_keys[i].key))) {
	      fprintf(stderr, ogg_comment_keys[i].formatstr,
		      cc + strlen(ogg_comment_keys[i].key));
	      break;
	    }
	  if (ogg_comment_keys[i].key == NULL)
	    fprintf(stderr, "Unrecognized comment: '%s'\n", cc);
	}
	
	fprintf(stderr, "\nBitstream is %d channel, %ldHz\n",
		vi->channels, vi->rate);
	if (Options.statOpts.verbose > 1)
	  fprintf(stderr, "Encoded by: %s\n\n", vc->vendor);
      }
      
      if (ov_seekable (&vf)) {
	if ((realseekpos > ov_time_total(&vf, -1)) || (realseekpos < 0))
	  /* If we're out of range set it to right before the end. If we set it
	   * right to the end when we seek it will go to the beginning of the song */
	  realseekpos = ov_time_total(&vf, -1) - 0.01;
	
	Options.inputOpts.seekable = 1;
	Options.inputOpts.totalTime = ov_time_total(&vf, -1);
	Options.inputOpts.totalSamples = ov_pcm_total(&vf, -1);
	Options.statOpts.stats[2].enabled = 1;
	Options.statOpts.stats[3].enabled = 1;
	if (Options.statOpts.verbose > 0)
	  SetTime (Options.statOpts.stats, realseekpos / vi->rate);
	
	if (realseekpos > 0)
	  ov_time_seek(&vf, realseekpos);
      }
      else
	Options.inputOpts.seekable = 0;

      eos = 0;
      
      while (!eos && !exit_requested) {
	
	if (skipfile_requested) {
	  eof = eos = 1;
	  skipfile_requested = 0;
	  signal(SIGALRM,signal_activate_skipfile);
	  alarm(Options.playOpts.delay);
	  if (Options.outputOpts.buffer)
	    buffer_flush (Options.outputOpts.buffer);
	  break;
	}
	
	old_section = current_section;
	ret =
	  ov_read(&vf, convbuffer, sizeof(convbuffer), is_big_endian,
		  2, 1, &current_section);
	if (ret == 0) {
	  /* End of file */
	  eof = eos = 1;
	} else if (ret == OV_HOLE) {
	  if (Options.statOpts.verbose > 1) 
	    /* we should be able to resync silently; if not there are 
	       bigger problems. */
	    fprintf (stderr, "Warning: hole in the stream; probably harmless\n");
	} else if (ret < 0) {
	  /* Stream error */
	  fprintf(stderr, "Error: libvorbis reported a stream error.\n");
	} else {
	  /* did we enter a new logical bitstream */
	  if (old_section != current_section && old_section != -1)
	    eos = 1;
	  
	  Options.statOpts.stats[4].arg.doublearg = (double) ov_bitrate_instant (&vf) / 1000.0f;

	  do {
	    if (nthc-- == 0) {
	      if (Options.outputOpts.buffer)
		SubmitData (Options.outputOpts.buffer, convbuffer, ret, 1);
	      else
		OutBufferWrite (convbuffer, ret, 1, &Options, 0);
	      nthc = Options.playOpts.nth - 1;
	    }
	  } while (++ntimesc < Options.playOpts.ntimes);
	  ntimesc = 0;
	  
#if 0
	  /* old status code */
	  if (ov_seekable (&vf)) {
	    u_pos = ov_time_tell(&vf);
	    c_min = (long) u_pos / (long) 60;
	    c_sec = u_pos - 60 * c_min;
	    r_min = (long) (u_time - u_pos) / (long) 60;
	    r_sec = (u_time - u_pos) - 60 * r_min;
	    if (Options.outputOpts.buffer)
	      fprintf(stderr,
		      "\rTime: %02li:%05.2f [%02li:%05.2f] of %02li:%05.2f, Bitrate: %5.1f, Buffer fill: %3.0f%%   \r",
		      c_min, c_sec, r_min, r_sec, t_min, t_sec,
		      (double) ov_bitrate_instant(&vf) / 1000.0F,
		      (double) buffer_full(Options.outputOpts.buffer) / (double) Options.outputOpts.buffer->size * 100.0F);
	    else
	      fprintf(stderr,
		      "\rTime: %02li:%05.2f [%02li:%05.2f] of %02li:%05.2f, Bitrate: %5.1f   \r",
		      c_min, c_sec, r_min, r_sec, t_min, t_sec,
		      (double) ov_bitrate_instant(&vf) / 1000.0F);
	  } else {
	    /* working around a bug in vorbisfile */
	    u_pos = (double) ov_pcm_tell(&vf) / (double) vi->rate;
	    c_min = (long) u_pos / (long) 60;
	    c_sec = u_pos - 60 * c_min;
	    if (Options.outputOpts.buffer)
	      fprintf(stderr,
		      "\rTime: %02li:%05.2f, Bitrate: %5.1f, Buffer fill: %3.0f%%   \r",
		      c_min, c_sec,
		      (float) ov_bitrate_instant (&vf) / 1000.0F,
		      (double) buffer_full(Options.outputOpts.buffer) / (double) Options.outputOpts.buffer->size * 100.0F);
	    else
	      fprintf(stderr,
		      "\rTime: %02li:%05.2f, Bitrate: %5.1f   \r",
		      c_min, c_sec,
		      (float) ov_bitrate_instant (&vf) / 1000.0F);
	  }
#endif
	}
      }
    }
    
    alarm(0);
    signal(SIGALRM,SIG_DFL);
    signal(SIGINT,signal_quit);
    
    ov_clear(&vf);
    
    if (Options.outputOpts.buffer) {
      buffer_MarkEOS (Options.outputOpts.buffer);
      buffer_WaitForEmpty (Options.outputOpts.buffer);
    }

    if (Options.statOpts.quiet < 1)
      fprintf(stderr, "\nDone.\n");
    
    if (exit_requested)
      exit (0);
}

int open_audio_devices()
{
  static int prevrate=0, prevchan=0;
  devices_t *current;
  ao_sample_format format;

  if(prevrate == Options.outputOpts.rate && prevchan == Options.outputOpts.channels)
    return 0;
  
  if(prevrate !=0 && prevchan!=0)
	{
	  if (Options.outputOpts.buffer)
	    buffer_WaitForEmpty (Options.outputOpts.buffer);

	  close_audio_devices (Options.outputOpts.devices);
	}
  
  format.rate = prevrate = Options.outputOpts.rate;
  format.channels = prevchan = Options.outputOpts.channels;
  format.bits = 16;
  format.byte_format = AO_FMT_NATIVE;

  current = Options.outputOpts.devices;
  while (current != NULL) {
    ao_info *info = ao_driver_info(current->driver_id);
    
    if (Options.statOpts.verbose > 0) {
      fprintf(stderr, "Device:   %s\n", info->name);
      fprintf(stderr, "Author:   %s\n", info->author);
      fprintf(stderr, "Comments: %s\n", info->comment);
      fprintf(stderr, "\n");	
    }
    
    if (current->filename == NULL)
      current->device = ao_open_live(current->driver_id, &format,
				     current->options);
    else
      current->device = ao_open_file(current->driver_id, current->filename,
				     0, &format, current->options);

    if (current->device == NULL) {
      switch (errno) {
      case AO_ENODRIVER:
	fprintf(stderr, "Error: Device not available.\n");
	break;
      case AO_ENOTLIVE:
	fprintf(stderr, "Error: %s requires an output filename to be specified with -f.\n", info->short_name);
	break;
      case AO_EBADOPTION:
	fprintf(stderr, "Error: Unsupported option value to %s device.\n",
		info->short_name);
	break;
      case AO_EOPENDEVICE:
	fprintf(stderr, "Error: Cannot open device %s.\n",
		info->short_name);
	break;
      case AO_EFAIL:
	fprintf(stderr, "Error: Device failure.\n");
	break;
      case AO_ENOTFILE:
	fprintf(stderr, "Error: An output file cannot be given for %s device.\n", info->short_name);
	break;
      case AO_EOPENFILE:
	fprintf(stderr, "Error: Cannot open file %s for writing.\n",
		current->filename);
	break;
      case AO_EFILEEXISTS:
	fprintf(stderr, "Error: File %s already exists.\n", current->filename);
	break;
      default:
	fprintf(stderr, "Error: This error should never happen.  Panic!\n");
	break;
      }
	
      return -1;
    }
    current = current->next_device;
  }
  
  return 0;
}

void ogg123_onexit (int exitcode, void *arg)
{
  if (Options.inputOpts.buffer) {
    StreamInputCleanup (Options.inputOpts.buffer);
    Options.inputOpts.buffer = NULL;
  }
      
  if (Options.outputOpts.buffer) {
    buffer_cleanup (Options.outputOpts.buffer);
    Options.outputOpts.buffer = NULL;
  }

  ao_onexit (exitcode, Options.outputOpts.devices);
}
