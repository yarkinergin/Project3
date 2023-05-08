#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include "rm.h"
#include <stdbool.h>

// global variables

int DA;  // indicates if deadlocks will be avoided or not
int N;   // number of processes
int M;   // number of resource types
int ExistingRes[MAXR]; // Existing resources vector

//..... other definitions/variables .....
//.....
//.....
int AvailableRes[MAXR]; // Available resources
int MaxRes[MAXP][MAXR];
int Allocated[MAXP][MAXR];
int Need[MAXP][MAXR];
int ProcessingThreads[MAXP]; // Threads processing. 1 if processes, 0 if terminated.

int Request[MAXP][MAXR];

pthread_mutex_t lock;
pthread_cond_t cvs[MAXP];


// end of global variables

int rm_thread_started(int tid)
{
    ProcessingThreads[tid] = pthread_self();

    int ret = 0;
    return (ret);
}


int rm_thread_ended()
{
    int selfId = pthread_self(); 
    for (int i = 0; i < N; i++){
        if (ProcessingThreads[i] == selfId){
            ProcessingThreads[i] = 0;
            return 0;
        }
    }

    int ret = -1;
    return (ret);
}


int rm_claim(int claim[])
{
    int tid = -1;
    int selfId = pthread_self(); 

    for (int i = 0; i < N; i++){
        if (ProcessingThreads[i] == selfId)
            tid = i;
    }

    if (DA != 1 || tid < 0){
        return -1;
    }

    for (int i = 0; i < M; i++){
        if (claim[i] > ExistingRes[i])
            return -1;

        if(DA == 1){
            MaxRes[tid][i] = claim[i];
            Need[tid][i] = claim[i];
        }
    }

    int ret = 0;
    return(ret);
}


int rm_init(int p_count, int r_count, int r_exist[],  int avoid)
{
    int i;
    int ret = 0;
    
    if( 0 > p_count || p_count > MAXP || 0 > r_count || r_count > MAXR || avoid < 0 || avoid > 1){
        return -1;
    }

    DA = avoid;
    N = p_count;
    M = r_count;
    // initialize (create) resources
    for (i = 0; i < M; ++i)
        ExistingRes[i] = r_exist[i];
    // resources initialized (created)

    for (i = 0; i < M; ++i)
        AvailableRes[i] = ExistingRes[i];

    for(int i = 0; i < N; i++){
        for( int j = 0; j < M; j++){
            Request[i][j] = 0;
            MaxRes[i][j] = 0;
            Need[i][j] = 0;
        }
    }
    
    return  (ret);
} 


int rm_request (int request[])
{
    pthread_mutex_lock(&lock);
    int ret = 0;
    int tid = -1;
    int selfId = pthread_self(); 

    for (int i = 0; i < N; i++){
        if (ProcessingThreads[i] == selfId)
            tid = i;
    }
    if (tid < 0)
        return -1;
 
    for (int i = 0; i < M; i++)
        Request[tid][i] = request[i]; 

    if(DA == 1){
        int work[M];
        bool finish[N];
        bool workBigger = true;
        bool safe = true;

        for (int i = 0; i < M; i++){
            if (request[i] > Need[tid][i]){
                Request[tid][i] = 0;
                pthread_mutex_unlock(&lock);
                return -1;
            }    
        }

        for (int i = 0; i < M; ++i){
            while(AvailableRes[i] < request[i]){
                pthread_cond_wait(&cvs[tid], &lock);
            }

            Request[tid][i] = 0;

            Allocated[tid][i] += request[i];
            Need[tid][i] -= request[i];
            AvailableRes[i] -= request[i]; 
        }   

        for (int i = 0; i < M; i++)
            work[i] = AvailableRes[i];

        for (int j = 0; j < N; j++)
            finish[j] = false;

        for (int i = 0; i < N; i++){
            if (finish[i])
                continue;

            for (int j = 0; j < M; j++){
                if(Need[i][j] > work[j])
                    workBigger = false;
            }
            if( workBigger){
                for (int j = 0; j < M; j++){
                    work[j] += Allocated[i][j];
                }
                finish[i] = true;
            }
        }
        for (int i = 0; i < N; i++){
            if (!finish[i])
                safe = false;
        }
        if (!safe){
            for (int i = 0; i < M; ++i){
                Allocated[tid][i] -= request[i];
                Need[tid][i] += request[i];
                AvailableRes[i] += request[i];
            }
            pthread_mutex_unlock(&lock);
            return -1;
        }
        pthread_mutex_unlock(&lock);
        return 0;
    }
    else if(DA == 0){
        for (int i = 0; i < M; ++i){
            while(AvailableRes[i] < request[i])
                pthread_cond_wait(&cvs[tid], &lock);
            
            Request[tid][i] = 0;

            Allocated[tid][i] += request[i];
            AvailableRes[i] -= request[i];
        }
    }
    
    pthread_mutex_unlock(&lock);
    return(ret);
}


int rm_release (int release[])
{
    pthread_mutex_lock(&lock);
    int ret = 0;
    int tid = -1;
    int selfId = pthread_self();

    for (int i = 0; i < N; i++){
        if (ProcessingThreads[i] == selfId)
            tid = i;
    }

    if (tid < 0){
        pthread_mutex_unlock(&lock);
        return -1;
    }

    for (int i = 0; i < M; i++){
        if (release[i] > Allocated[tid][i]){
            pthread_mutex_unlock(&lock);
            return -1;
        }
    }

    for (int i = 0; i < M; ++i){        
        Allocated[tid][i] -= release[i];
        AvailableRes[i] += release[i];

        if( DA == 1)
            Need[tid][i] += release[i];
    }

    for (int i = 0; i < N; i++){
        pthread_cond_signal(&cvs[i]);
    }
    pthread_mutex_unlock(&lock);

    return (ret);
}


int rm_detection()
{
    int ret = 0;
    int work[M];
    bool finish[N];
    bool nullReq = true;
    bool workBigger = true;

    for(int i = 0; i < M; i++)
        work[i] = AvailableRes[i];

    for(int i = 0; i < N; i++){
        nullReq = true;
        for(int j = 0; j < M; j++){
            if (Request[i][j] != 0){
                nullReq = false;
            }
        }
        if(nullReq)
            finish[i] = true;
        else
            finish[i] = false;
    }
    for(int i = 0; i < N; i++){
        if(!finish[i]){
            for(int j = 0; j < M; j++){
                if(Request[i][j] > work[j])
                    workBigger = false;
            }
            if(workBigger){
                for(int j = 0; j < M; j++){
                    work[j] += Allocated[i][j];
                }
                finish[i] = true;
            }
        }
    }
    for(int i = 0; i < N; i++){
        if(!finish[i])
            ret += 1;
    }
    
    return (ret);
}


void rm_print_state (char hmsg[])
{
    pthread_mutex_lock(&lock);
    printf("#######################\n");
    printf("%s\n", hmsg);
    printf("#######################\n");
    printf("Exist:\n");


    for(int i = 0; i < M; i++){
        printf("\tR%d", i);
    }
    printf("\n");

    for(int i = 0; i < M; i++){
        printf("\t%d", ExistingRes[i]);
    }
    printf("\n");
    printf("\n");

    printf("Available:\n");
    for(int i = 0; i < M; i++){
        printf("\tR%d", i);
    }
    printf("\n");

    for(int i = 0; i < M; i++){
        printf("\t%d", AvailableRes[i]);
    }
    printf("\n");
    printf("\n");

    printf("Allocation:\n");
    for(int i = 0; i < M; i++){
        printf("\tR%d", i);
    }
    printf("\n");
    for(int i = 0; i < N; i++){
        printf("T%d:", i);
        for(int j = 0; j < M; j++){
            printf("\t%d", Allocated[i][j]);
        }
        printf("\n");
    }
    printf("\n");

    printf("Request:\n");
    for(int i = 0; i < M; i++){
        printf("\tR%d", i);
    }
    printf("\n");
    for(int i = 0; i < N; i++){
        printf("T%d:", i);
        for(int j = 0; j < M; j++){
            printf("\t%d", Request[i][j]);
        }
        printf("\n");
    }
    printf("\n");

    printf("MaxDemand:\n");
    for(int i = 0; i < M; i++){
        printf("\tR%d", i);
    }
    printf("\n");
    for(int i = 0; i < N; i++){
        printf("T%d:", i);
        for(int j = 0; j < M; j++){
            printf("\t%d", MaxRes[i][j]);
        }
        printf("\n");
    }
    printf("\n");

    printf("Need:\n");
    for(int i = 0; i < M; i++){
        printf("\tR%d", i);
    }
    printf("\n");
    for(int i = 0; i < N; i++){
        printf("T%d:", i);
        for(int j = 0; j < M; j++){
            printf("\t%d", Need[i][j]);
        }
        printf("\n");
    }
    printf("#######################\n");

    pthread_mutex_unlock(&lock);
    return;
}
