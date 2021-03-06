#if !defined(_AIX)
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <asm/unistd.h>
#include "../drd.h"


static int s_debug = 0;


static int getktid()
{
#ifdef __NR_gettid
  return syscall(__NR_gettid);
#else
  return -1;
#endif
}

static void print_thread_id(const char* const label)
{
  if (s_debug)
  {
    char msg[256];
    snprintf(msg, sizeof(msg),
             "%spid %d / kernel thread ID %d / Valgrind thread ID %d\n",
             label, getpid(), getktid(), vg_get_valgrind_threadid());
    write(STDOUT_FILENO, msg, strlen(msg));
  }
}

static void SignalHandler(const int iSignal)
{
  print_thread_id("Signal was delivered to ");
}

void* thread_func(void* thread_arg)
{
  print_thread_id("thread: ");

  sleep(10);
  //assert(result < 0 && errno == EINTR);

  return 0;
}

int main(int argc, char** argv)
{
  int vgthreadid;
  pthread_t threadid;
  struct timespec tsDelay;

  // Primitive argument parsing.
  if (argc > 1)
    s_debug = 1;

  vgthreadid = vg_get_valgrind_threadid();

  print_thread_id("main: ");

  {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &SignalHandler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, 0);
  }

  pthread_create(&threadid, 0, thread_func, 0);
  // Wait until the thread is inside clock_nanosleep().
  tsDelay.tv_sec = 0;
  tsDelay.tv_nsec = 20 * 1000 * 1000;
  nanosleep(&tsDelay, 0);
  // And send SIGALRM to the thread.
  pthread_kill(threadid, SIGALRM);
  pthread_join(threadid, 0);

  return 0;
}

#else /* !defined(_AIX) */
#include <stdio.h>
int main ( void ) {
  fprintf(stderr, "This test does not compile on AIX5.\n");
  return 0;
}
#endif /* !defined(_AIX) */
