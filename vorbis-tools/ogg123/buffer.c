/* buffer.c
 *  buffering code for ogg123. This is Unix-specific. Other OSes anyone?
 *
 * Thanks to Lee McLouchlin's buffer(1) for inspiration; no code from
 * that program is used in this buffer.
 */

#include <sys/types.h>
#if HAVE_SMMAP
#include <sys/mman.h>
#else
#include <sys/ipc.h>
#include <sys/shm.h>
#endif
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h> /* for fork and pipe*/
#include <fcntl.h>
#include <signal.h>

#include "ogg123.h"
#include "buffer.h"

#undef DEBUG_BUFFER

#ifdef DEBUG_BUFFER
FILE *debugfile;
#define DEBUG0(x) do { fprintf (debugfile, x "\n" ); } while (0)
#define DEBUG1(x, y) do { fprintf (debugfile, x "\n" , y); } while (0)
#else
#define DEBUG0(x)
#define DEBUG1(x, y)
#endif

void signal_handler (int sig)
{
}

/* Initialize the buffer structure. */
void buffer_init (buf_t *buf, long size, long prebuffer)
{
  DEBUG0("buffer init");
  memset (buf, 0, sizeof(*buf));
  buf->status = 0;
  buf->reader = buf->writer = buf->buffer;
  buf->end = buf->buffer + (size - 1);
  buf->size = size;
  buf->prebuffer = prebuffer;
  buf->curfill = 0;
  if (prebuffer > 0)
    buf->status |= STAT_PREBUFFER;
}

/* Main write loop. No semaphores. No deadlock. No problem. I hope. */
void writer_main (volatile buf_t *buf, devices_t *d)
{
  devices_t *d1;
  signal (SIGINT, SIG_IGN);
  signal (SIGUSR1, signal_handler);

  DEBUG0("r: writer_main");
  while (! (buf->status & STAT_SHUTDOWN && buf->curfill == 0))
    {
      /* Writer just waits on reader to be done with submit_chunk
       * Reader must ensure that we don't deadlock. */

      /* prebuffering */
    prebuffer:
      while (buf->status & STAT_PREBUFFER)
	pause();

      if (buf->curfill == 0) {
	if (! (buf->status & STAT_FLUSH)) /* likely unnecessary */
	  buf->status |= STAT_UNDERFLOW;
	DEBUG0("alerting writer");
	kill (buf->readerpid, SIGUSR1);
      }

      if (buf->status & STAT_FLUSH) {
      flush:
	DEBUG0("r: buffer flush");
	buf->curfill = 0;
	buf->reader = buf->writer;
	buf->status &= ~STAT_FLUSH;
	DEBUG1("buf->status = %d", buf->status);
	kill (buf->readerpid, SIGUSR1);
      }

      if (buf->curfill == 0 && !(buf->status & STAT_SHUTDOWN)
	  && !(buf->status & STAT_FLUSH)) {
	buf->status |= STAT_PREBUFFER;
	goto prebuffer;
      }
	/*	DEBUG1("looping on buffer underflow, status is %d", buf->status);*/

      if (buf->status & STAT_FLUSH)
	goto flush;

      if (buf->reader == buf->writer)
	break;

      /* devices_write (buf->writer->data, buf->writer->len, d); */
      DEBUG0("writing chunk");
      {
	d1 = d;
	while (d1 != NULL) {
	  ao_play(d1->device, buf->writer->data, buf->writer->len);
	  d1 = d1->next_device;
	}
      }

      DEBUG0("incrementing pointer");
      if (buf->writer == buf->end)
	buf->writer = buf->buffer;
      else
	buf->writer++;
      buf->curfill--;
      if (buf->curfill + 1 == buf->size)
	kill (buf->readerpid, SIGUSR1);
   }
  DEBUG0("r: shutting down buffer");
  buf->status = 0;
  write (buf->fds[1], "2", sizeof(int));
  kill (buf->writerpid, SIGUSR1);
  DEBUG0("r: exiting");
  _exit(0);
}

/* fork_writer is called to create the writer process. This creates
 * the shared memory segment of 'size' chunks, and returns a pointer
 * to the buffer structure that is shared. Just pass this straight to
 * submit_chunk and all will be happy. */

buf_t *fork_writer (long size, devices_t *d, long prebuffer)
{
  int childpid;
  buf_t *buf;

#if HAVE_SMMAP
  int fd;

  if ((fd = open("/dev/zero", O_RDWR)) < 0)
    {
      perror ("cannot open /dev/zero");
      exit (1);
    }
  if ((buf = (buf_t *) mmap (0, sizeof(buf_t) + sizeof (chunk_t) * (size - 1),
                             PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED)
    {
      perror("mmap");
      exit(1);
    }
  close(fd);
#else
  /* Get the shared memory segment. */
  int shmid = shmget (IPC_PRIVATE,
			  sizeof(buf_t) + sizeof (chunk_t) * (size - 1),
			  IPC_CREAT|SHM_R|SHM_W);

  if (shmid == -1)
    {
      perror ("shmget");
      exit (1);
    }
  
  /* Attach the segment to us (and our kids). Get and store the pointer. */
  buf = (buf_t *) shmat (shmid, 0, 0);
  
  if (buf == NULL)
    {
      perror ("shmat");
      exit (1);
    }

  /* Remove segment after last process detaches it or terminates. */
  shmctl(shmid, IPC_RMID, 0);
#endif /* HAVE_SMMAP */

#ifdef DEBUG_BUFFER
  debugfile = fopen ("/tmp/bufferdebug", "w"); /* file can be a pipe */
  setvbuf (debugfile, NULL, _IONBF, 0);
#endif

  buffer_init (buf, size, prebuffer);
  
  /* Create a pipe for communication between the two processes. Unlike
   * the first incarnation of an ogg123 buffer, the data is not transferred
   * over this channel, only occasional "WAKE UP!"'s. */

  if (pipe (buf->fds))
    {
      perror ("pipe");
      exit (1);
    }

  /* write should never block; read should always block. */
  fcntl (buf->fds[1], F_SETFL, O_NONBLOCK);

  fflush (stdout);

  signal (SIGUSR1, signal_handler);

  buf->readerpid = getpid();

  childpid = fork();
  
  if (childpid == -1)
    {
      perror ("fork");
      exit (1);
    }

  if (childpid == 0)
    {
      buf->writerpid = getpid();
      writer_main (buf, d);
      return NULL;
    }
  else {
    buf->writerpid = childpid;
    return buf;
  }
}

void submit_chunk (buf_t *buf, chunk_t chunk)
{
  struct timeval tv;
  static fd_set set;

  FD_ZERO(&set);
  FD_SET(buf->fds[0], &set);

  DEBUG0("submit_chunk");
  /* Wait wait, don't step on my sample! */
  while (buf->curfill == buf->size)
    {
      /* buffer overflow (yikes! no actually it's a GOOD thing) */
      int ret;
      char t;
      
      DEBUG0("w: looping on buffer overflow");
      tv.tv_sec = 1;
      tv.tv_usec = 0;
      ret = select (buf->fds[0]+1, &set, NULL, NULL, &tv);
      
      DEBUG1("w: select returned %d", ret);
      if (ret > 0)
	read (buf->fds[0], &t, sizeof(int));
    }
	      
  DEBUG0("writing chunk");
  *(buf->reader) = chunk;
  /* do this atomically */
  if (buf->reader == buf->end)
    buf->reader = buf->buffer;
  else
    buf->reader++;
  buf->curfill++;

  if (buf->status & STAT_PREBUFFER && buffer_full(buf) >= buf->prebuffer) {
    buf->status &= ~STAT_PREBUFFER;
    kill (buf->writerpid, SIGUSR1);
  }
  else if (buf->status & STAT_UNDERFLOW) {
    buf->status |= STAT_PREBUFFER;
    buf->status &= ~STAT_UNDERFLOW;
    kill (buf->writerpid, SIGUSR1);
  }
  DEBUG0("submit_chunk exit");
}

void buffer_flush (buf_t *buf)
{
  DEBUG0("flush buffer");
  buf->status |= STAT_FLUSH;
}

void buffer_shutdown (buf_t *buf)
{
  DEBUG0("shutdown buffer");
  buf->status |= STAT_SHUTDOWN;
  buf->status &= ~STAT_PREBUFFER;
  while (buf->status != 0)
    {
      DEBUG0("waiting on reader to quit");
      pause();
    } 
#ifndef HAVE_SMMAP
  /* Deallocate the shared memory segment. */
  shmdt(buf);
#endif /* HAVE_SMMAP */
  DEBUG0("buffer done.");
}

long buffer_full (buf_t* buf) {
  return buf->curfill;
}

void buffer_cleanup (buf_t *buf) {
  if (buf) {
    if (buf->writerpid)
      kill (buf->writerpid, SIGTERM);
    wait (0);
    buf->writerpid = 0;
#ifndef HAVE_SMMAP
    shmdt(buf);
#endif
  }
}
