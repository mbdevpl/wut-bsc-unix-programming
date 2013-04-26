#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#define ERR(source) (perror(source),\
		     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))

struct req{
  int id;
  char c;
};

typedef struct{
  int id;
  int * request;
  char * str;
  char * s;
  struct req *requestS;
  int *idlethreads;
  pthread_cond_t *cond_request;
  pthread_mutex_t *mutex_request;
  int * done;
  pthread_cond_t *cond_s;
  pthread_mutex_t *mutex_s;
} thread_arg;

void freeWords(int argc,
	       char ** words){
  int i;
  for(i = 0; i < argc - 1; i += 1)
    free(words[i]);
}

void joinThreads(int argc,
		 pthread_t * thread,
		 pthread_mutex_t * mutex_s,
		 pthread_cond_t * cond_s){
  int i;

  for(i = 0; i < argc - 1; i += 1){
    if(pthread_join(thread[i], NULL) != 0)
      ERR("pthread_join");
    if(pthread_mutex_destroy(&mutex_s[i]) != 0)
      ERR("pthread_mutex_destroy");
    if(pthread_cond_destroy(&cond_s[i]) != 0)
      ERR("pthread_cond_destroy");
  }
}

char convert(char c){
  if(c >= 65 && c <= 90)
    c += 32;
  else if(c >= 97 && c <= 122)
    c -= 32;
  return c;
}

void usage(char *name){
  fprintf(stderr, "USAGE: %s strings\n",name);
  exit(EXIT_FAILURE);
}

void *threadfunc(void *arg){
  thread_arg targ;
  int len = 0;

  memcpy(&targ, arg, sizeof(thread_arg));
  len = strlen(targ.str);

  while(len-- >= 0){
    if(pthread_mutex_lock(targ.mutex_request) != 0)
      ERR("pthread_mutex_lock mutex_request");

    while(*targ.request != 0)
      if(pthread_cond_wait(targ.cond_request, targ.mutex_request) != 0)
	ERR("pthread_cond_wait");

    *targ.request = 1;
    targ.requestS->id = targ.id;
    targ.requestS->c = targ.str[len];

    if(pthread_mutex_unlock(targ.mutex_request) != 0)
      ERR("pthread_mutex_unlock");
    if(pthread_cond_broadcast(targ.cond_request) != 0)
      ERR("pthread_cond_signal");

    if(pthread_mutex_lock(targ.mutex_s) != 0)
      ERR("pthread_mutex_lock");
    while(*targ.done == 0)
      if(pthread_cond_wait(targ.cond_s, targ.mutex_s) != 0)
	ERR("pthread_cond_wait");
    targ.str[len] = *targ.s;
    *targ.s = 0;
    *targ.done = 0;
    if(pthread_mutex_unlock(targ.mutex_s) != 0)
      ERR("pthread_mutex_unlock");;
  }

  if(pthread_mutex_lock(targ.mutex_request) != 0)
    ERR("pthread_mutex_lock");
  *targ.idlethreads += 1;
  if(pthread_mutex_unlock(targ.mutex_request) != 0)
    ERR("pthread_mutex_unlock");
  if(pthread_cond_signal(targ.cond_request) != 0)
    ERR("pthread_cond_signal");

  return NULL;
}

int main(int argc, char ** argv){
  int i, idlethreads = 0;
  char s[argc - 1];
  char *words[argc - 1];
  int done[argc - 1];
  pthread_t thread[argc - 1];
  thread_arg targ[argc - 1];

  pthread_cond_t cond_request = PTHREAD_COND_INITIALIZER;
  pthread_mutex_t mutex_request = PTHREAD_MUTEX_INITIALIZER;
  struct req * requestS = (struct req*)malloc(sizeof(struct req));
  requestS->id = -1;
  requestS->c = 0;
  int request = -1;

  pthread_cond_t cond_s[argc - 1];
  pthread_mutex_t mutex_s[argc - 1];

  if(argc < 2)
    usage(argv[0]);
  for(i = 0; i < argc - 1; i += 1){
    words[i] = (char*)malloc((strlen(argv[i + 1]) * sizeof(char)));
    strcpy(words[i], argv[i + 1]);

    if(pthread_mutex_init(&mutex_s[i], NULL) != 0)
      ERR("pthread_mutex_init");
    if(pthread_cond_init(&cond_s[i], NULL) != 0)
      ERR("pthread_cond_init");

    done[i] = 0;
    s[i] = 0;

    targ[i].id = i;
    targ[i].str = words[i];
    targ[i].s = &s[i];
    targ[i].done = &done[i];
    targ[i].requestS = requestS;
    targ[i].request = &request;
    targ[i].cond_request = &cond_request;
    targ[i].mutex_request = &mutex_request;
    targ[i].idlethreads = &idlethreads;
    targ[i].cond_s = &cond_s[i];
    targ[i].mutex_s = &mutex_s[i];

    if(pthread_create(&thread[i], NULL, threadfunc, (void *)&targ[i]) != 0)
      ERR("pthread_create");
  }

  while(1){
    if(pthread_mutex_lock(&mutex_request) != 0)
      ERR("pthread_mutex_lock mutex_request main");

    request = 0;

    if(pthread_mutex_unlock(&mutex_request) != 0)
      ERR("pthread_mutex_unlock");

    if(pthread_cond_signal(&cond_request) != 0)
      ERR("pthread_cond_signal");

    if(pthread_mutex_lock(&mutex_request) != 0)
      ERR("pthread_mutex_lock");

    while(request == 0 && idlethreads != argc - 1)
      if(pthread_cond_wait(&cond_request, &mutex_request) != 0)
	ERR("pthread_mutex_wait request");

    if(idlethreads == argc - 1){ 
      if(pthread_mutex_unlock(&mutex_request) != 0)
	ERR("pthread_mutex_unlock mutex_request");
      break;
    }

    if(pthread_mutex_lock(&mutex_s[requestS->id]) != 0)
      ERR("pthread_mutex_lock mutex_s");

    s[requestS->id] = convert(requestS->c);
    done[requestS->id] = 1;
    
    if(pthread_mutex_unlock(&mutex_s[requestS->id]) != 0)
       ERR("pthread_mutex_unlock mutex_s");
    if(pthread_cond_signal(&cond_s[requestS->id]) != 0)
      ERR("pthread_cond_signal");

    request = -1;

    if(pthread_mutex_unlock(&mutex_request) != 0)
      ERR("pthread_mutex_unlock mutex_request");
  }
  
  joinThreads(argc, thread, mutex_s, cond_s);

  if(pthread_mutex_destroy(&mutex_request) != 0)
    ERR("pthread_mutex_destroy");
  if(pthread_cond_destroy(&cond_request) != 0)
    ERR("pthread_cond_destroy");

  for(i = 0; i < argc - 1; i += 1)
    printf("%s\n", words[i]);

  freeWords(argc, words);
  free(requestS);
  return EXIT_SUCCESS;
}
