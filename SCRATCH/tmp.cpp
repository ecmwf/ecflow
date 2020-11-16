#include <pthread.h>
#include <stdio.h>
#include <string.h>

/* function to be run as a thread always must have the same signature:
   it has one void* parameter and returns void */
void *threadfunction(void *arg)
{
  printf("Hello, World!\n"); /*printf() is specified as thread-safe as of C11*/
  return 0;
}

int main(void)
{
  pthread_t thread;
  int createerror = pthread_create(&thread, NULL, threadfunction, NULL);
  /*creates a new thread with default attributes and NULL passed as the argument to the start routine*/
  if (!createerror) /*check whether the thread creation was successful*/
    {
      pthread_join(thread, NULL); /*wait until the created thread terminates*/
      return 0;
    }
  fprintf("%s\n", strerror(createerror), stderr);
  return 1;
}
