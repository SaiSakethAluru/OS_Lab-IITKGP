#include <iostream>
#include <pthread.h>
#include <signal.h>
#include <bits/stdc++.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#define BUFFSIZE 500
#define NUM_THREADS 10
#define NUMS_PER_THREAD 10000
#define delta 1000
using namespace std;

int buffer[BUFFSIZE];
vector<int> status(NUM_THREADS);
int buffer_count = 0;
int in = 0;
int out = 0;
pthread_mutex_t sig_mutex;
pthread_mutexattr_t ma;
int current_thread = -1;

void signal_handler(int signo)
{
	if(signo == SIGUSR2){
		pause();
	}
	else{

	}
}
void * runner (void * param)
{

	int id = *(int*)param;
	int nums_left = NUMS_PER_THREAD;
	// struct sigaction act;
	// memset(&act,0,sizeof(act));
	// act.sa_handler = signal_handler;
	// sigaction(SIGUSR1,&act,0);
	// sigaction(SIGUSR2,&act,0);
	pause();
	if(status[id]==1){
		while(1){
			pause();
			while(nums_left && buffer_count != BUFFSIZE){
				pthread_mutex_lock(&sig_mutex);
				int new_num = rand()%1000+1;
				// printf("Producer %d produced %d\n",id,new_num);
				// cout<<"Producer "<<id<<" produced "<<new_num<<endl;
				buffer[in] = new_num;
				in = (in+1)%BUFFSIZE;
				buffer_count++;
				// cout<<"count = "<<buffer_count<<endl;
				nums_left--;
				// cout<<"Numes left for "<<id<<" are "<<nums_left<<endl;
				pthread_mutex_unlock(&sig_mutex);
			}
			if(nums_left==0){
				pthread_mutex_lock(&sig_mutex);
				status[id]=2;
				pthread_mutex_unlock(&sig_mutex);
				pthread_exit(0);
			}
		}
	}
	else if(status[id]==0){
		while(1)
		{
			while(buffer_count>0){
				pthread_mutex_lock(&sig_mutex);
				int new_num = buffer[out];
				// printf("Consumer %d consumed %d\n",id,new_num);
				// cout<<"Consumer "<<id<<" consumed "<<new_num<<endl;
				out = (out+1)%BUFFSIZE;
				buffer_count--;	
				// cout<<"count = "<<buffer_count<<endl;

				pthread_mutex_unlock(&sig_mutex);
			}
			pthread_mutex_lock(&sig_mutex);
			if(count(status.begin(),status.end(),1) == 0 && buffer_count <= 0)
			{
				status[id] = 3;
				pthread_mutex_unlock(&sig_mutex);
				pthread_exit(0);
			}
			pthread_mutex_unlock(&sig_mutex);
			pause();
		}
	}
	// /(&sig_mutex);
}

void * reporter(void* param)
{
	int prev_thread = -1;

	while(current_thread == -1);

	while(1)
	{
		// pthread_mutex_lock(&sig_mutex);
		if(current_thread == -2)
		{
			cout<<"Consumer Thread "<<prev_thread<<" has terminated"<<endl;
			pthread_exit(0);
		}
		if( current_thread == prev_thread)
			continue;
		if(prev_thread == -1)
			cout<<"Execution of "<<current_thread<<" has started"<<endl;
		else
		if(current_thread != -2)
			cout<<"The execution has changed from "<<prev_thread<< " to "<<current_thread<<endl;
		if((status[prev_thread] == 2 || status[prev_thread] == 3) && current_thread != -2){
			if(status[prev_thread]==2)
				cout<<"Producer ";
			else cout<<"Consumer ";
			cout<<"thread "<<prev_thread<<" has terminated"<<endl;
		}
		// if(current_thread != -1 &&current_thread != -2  && (status[current_thread] == 2 || status[current_thread] == 3)){
		// 	if(status[current_thread]==2)
		// 		cout<<"Producer ";
		// 	else cout<<"Consumer ";
		// 	cout<<"Thread "<<current_thread<<" has terminated"<<endl;
		// }
		if(current_thread != -2)
			prev_thread = current_thread;
		// pthread_mutex_unlock(&sig_mutex);

	}

}

void* scheduler(void* param)
{
	pthread_t* mythreads = (pthread_t*)param;
	queue<int> scheduler_q;
	for(int i=0;i<NUM_THREADS;i++){
		scheduler_q.push(i);
	}
	// printf("Pushed ids into queue\n");
	pthread_mutex_lock(&sig_mutex);
	// cout<<"Pushed ids into queue"<<endl;
	pthread_mutex_unlock(&sig_mutex);
	while(1){
		// if(pthread_mutex_lock(&sig_mutex)!=0){
		// 	perror("Lock failed in sched");
		// 	exit(1);
		// }
		pthread_mutex_lock(&sig_mutex);
		
		int n = count(status.begin(),status.end(),1);

		int m = count(status.begin(),status.end(),0);
		// cout<<"Thread status = ";
		for(int i=0;i<NUM_THREADS;i++){
			// cout<<status[i]<<" ";
		}
		// cout<<endl;
		// cout<<" the number of 1s left are "<<n<<" 0s is "<<m<<" whereas the buffer count is "<<buffer_count<<endl;
		if(n == 0 && buffer_count <= 0 && m==0){
			// break;
			// cout<<"The weirdest infinite loop with 1s = "<<n<<" 0s is "<<m<<" and buffer count "<<buffer_count<<endl;
			current_thread = -2;
			pthread_mutex_unlock(&sig_mutex);
			pthread_exit(0);
		}
		
		// if(pthread_mutex_unlock(&sig_mutex)!=0){
		// 	perror("Unlock failed in sched");
		// 	exit(1);
		// }
		
		int curr = scheduler_q.front();
		scheduler_q.pop();
		// printf("Going to send signal SIGUSR1 to id %d\n",curr);
		// cout<<"Going to send signal SIGUSR1 to id "<<curr<<endl;
		// pthread_t curr_id = mythreads[curr];
		if(status[curr]== 0 || status[curr]== 1)
			pthread_kill(mythreads[curr],SIGUSR1);
		current_thread = curr;
		// cout<<"Sent signal SIGUSR1 to "<<curr<<endl;
		pthread_mutex_unlock(&sig_mutex);
		// printf("Sent signal SIGUSR1 to id %d, tid %ld\n",curr, mythreads[curr]);
		usleep(delta);
		
		pthread_mutex_lock(&sig_mutex);
		// cout<<"Going to send signal SIGUSR2 to id "<<curr<<endl;
		if(status[curr]== 0 || status[curr]== 1)
			pthread_kill(mythreads[curr],SIGUSR2);
		// printf("Going to send signal SIGUSR2 to id %d\n",curr);
		// printf("Sent signal SIGUSR2 to id %d, tid %ld\n",curr, mythreads[curr]);
		// cout<<"Sent signal SIGUSR2 to "<<curr<<endl;
		if(status[curr] != 2 && status[curr] != 3){
			scheduler_q.push(curr);
			// printf("Pushing it back\n");
			// cout<<"Pushing it back"<<endl;
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
	pthread_t sched_thread, reporter_thread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	srand(time(NULL));
	int initializer[] = {0,0,0,0,0,0,0,1,0,0};
	int id,i;
	signal(SIGUSR1,signal_handler);
	signal(SIGUSR2,signal_handler);

	for(i=0;i<NUM_THREADS;i++){
		int* id = new int;
		*id = i;
		if(rand()%2==0){
			status[i] = 0;
			cout<<"Consumer thread "<<i<<" is created"<<endl;
		}
		else{ 
			status[i] = 1;
			cout<<"Producer thread "<<i<<" is created"<<endl;
		}
		// status[i] = initializer[i];
		pthread_create(&mythreads[i],NULL,runner,(void *)id);
	}
	pthread_create(&sched_thread,NULL,scheduler,mythreads);
	pthread_create(&reporter_thread,NULL,reporter,NULL);
	for(int i=0;i<NUM_THREADS;i++){
		pthread_join(mythreads[i],NULL);
		// cout<<" We are joined by "<<i<<endl;
	}

	pthread_join(sched_thread,NULL);
	cout<<"Scheduler thread has terminated"<<endl;
	pthread_join(reporter_thread,NULL);
	cout<<"Reporter thread has terminated"<<endl;
	cout<<"Everything done"<<endl;
	return 0;
}
