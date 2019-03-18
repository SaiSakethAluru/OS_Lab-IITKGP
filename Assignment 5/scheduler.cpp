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
int main(int argc, char* argv[])
{
	int mq1 = atoi(argv[1]);
	// int mq1_id = atoi(argv[1]);
	int mq2 = atoi(argv[2]);
	// int mq2_id = atoi(argv[2]);
	int mq1_id = msgget(mq1,IPC_CREAT|0666);
	int mq2_id = msgget(mq2,IPC_CREAT|0666);

	int k = atoi(argv[3]);
	if(debug)
	cout<<"SCHED: "<<" mq1_id = "<<mq1_id<<" mq2_id = "<<mq2_id<<endl;
	if(debug)
	printf("SCHED: Received k value %d\n",k);
	int i=0;
	while(i<k){
		process_node pr_node;
		bzero(&pr_node, sizeof(process_node));
		pr_node.mtype = 1;
		msgrcv(mq1_id,&pr_node,sizeof(pr_node.pid),1,0);
		int curr_pid = pr_node.pid;
		if(debug)
		printf("SCHED: Received process %d\n",curr_pid);
		// kill(curr_pid,SIGUSR1);
		if(kill(curr_pid,SIGUSR1)<0){
			perror("SCHED: ERROR, kill");
		}
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
		// process_node pr_node;
		pr_node.mtype = 1;
		pr_node.pid = curr_pid;
		msgsnd(mq1_id,&pr_node,sizeof(pr_node.pid),0);
		if(debug)
		printf("SCHED: Added to queue\n");

	}
	pid_t master_pid = getppid();
	if(debug)
	printf("SCHED: Master has pid %d\n",master_pid);
	kill(master_pid,SIGUSR1);
	if(debug)
	printf("SCHED: Sent signal to master\n");
	pause();
}

