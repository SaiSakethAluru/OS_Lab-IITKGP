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
int main(int argc, char* argv[])
{
	int mq1_id = atoi(argv[1]);
	int mq2_id = atoi(argv[2]);
	int k = atoi(argv[3]);
	if(debug)
	printf("Received k value %d\n",k);
	int i=0;
	while(i<k){
		int curr_pid;
		msgrcv(mq1_id,&curr_pid,sizeof(int),1,0);
		if(debug)
		printf("Received process %d\n",curr_pid);
		kill(curr_pid,SIGUSR1);
		int status;
		msgrcv(mq2_id,&status,sizeof(int),1,0);
		// terminated
		if(debug)
		printf("Received status %d\n");
		if(status==0){
			if(debug)
			printf("terminated\n");
			i++;
			continue;
		}
		msgsnd(mq1_id,&curr_pid,sizeof(int),0);
		if(debug)
		printf("Added to queue\n");

	}
	pid_t master_pid = getppid();
	if(debug)
	printf("Master has pid %d\n",master_pid);
	kill(master_pid,SIGUSR1);
	if(debug)
	printf("Sent signal to master\n");
	pause();
}

