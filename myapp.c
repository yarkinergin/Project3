#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdarg.h>
#include "rm.h"

#define NUMR 6        // number of resource types
#define NUMP 4        // number of threads

int AVOID = 1;
int exist[6] =  {5, 2, 4, 7, 3, 5};  // resources existing in the system

void pr (int tid, char astr[], int m, int r[])
{
    int i;
    printf ("thread %d, %s, [", tid, astr);
    for (i=0; i<m; ++i) {
        if (i==(m-1))
            printf ("%d", r[i]);
        else
            printf ("%d,", r[i]);
    }
    printf ("]\n");
}


void setarray (int r[MAXR], int m, ...)
{
    va_list valist;
    int i;
    
    va_start (valist, m);
    for (i = 0; i < m; i++) {
        r[i] = va_arg(valist, int);
    }
    va_end(valist);
    return;
}

void *threadfunc1 (void *a)
{
    int tid;
    int request1[MAXR];
    int request2[MAXR];
    int claim[MAXR];

    tid = *((int*)a);
    rm_thread_started (tid);

    setarray(claim, NUMR, 3, 2, 4, 5, 3, 3);
    rm_claim (claim);

    setarray(request1, NUMR, 2, 2, 2, 2, 1, 1);
    pr (tid, "REQ", NUMR, request1);
    rm_request (request1);

    sleep(8);

    setarray(request2, NUMR, 1, 0, 2, 2, 1, 2);
    pr (tid, "REQ", NUMR, request2);
    rm_request (request2);

    rm_release (request1);
    rm_release (request2);

    rm_thread_ended();
    pthread_exit(NULL);
}

void *threadfunc2 (void *a)
{
    int tid;
    int request1[MAXR];
    int request2[MAXR];
    int request3[MAXR];
    int claim[MAXR];

    tid = *((int*)a);
    rm_thread_started (tid);

    setarray(claim, NUMR, 5, 2, 4, 7, 3, 3);
    rm_claim (claim);

    setarray(request1, NUMR, 2, 1, 2, 3, 1, 0);
    pr (tid, "REQ", NUMR, request1);
    rm_request (request1);

    sleep(6);

    setarray(request2, NUMR, 1, 0, 2, 3, 1, 2);
    pr (tid, "REQ", NUMR, request2);
    rm_request (request2);

    sleep(6);

    setarray(request3, NUMR, 2, 1, 0, 1, 1, 0);
    pr (tid, "REQ", NUMR, request3);
    rm_request (request3);

    rm_release (request1);
    rm_release (request2);
    rm_release (request3);

    rm_thread_ended();
    pthread_exit(NULL);
}

void *threadfunc3 (void *a)
{
    int tid;
    int request1[MAXR];
    int request2[MAXR];
    int claim[MAXR];

    tid = *((int*)a);
    rm_thread_started (tid);

    setarray(claim, NUMR, 4, 2, 4, 4, 3, 2);
    rm_claim (claim);

    setarray(request1, NUMR, 3, 1, 2, 2, 1, 2);
    pr (tid, "REQ", NUMR, request1);
    rm_request (request1);

    sleep(3);

    setarray(request2, NUMR, 1, 0, 0, 1, 2, 0);
    pr (tid, "REQ", NUMR, request2);
    rm_request (request2);

    rm_release (request1);
    rm_release (request2);

    rm_thread_ended();
    pthread_exit(NULL);
}

void *threadfunc4 (void *a)
{
    int tid;
    int request1[MAXR];
    int request2[MAXR];
    int claim[MAXR];

    tid = *((int*)a);
    rm_thread_started (tid);

    setarray(claim, NUMR, 5, 2, 4, 7, 3, 5);
    rm_claim (claim);

    setarray(request1, NUMR, 2, 1, 2, 3, 2, 2);
    pr (tid, "REQ", NUMR, request1);
    rm_request (request1);

    sleep(10);

    setarray(request2, NUMR, 3, 1, 2, 2, 0, 1);
    pr (tid, "REQ", NUMR, request2);
    rm_request (request2);

    rm_release (request1);
    rm_release (request2);

    rm_thread_ended();
    pthread_exit(NULL);
}

int main(int argc, char **argv)
{
    int i;
    int tids[NUMP];
    pthread_t threadArray[NUMP];
    int count;
    int ret;

    if (argc != 2) {
        printf ("usage: ./app avoidflag\n");
        exit (1);
    }

    AVOID = atoi (argv[1]);
    
    if (AVOID == 1)
        rm_init (NUMP, NUMR, exist, 1);
    else
        rm_init (NUMP, NUMR, exist, 0);

    i = 0;
    tids[i] = i;
    pthread_create (&(threadArray[i]), NULL,
                    (void *) threadfunc1, (void *)
                    (void*)&tids[i]);
    
    i = 1;
    tids[i] = i;
    pthread_create (&(threadArray[i]), NULL,
                    (void *) threadfunc2, (void *)
                    (void*)&tids[i]);

    i = 2;
    tids[i] = i;
    pthread_create (&(threadArray[i]), NULL,
                    (void *) threadfunc3, (void *)
                    (void*)&tids[i]);
    
    i = 3;
    tids[i] = i;
    pthread_create (&(threadArray[i]), NULL,
                    (void *) threadfunc4, (void *)
                    (void*)&tids[i]);

    count = 0;
    while ( count < 20) {
        sleep(2);
        rm_print_state("The current state");
        ret = rm_detection();
        if (ret > 0) {
            if(AVOID == 1)
                printf ("deadlock detected, count=%d  (AVOIDING...)\n", ret);
            else
                printf ("deadlock detected, count=%d\n", ret);
            rm_print_state("state after deadlock");
        }
        count++;
    }
    
    if (ret == 0) {
        for (i = 0; i < NUMP; ++i) {
            pthread_join (threadArray[i], NULL);
            printf ("joined\n");
        }
    }                                
}