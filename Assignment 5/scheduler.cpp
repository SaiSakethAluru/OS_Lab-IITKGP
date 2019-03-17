#include <iostream>
#include <bits/stdc++.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

using namespace std;

int main(int argc, char* argv[])
{
	int mq1_id = atoi(argv[1].c_str());
	int mq2_id = atoi(argv[2].c_str());
	for(;;){
		int curr_pid;
		msgrcv(mq1_id,&curr_pid,sizeof(int),1,0);
		kill(curr_pid,SIGUSR1);
		int status;
		msgrcv(mq2_id,&status,sizeof(int),1,0);
		// terminated
		if(status==0){
			printf("terminated\n");
			continue;
		}
		msgsnd(mq1_id,&curr_pid,sizeof(int),0);
	}
	pid_t master_pid = getppid();
	kill(master_pid,SIGUSR1);
	pause();
}
