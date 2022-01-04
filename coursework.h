#include <sys/time.h>
// Duration of the time slice for the round robin algorithm
#define TIME_SLICE 5

// Number of processes to create
#define NUMBER_OF_PROCESSES 10

// size of the bounded buffer for task 2 onwards
#define BUFFER_SIZE 5

// number of consumers to use from task 3 onwards
#define NUMBER_OF_CONSUMERS 5

// maximum duration of the individual processes, in milli seconds. Note that the times themselves will be chosen at random in ]0,100]
#define MAX_BURST_TIME 100 


#define NEW 1
#define READY 2
#define RUNNING 3
#define FINISHED 4

/* 
 * Definition of the structure containing the process characteristics. These should be sufficient for the full implementation of all tasks.
 */

struct process
{
	int iProcessId;
	struct timeval oTimeCreated;
	int iBurstTime;
	struct process * oNext;
	int iState;
};

struct process * generateProcess();
long int getDifferenceInMilliSeconds(struct timeval start, struct timeval end);
void simulateRoundRobinProcess(struct process * oTemp, struct timeval * oStartTime, struct timeval * oEndTime);
void runProcess(int iBurstTime, struct timeval * oStartTime, struct timeval * oEndTime);


