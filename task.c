#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <semaphore.h>
#include <pthread.h>
#include "coursework.h"

struct calculationBlock {
    sem_t empty;
    sem_t full;
    sem_t mutex;

    struct process * pHead;
    struct process * pTail;

    int NUMBER_OF_PROCESS_CREATED;
    int initialBurstTime[NUMBER_OF_PROCESSES];

    double Avg_response_time;
    double Avg_turnAround_time;
    int response[NUMBER_OF_PROCESSES];                      // number of the maximun process to create
    int turnAround[NUMBER_OF_PROCESSES];                    // number of the maximun process to create
};

struct calculationBlock semCalculation;

// Add a new process at the last of the linked-list
void addL(struct process * newProcess, struct process ** pHead, struct process ** pTail) {
	if((*pHead) == NULL) {
		newProcess->oNext = (*pHead);
		(*pTail) = (*pHead) = newProcess;
	} else {
		newProcess->oNext = NULL;
		(*pTail)->oNext = newProcess;
		(*pTail) = newProcess;
	}
}

// Get a new process at the front of the linked-list
struct process * removeF(struct process ** pHead, struct process ** pTail) {
	struct process * toRemoveProcess = (struct process *) malloc (sizeof(struct process));
	struct process * pTemp = NULL;

	if ((*pHead) != NULL) {
		pTemp = (*pHead);
		(*pHead) = (*pHead)->oNext;

		if((*pHead) == NULL)
			(*pTail) = NULL;

		toRemoveProcess->iProcessId = pTemp->iProcessId;
		toRemoveProcess->oTimeCreated = pTemp->oTimeCreated;
		toRemoveProcess->iBurstTime = pTemp->iBurstTime;
		toRemoveProcess->oNext = NULL;
		toRemoveProcess->iState = pTemp->iState;

		free(pTemp);
	}

	return toRemoveProcess;
}

// Producer function
void *producer_func(void* arg) {
    int index = *(int* )arg;

    while(1) {
        sem_wait(&semCalculation.empty);  // empty (if empty < 0, then go to sleep)
        sem_wait(&semCalculation.mutex);  // mutex (if mutex < 0, then go to sleep)

        // ---------- Enter Critical Section ----------
        if(semCalculation.NUMBER_OF_PROCESS_CREATED == NUMBER_OF_PROCESSES){
            // All processes produced successfully
            sem_post(&semCalculation.full);
            sem_post(&semCalculation.mutex);
            break;
        }

        // Create One Process if not exceed maximum number
        struct process * otemp = generateProcess();
        semCalculation.initialBurstTime[otemp->iProcessId] = otemp->iBurstTime;

        // Add to the buffer
        addL(otemp, &semCalculation.pHead, &semCalculation.pTail);
        semCalculation.NUMBER_OF_PROCESS_CREATED++;
        // ---------- Exit Critical Section ----------

        sem_post(&semCalculation.mutex);           // mutex++

        //printf("Producer,    New Process Id = %d, Item Produced = %d, Burst Time = %d\n", otemp->iProcessId, semCalculation.NUMBER_OF_PROCESS_CREATED, otemp->iBurstTime);
        //printf("New Process Id = %d, ", otemp->iProcessId);
        //printf("Item Produced = %d, ", semCalculation.NUMBER_OF_PROCESS_CREATED);
        //printf("Burst Time = %d\n", otemp->iBurstTime);

        int time = semCalculation.initialBurstTime[otemp->iProcessId];
        while(time > 0) {
            sem_post(&semCalculation.full);        // post enough semCalculationaphore for consumer
            time = time - TIME_SLICE;
        }
    }
    pthread_exit(NULL);                             // if number of process created reached maximum, exit the thread
}

// Consumer function
void *consumer_func(void* arg) {
    int index = *(int* )arg;

    while(1) {
        sem_wait(&semCalculation.full);   // full (if full < 0, then go to sleep)
        sem_wait(&semCalculation.mutex);  // mutex (if mutex < 0, then go to sleep)

        // ---------- Enter Critical Section ----------
        if(semCalculation.pHead == NULL){
            sem_post(&semCalculation.full);
            sem_post(&semCalculation.mutex);
            if(semCalculation.NUMBER_OF_PROCESS_CREATED == NUMBER_OF_PROCESSES){
                break;
            }else{
                continue;
            }
        }

        // Get the process (waiting for remove) from the head
        struct process * otemp = removeF(&semCalculation.pHead, &semCalculation.pTail);

        sem_post(&semCalculation.mutex);
        // ---------- Exit Critical Section -----------

        // Run Process!
        struct timeval oStartTime;
        struct timeval oEndTime;
        int previousBurstTime = otemp->iBurstTime;

        // Check whether if it is the first run
        int isFirst = 0;
        if (previousBurstTime == semCalculation.initialBurstTime[otemp->iProcessId]) {
            isFirst = 1;
            simulateRoundRobinProcess(otemp, &oStartTime, &oEndTime);
            semCalculation.response[otemp -> iProcessId] = getDifferenceInMilliSeconds(otemp -> oTimeCreated, oStartTime);
        } else {
            simulateRoundRobinProcess(otemp, &oStartTime, &oEndTime);
        }

        // ---------- Enter Critical Section ----------
        sem_wait(&semCalculation.mutex);
        // If the remaining time is greater than 0, add to the process linked list
        int activate_producer = 0;
        if(otemp->iBurstTime > 0) {
            otemp->oNext = NULL;
            addL(otemp, &semCalculation.pHead, &semCalculation.pTail);
        } else {
            isFirst = 2;
            activate_producer = 1;
            semCalculation.turnAround[otemp -> iProcessId] = getDifferenceInMilliSeconds(otemp -> oTimeCreated, oEndTime);
            semCalculation.Avg_response_time += semCalculation.response[otemp -> iProcessId];
            semCalculation.Avg_turnAround_time += semCalculation.turnAround[otemp -> iProcessId];
        }

        // Print Response / TurnAround Time for each process
        printf("Consumer ID = %d, ", index);
        printf("Process ID = %d, ", otemp -> iProcessId);
        printf("Previous Burst Time = %d, ", previousBurstTime);
        printf("New Burst Time = %d",otemp -> iBurstTime);

        // If the process runs for the first time
        if (isFirst == 1) {
            printf(", Response Time = %d", semCalculation.response[otemp -> iProcessId]);
        }

        // If the process has finished running
        if (isFirst == 2) {
            printf(", TurnAround Time: %d ", semCalculation.turnAround[otemp -> iProcessId]);
            otemp->oNext = NULL;
            free(otemp);
        }
        printf("\n");

        // ---------- Exit Critical Section -----------

        sem_post(&semCalculation.mutex); // mutex++

        if(activate_producer){
            // Process finished running, activate producer
            sem_post(&semCalculation.empty);
        }
    }

    pthread_exit(NULL); // if number of process created reached maximum, exit the thread
}

int main(void) {
    // Initialize the semaphores
    sem_init(&semCalculation.empty, 0, BUFFER_SIZE);    // Initialize the empty to BUFFER_SIZE
    sem_init(&semCalculation.full, 0, 0);               // Initialize the full to 0
    sem_init(&semCalculation.mutex, 0, 1);              // Initialize the mutex to 1 because only one thread can enter the critical section
    
    // Initialize calculationBlock struct
    semCalculation.NUMBER_OF_PROCESS_CREATED = 0;
    semCalculation.pHead = NULL;
    semCalculation.pTail = NULL;
    semCalculation.Avg_response_time = 0;
    semCalculation.Avg_turnAround_time = 0;

    pthread_t producer;
    pthread_t consumerArray[NUMBER_OF_CONSUMERS];
    int i = 0;
    int index_producer;
    int index_consumer[NUMBER_OF_CONSUMERS];
   
    // Create producer and consumer processes
    pthread_create(&producer, NULL, producer_func, &index_producer);

    for(i = 0; i < NUMBER_OF_CONSUMERS; i++){
        pthread_t consumer;
        index_consumer[i] = i;
        pthread_create(&consumer, NULL, consumer_func, &index_consumer[i]);
        consumerArray[i] = consumer;
    }

    // Wait for all subthread to exit
    pthread_join(producer, NULL);
    
    int j;
    for(j = 0; j < NUMBER_OF_CONSUMERS; j++){
        pthread_join(consumerArray[j], NULL);
    }

    // Calculate average response and turn around time
    int n = NUMBER_OF_PROCESSES; // number of the maximun job
    semCalculation.Avg_response_time /= n;
    semCalculation.Avg_turnAround_time /= n;
    printf("----------\n");
    printf("Average Response Time: %lf \n", semCalculation.Avg_response_time);
    printf("Average TurnAround Time: %lf \n", semCalculation.Avg_turnAround_time);

    // Destroy semCalculationaphores and exit the program
    sem_destroy(&semCalculation.empty);
    sem_destroy(&semCalculation.full);
    sem_destroy(&semCalculation.mutex);
    return 0;
}