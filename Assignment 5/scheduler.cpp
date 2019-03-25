#include <iostream>
#include <bits/stdc++.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>

using namespace std;
//A parameter for printing debug statements
int debug=1;


/*
	This a a structure that is used for transferring the next
	pid over the message queue M1. This data strucure can be 
	read by the scheduler also as it is the sol rescepient of the
	messages from MQ1
	The "mtype" attribute is used to recognize the recepinet and
	the pid indicates the pid of the process that has to be signalled
*/
typedef struct {
	long mtype;
	int pid;
}process_node;

/*
	This is a data structure that is used to send the Page Fault or
	terminated message to the scheduler
*/
typedef struct {
	long mtype;
	int status;
}status_node;

/*
	This is a handler that upon receiving the signal from master, exits the
	scheduler.
*/
void sig_handler(int signo)
{
	if(signo == SIGUSR2)
	{
		cout<<"SCHED: Exiting the Scheduler"<<endl;
		sleep(10);
		exit(0);
	}
	else
	{
		cout<<"SCHED: signal received form incorrect address"<<endl;
	}
}

int main(int argc, char* argv[])
{
	//Install the signal handlers
	/*
		SIGUSR1:- A signal that if received, indicates an error in the code
		SIGUSR2:- A signal from master that requests termination of the scheduler
	*/
	signal(SIGUSR2,sig_handler);
	signal(SIGUSR1,sig_handler);

	/*
		Get the message queues from the command line arguments
	*/

	/*
		argv[1] = mq1
		argv[2] = mq2
		argv[3] = k(Number of processes)
	*/
	int mq1 = atoi(argv[1]);
	int mq2 = atoi(argv[2]);
	int mq1_id = msgget(mq1,IPC_CREAT|0666);
	int mq2_id = msgget(mq2,IPC_CREAT|0666);
	
	if(debug)
		cout<<"SCHED: "<<" mq1_id = "<<mq1_id<<" mq2_id = "<<mq2_id<<endl;

	//The the number of processes from command line arguments
	int k = atoi(argv[3]);

	if(debug)
		printf("SCHED: Received k value %d\n",k);
	
	//Iterate for the number of processes
	int i=0;
	while(i<k){

		/******Receive process id from MQ1 to start scheduling***************/
		process_node *pr_node;// A node to receive the next process to schedule
		pr_node = new process_node;
		bzero(pr_node, sizeof(process_node));
		pr_node->mtype = 1;// The mtype is 1 to receive a process
		msgrcv(mq1_id,pr_node,sizeof(pr_node->pid),1,0);
		int curr_pid = pr_node->pid;// Get the pid of the process
		
		//Wait for the process to start(Synchronization step)
		sleep(1);
		
		if(debug)
			printf("SCHED: Received process %d\n",curr_pid);

		//Ask the process to start
		if(kill(curr_pid,SIGUSR1)<0){
			perror("SCHED: ERROR, kill");
		}
		else
		if(debug)
			cout<<"SCHED: Sent signal for process "<<\
				curr_pid<<" to start"<<endl;

		/******Wait for reply from the MMU************/
		//The reply can either be termination(0) or Page fault(1)
		status_node st_node;
		st_node.mtype = 1;// The mtype for this node is 1
		msgrcv(mq2_id,&st_node,sizeof(st_node.status),1,0);
		int status = st_node.status;
		
		if(debug)
			printf("SCHED: Received status %d\n",status);
		
		// terminated
		if(status==0){
			if(debug)
				printf("SCHED: terminated\n");
			i++;// Increment the number of processes finished
			continue;
		}

		// If page fault was received
		if(debug)
			cout<<"SCHED: sending pid "<<curr_pid<<" back to schedule "<<endl;
		
		//Schedule the process again, this will again be received by the scheduler itself
		process_node *pr_node2 = new process_node;
		pr_node2->mtype = 1;// Mtype is one for the process node
		pr_node2->pid = curr_pid;
		msgsnd(mq1_id,pr_node2,sizeof(pr_node2->pid),0);
		
		if(debug)
			printf("SCHED: Added to queue\n");

	}

	/**********All processes have terminated********************/
	pid_t master_pid = getppid()-1;// A getppid()-1 is the master's pid\
	a property based on observation

	if(debug)
		printf("SCHED: Master has pid %d\n",master_pid);

	// Signal the master that all processes have finished
	if(kill(master_pid,SIGUSR1)<0)
		cout<<"SCHED: Incorrect master pid"<<endl;

	if(debug)
		printf("SCHED: Sent signal to master\n");

	// Wait for master to respond	
	pause();
}

