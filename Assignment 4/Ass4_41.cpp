#include <iostream>
#include <pthread.h>
#include <signal.h>
#include <bits/stdc++.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#define BUFFSIZE 50
#define NUM_THREADS 2
#define NUMS_PER_THREAD 10
using namespace std;

int buffer[BUFFSIZE];
vector<int> status(NUM_THREADS);
int buffer_count = 0;
int in = 0;
int out = 0;
pthread_mutex_t sig_mutex;
pthread_mutexattr_t ma;

void *producer(void* param)
{
	// int id = *(int *)param;
	// free(param);
	int id = 0;
	printf("pid = %d\n",id );
	sigset_t signal_set;
	int sig,flag = 0;
	int nums_left = NUMS_PER_THREAD;
	struct timespec timeout = {0,1};
	while(1){
		sigfillset(&signal_set);
		// sig = sigtimedwait(&signal_set,NULL,&timeout);
		sig = sigwaitinfo(&signal_set,NULL);

		// printf("Prod Thread id= %d, Signal value = %d\n",id,sig);
		if(sig == SIGUSR1)
			flag = 1;
		else if(sig == SIGUSR2)
			flag = 0;
		switch(flag){
			case 1:
				if(pthread_mutex_lock(&sig_mutex)!=0){
					perror("Lock failed in prod");
					exit(1);
				}
				if(sig>0)
					printf("Prod Thread with id %d got SIGUSR1 and locked the buffer\n",id);
				if(nums_left && buffer_count != BUFFSIZE){
					int new_num = rand()%1000+1;
					printf("Prod Thread with id %d produced %d\n",id,new_num);
					buffer[in] = new_num;
					in = (in+1)%BUFFSIZE;
					buffer_count++;
					printf("buffer_count = %d\n",buffer_count);
					nums_left--;
				}
				if(nums_left==0){
					status[id] = 2;
				}
				if(pthread_mutex_unlock(&sig_mutex)!=0){
					perror("Unlock failed in prod");
					exit(1);
				}
				if(nums_left == 0){
					int retval = 0;
					pthread_exit(&retval);					
				}
				printf("Prod thread %d unlocked buffer\n",id);
				break;
			case 0:
				if(sig>0)
					printf("Prod Thread with id %d got SIGUSR2 and unlocked the buffer\n",id);
				break;	
			default:
				printf("Error in switch case\n");		
		}
	}
}
void *consumer(void * param)
{
	// int id = *(int *)param;
	int id = 1;
	printf("cid = %d\n",id);
	// free(param);
	sigset_t signal_set;
	int sig,flag = 0;
	struct timespec timeout = {0,1};
	while(1){
		sigfillset(&signal_set);
		// sig = sigtimedwait(&signal_set,NULL,&timeout);
		sig = sigwaitinfo(&signal_set,NULL);
		// printf("Cons Thread id = %d,Signal value = %d\n",id,sig);
		if(sig == SIGUSR1)
			flag = 1;
		else if(sig == SIGUSR2)
			flag = 0;
		switch(flag){
			case 1:
				printf("Reached case 1\n");
				if(pthread_mutex_lock(&sig_mutex)!=0){
					perror("Lock failed in cons");
					exit(1);
				}
				printf("Cons Thread with id %d got SIGUSR1 and locked the buffer\n",id);
				if(buffer_count){
					int new_num = buffer[out];
					printf("Cons Thread with id %d consumed %d from buffer\n",id,new_num);
					out = (out+1)%BUFFSIZE;
					buffer_count--;
					printf("buffer_count = %d\n",buffer_count);

				}
				else{
					int retval = 0;
					printf("Exiting\n");
					pthread_exit(&retval);
				}
				if(pthread_mutex_unlock(&sig_mutex)!=0){
					perror("Unlock failed in cons");
					exit(1);
				}
				printf("Cons thread %d unlocked buffer\n",id);
				break;
			case 0:
				// printf("Reached case 0\n");
				if(sig>0)
					printf("Cons Thread with id %d got SIGUSR2 and unlocked the buffer\n",id);
				break;
			default:
				printf("Error in switch case\n");			
		}
	}
}


void* scheduler(void* param)
{
	pthread_t* mythreads = (pthread_t*)param;
	queue<int> scheduler_q;
	for(int i=0;i<NUM_THREADS;i++){
		scheduler_q.push(i);
	}
	printf("Pushed ids into queue\n");
	int delta = 20;
	while(!q.empty()){
		if(pthread_mutex_lock(&sig_mutex)!=0){
			perror("Lock failed in sched");
			exit(1);
		}
		if(count(status.begin(),status.end(),1) == 0 && buffer_count == 0){
			pthread_mutex_unlock(&sig_mutex);
			// break;
			exit(0);
		}
		if(pthread_mutex_unlock(&sig_mutex)!=0){
			perror("Unlock failed in sched");
			exit(1);
		}
		int curr = scheduler_q.front();
		scheduler_q.pop();
		printf("Going to send signal SIGUSR1 to id %d\n",curr);
		// pthread_t curr_id = mythreads[curr];
		pthread_kill(mythreads[curr],SIGUSR1);
		printf("Sent signal SIGUSR1 to id %d, tid %ld\n",curr, mythreads[curr]);
		usleep(delta);
		printf("Going to send signal SIGUSR2 to id %d\n",curr);
		pthread_kill(mythreads[curr],SIGUSR2);
		printf("Sent signal SIGUSR2 to id %d, tid %ld\n",curr, mythreads[curr]);
		pthread_mutex_lock(&sig_mutex);
		if(status[curr] != 2){
			scheduler_q.push(curr);
			printf("Pushing it back\n");
		}
		pthread_mutex_unlock(&sig_mutex);
	}
}
int main()
{
	pthread_mutexattr_init(&ma);
	pthread_mutexattr_settype(&ma,PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&sig_mutex,&ma);
	pthread_t mythreads[NUM_THREADS];
	pthread_t sched_thread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	srand(time(NULL));
	int id,i;
	for(i=0;i<NUM_THREADS;i++){
		// int* id = (int*)malloc(sizeof(int));
		// id = i;
		// *id = i;
		// if(rand()%2==0){
		if(i%2==0){
			printf("Id %d is a producer\n",i);
			pthread_create(&mythreads[i],NULL,producer,&i);
			status[i] = 1;
			struct sigaction sa,sb;
			sa.sa_handler = (void (*) (int))producer;
			sb.sa_handler = (void (*) (int))producer;
			sa.sa_flags = SA_RESTART;
			sb.sa_flags = SA_RESTART;
			// sa.sa_flags = 0;
			sigemptyset(&sa.sa_mask);
			sigemptyset(&sb.sa_mask);
			sigaction(SIGUSR1,&sa,0);
			sigaction(SIGUSR2,&sb,0);
		}
		else{
			printf("Id %d is a consumer\n",i);
			pthread_create(&mythreads[i],NULL,consumer,&i);
			status[i] = 0;
			struct sigaction sa,sb;
			sa.sa_handler = (void (*) (int))consumer;
			sb.sa_handler = (void (*) (int))consumer;
			sa.sa_flags = SA_RESTART;
			sb.sa_flags = SA_RESTART;
			// sa.sa_flags = 0;
			sigemptyset(&sa.sa_mask);
			sigemptyset(&sb.sa_mask);
			sigaction(SIGUSR1,&sa,0);
			sigaction(SIGUSR2,&sb,0);
		}
	}
	pthread_create(&sched_thread,NULL,scheduler,mythreads);
	void **status_ptr;
	// for(int i=0;i<NUM_THREADS;i++){
	// 	pthread_join(mythreads[i],status_ptr);
	// }
	pthread_join(sched_thread,status_ptr);
	printf("Scheduler died\n");
	pthread_join(mythreads[0],status_ptr);
	pthread_join(mythreads[1],status_ptr);
	
	printf("Everything done\n");
	return 0;
}