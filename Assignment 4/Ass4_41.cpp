#include <iostream>
#include <pthread.h>
#include <signal.h>
#include <bits/stdc++.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
// Max size of the shared buffer
#define BUFFSIZE 500
// Number of worker threads to create
#define NUM_THREADS 10
// Random numbers produced per producer thread
#define NUMS_PER_THREAD 1000
// Time quantum of roundrobin (in ms)
#define delta 1000
using namespace std;

vector<int> status(NUM_THREADS);	// the status vector
/*
	status = 0 --> active consumer
	status = 1 --> active producer
	status = 2 --> terminated/dead producer
	status = 3 --> terminated/dead consumer
*/
int buffer[BUFFSIZE];				// The buffer to store the produced items
int buffer_count = 0;				// the number of items currently in the buffer
int in = 0;							// the current first for producing
int out = 0;						// the current index for consumption
pthread_mutex_t sig_mutex;			// the varialbe for mutex lock
pthread_mutexattr_t ma;				// the attribute for mutex lock
int current_thread = -1;			// the current executing thread

//The signal handler function to stop or play threads
void signal_handler(int signo)
{
	// If signal SIGUSR1 is given, then pause the current thread until next signal
	if(signo == SIGUSR1){
		pause();
	}
	else{
		// do nothing - resumes from pause call
	}
}

// The runner function for worker threads. This function uses status to find\
if the thread is a producer or a consumer
void * runner (void * param)
{


	int id = *(int*)param;				// get the id
	int nums_left = NUMS_PER_THREAD;	// If the thread is a producer then the \
	then the number of items it has to consume.
	
	// Pause to wait for the first signal
	pause();

	// If the thread is a producer 
	if(status[id]==1){
		// Keep continuing
		while(1){
			while(nums_left && buffer_count != BUFFSIZE){
				pthread_mutex_lock(&sig_mutex);			//start critical section
				int new_num = rand()%1000+1;			// generate a new number
				buffer[in] = new_num;					// add the number to the buffer
				in = (in+1)%BUFFSIZE;					// increment the input buffer
				buffer_count++;							// Increment the buffer count
				nums_left--;							// Decrease the number left for the particular\
															thread to follow
				pthread_mutex_unlock(&sig_mutex);		// end critical section
			}
			if(nums_left==0){							// If while loop breaks
				pthread_mutex_lock(&sig_mutex);			// lock for CS
				status[id]=2;							// set the status to producer complete
				pthread_mutex_unlock(&sig_mutex);		// unlock mutex
				pthread_exit(0);						// exit the thread
			}
		}
	}
	// If the thread is a consumer
	else if(status[id]==0){
		while(1)// Keep executing till the thread exits
		{
			while(buffer_count>0){					// While there is still left to consume
				pthread_mutex_lock(&sig_mutex);		// Mutex lock
				int new_num = buffer[out];			// get the number from the buffer
				out = (out+1)%BUFFSIZE;				// Increment the out index
				buffer_count--;						// Decrement the buffer count
				pthread_mutex_unlock(&sig_mutex);	// Unlock the buffer
			}
			pthread_mutex_lock(&sig_mutex);			// lock the mutex for end cond check
			// if everything has been completed
			if(count(status.begin(),status.end(),1) == 0 && buffer_count <= 0)
			{
				status[id] = 3;
				pthread_mutex_unlock(&sig_mutex);	// unlock the mutex
				pthread_exit(0);					// exit the thread
			}
			pthread_mutex_unlock(&sig_mutex);		// unlock the mutex.
		}
	}
}

//The reporter thread
void * reporter(void* param)
{
	// Store the previous thread
	int prev_thread = -1;

	while(current_thread == -1);	// wait for at least one thread to start

	while(1)
	{
		if(current_thread == -2)	// If execution is over
		{
			cout<<"Consumer Thread "<<prev_thread<<" has terminated"<<endl;
			cout<<"Current buffer size = "<<buffer_count<<endl;
			pthread_exit(0);
		}
		if( current_thread == prev_thread){	// If no change in the thread
			continue;
		}
		// If the execution of first thread is starting
		if(prev_thread == -1){
			cout<<"Execution of "<<current_thread<<" has started"<<endl;
			cout<<"Current buffer size = "<<buffer_count<<endl;
		}
		else
		// If scheduler thread is still active, then print the context switch message
		if(current_thread != -2){
			cout<<"The execution has changed from "<<prev_thread<< " to "<<current_thread<<endl;
			cout<<"Current buffer size = "<<buffer_count<<endl;
			
		}
		// If the context switch happened and old thread has terminated
		if((status[prev_thread] == 2 || status[prev_thread] == 3) && current_thread != -2){
			if(status[prev_thread]==2)
				cout<<"Producer ";
			else cout<<"Consumer ";
			cout<<"thread "<<prev_thread<<" has terminated"<<endl;
			cout<<"Current buffer size = "<<buffer_count<<endl;
		}
		// If scheduler is still active, then update old thread's value
		if(current_thread != -2)
			prev_thread = current_thread;

	}

}

// The scheduler thread
void* scheduler(void* param)
{
	// List of worker thread's tid values
	pthread_t* mythreads = (pthread_t*)param;
	queue<int> scheduler_q;			// The FIFO queue to schedule the threads
	for(int i=0;i<NUM_THREADS;i++){
		scheduler_q.push(i);		// Push the threads into the queue
	}

	while(1){
		pthread_mutex_lock(&sig_mutex);	//Lock the mutex at the start of the scheduler

		//Count the number of active threads		
		int n = count(status.begin(),status.end(),1);
		int m = count(status.begin(),status.end(),0);

		// If the exit condition is true
		if(n == 0 && buffer_count <= 0 && m==0){
			current_thread = -2;
			pthread_mutex_unlock(&sig_mutex);// unlock mutex before exiting
			pthread_exit(0);
		}
		
		// get the next thread from the queue
		int curr = scheduler_q.front();
		scheduler_q.pop();

		// If it is an active thread
		if(status[curr]== 0 || status[curr]== 1)
			pthread_kill(mythreads[curr],SIGUSR2);
		current_thread = curr;
		pthread_mutex_unlock(&sig_mutex);// unlock the mutex and let the worker does its thing

		usleep(delta);//Wait for the worker to do stuff
		
		// Lock for stopping the thread		
		pthread_mutex_lock(&sig_mutex);


		// Send the next signal for the thread to stop - only if it has not yet terminated
		if(status[curr]== 0 || status[curr]== 1)
			pthread_kill(mythreads[curr],SIGUSR1);
		
		// If the thread is not complete yet push it back
		if(status[curr] != 2 && status[curr] != 3){
			scheduler_q.push(curr);
		}

		// unlock the mutex for the next one
		pthread_mutex_unlock(&sig_mutex);
	}
}

// The main thread
int main()
{
	pthread_mutexattr_init(&ma);// Initialise the mutex attributes to default values
	// The mutex has to be recursive
	pthread_mutexattr_settype(&ma,PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&sig_mutex,&ma); 	// Initialse the mutex lock with the attributes

	pthread_t mythreads[NUM_THREADS];// declare the thread variables
	pthread_t sched_thread, reporter_thread;// declare the reporter and the scheduler
	pthread_attr_t attr;// Thread attributes
	pthread_attr_init(&attr);// Initialise pthread attributes to default values
	srand(time(NULL));// Set the random number seedd
	
	int id,i;// Variable to pass id to the function

	signal(SIGUSR1,signal_handler);// install the signal handlers
	signal(SIGUSR2,signal_handler);

	for(i=0;i<NUM_THREADS;i++){// Declare the threads
		int* id = new int;// Set the id
		*id = i;
		if(rand()%2==0){// Set if producer or consumer
			status[i] = 0;
			cout<<"Consumer thread "<<i<<" is created"<<endl;
		}
		else{ 
			status[i] = 1;
			cout<<"Producer thread "<<i<<" is created"<<endl;
		}
		pthread_create(&mythreads[i],NULL,runner,(void *)id);
	}
	// create the scheduler thread
	pthread_create(&sched_thread,NULL,scheduler,mythreads);
	// create the reporter thread
	pthread_create(&reporter_thread,NULL,reporter,NULL);

	// wait for all threads to join
	for(int i=0;i<NUM_THREADS;i++){
		pthread_join(mythreads[i],NULL);
	}
	// wait for scheduler to join
	pthread_join(sched_thread,NULL);
	cout<<"Scheduler thread has terminated"<<endl;
	// wait for reporter to join	
	pthread_join(reporter_thread,NULL);
	cout<<"Reporter thread has terminated"<<endl;
	cout<<"Everything done"<<endl;
	// destroy the mutex at the end
	pthread_mutex_destroy(&sig_mutex);
	return 0;
}
