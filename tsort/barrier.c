// Author: Nat Tuck
// CS3650 starter code

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>

#include "barrier.h"

barrier*
make_barrier(int nn)
{
    int rv;
    barrier* bb = mmap(0, sizeof(barrier), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if ((long) bb == -1) {
        perror("mmap(barrier)");
        abort();
    }

    rv = pthread_mutex_init(&(bb->barrier), NULL);
    if (rv == -1) {
        perror("mutex_init(barrier)");
        abort();
    }

    rv = pthread_mutex_init(&(bb->mutex), NULL);
    if (rv == -1) {
        perror("mutex_init(mutex)");
        abort();
    }
 
    pthread_cond_init(&(bb->condv),NULL);

    bb->count = nn;
    bb->seen  = nn;
    return bb;
}

void
barrier_wait(barrier* bb)
{
    int rv;

    rv = pthread_mutex_lock(&(bb->mutex));
    if (rv == -1) {
        perror("lock(mutex)");
        abort();
    }

    bb->seen--;
    int seen = bb->seen;

    rv = pthread_mutex_unlock(&(bb->mutex));
    if (rv == -1) {
        perror("unlock(mutex)");
        abort();
    }

    pthread_mutex_lock(&(bb->barrier));
    //bb->seen -= 1;
    //int seen = bb->seen;
    if(seen == 0)
  	pthread_cond_broadcast(&(bb->condv));
    else{
    	//while(seen != 0)
		pthread_cond_wait(&(bb->condv),&(bb->barrier));
    }

    rv = pthread_mutex_unlock(&(bb->barrier));
    if (rv == -1) {
        perror("wait(barrier)");
        abort();
    }
}

void
free_barrier(barrier* bb)
{
    int rv = munmap(bb, sizeof(barrier));
    if (rv == -1) {
        perror("munmap");
        abort();
    }
}

