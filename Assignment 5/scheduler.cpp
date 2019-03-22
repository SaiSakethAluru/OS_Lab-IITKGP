#include <iostream>
#include <bits/stdc++.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
using namespace std;
int debug=1;

typedef struct {
	long mtype;
	int pid;
}process_node;

typedef struct {
	long mtype;
	int status;
}status_node;

void sig_handler(int signo)
{
	if(signo == SIGUSR2)
	{
		sleep(10);
		cout<<"SCHED: Exiting the Scheduler"<<endl;
		exit(0);
	}
	else
	{
		cout<<"SCHED: signal received form incorrect address"<<endl;
	}
}

int main(int argc, char* argv[])
{
	signal(SIGUSR2,sig_handler);
	signal(SIGUSR1,sig_handler);
	int mq1 = atoi(argv[1]);
	// int mq1_id = atoi(argv[1]);
	int mq2 = atoi(argv[2]);
	// int mq2_id = atoi(argv[2]);
	int mq1_id = msgget(mq1,IPC_CREAT|0666);
	int mq2_id = msgget(mq2,IPC_CREAT|0666);
	
	if(debug)
		cout<<"SCHED: "<<" mq1_id = "<<mq1_id<<" mq2_id = "<<mq2_id<<endl;
	int k = atoi(argv[3]);
	if(debug)
	printf("SCHED: Received k value %d\n",k);
	int i=0;
	while(i<k){
		process_node *pr_node;
		pr_node = new process_node;
		bzero(pr_node, sizeof(process_node));
		pr_node->mtype = 1;
		msgrcv(mq1_id,pr_node,sizeof(pr_node->pid),1,0);
		int curr_pid = pr_node->pid;
		sleep(1);
		if(debug)
		printf("SCHED: Received process %d\n",curr_pid);
		// kill(curr_pid,SIGUSR1);
		if(kill(curr_pid,SIGUSR1)<0){
			perror("SCHED: ERROR, kill");
		}
		else
		if(debug)
			cout<<"SCHED: Sent signal for process "<<\
				curr_pid<<" to start"<<endl;
		// int status;

		status_node st_node;
		st_node.mtype = 1;
		msgrcv(mq2_id,&st_node,sizeof(st_node.status),1,0);
		int status = st_node.status;
		// terminated
		if(debug)
		printf("SCHED: Received status %d\n",status);
		if(status==0){
			if(debug)
			printf("SCHED: terminated\n");
			i++;
			continue;
		}
		if(debug)
		cout<<"SCHED: sending pid "<<curr_pid<<" back to schedule "<<endl;
		process_node *pr_node2 = new process_node;
		pr_node2->mtype = 1;
		pr_node2->pid = curr_pid;
		msgsnd(mq1_id,pr_node2,sizeof(pr_node2->pid),0);
		if(debug)
		printf("SCHED: Added to queue\n");

	}
	pid_t master_pid = getppid()-1;
	if(debug)
	printf("SCHED: Master has pid %d\n",master_pid);
	// sleep(10);

	if(kill(master_pid,SIGUSR1)<0)
		cout<<"SCHED: Incorrect master pid"<<endl;

	if(debug)
		printf("SCHED: Sent signal to master\n");
	// sleep(100);
	
	pause();
}

