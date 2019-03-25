#include <iostream>
#include <bits/stdc++.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <signal.h>

using namespace std;

// A parameter to print debug statements
int debug = 1;

/*
	This is a data structure that is used to receive from and trasmit
	messages to the processes
	The mtype can take values 1 when from process to mmu and 2 when from mmu to process
	the pid is the pid of the process sending the message and, 
	the page_no is the page number that the process is requesting
*/
typedef struct{
	long mtype;
	int pid;
	int page_no;
}page_req_node;

/*
	This is a data structure that sends the message from 
	the MMU to the process. The mtype value is 2 in this case
	and the frame_no is the response sent to the process
*/
typedef struct{
	long mtype;
	int frame_no;
}page_response_node;

// The handler to receive signal from the scheduler, asking it to start
void sig_handler(int signo)
{
	if(signo == SIGUSR1){
		// do nothing
	}
}


int main(int argc, char*argv[])
{
	// Get the arguments from command line
	/*
		argv[1] = Page referenve string
		argv[2] = mq1
		argv[3] = mq2
	*/

	//Get the page reference string and the message queues
    string pr_str = string(argv[1]);
    int mq1 = atoi(argv[2]);
	int mq3 = atoi(argv[3]);
	int mq1_id = msgget(mq1,IPC_CREAT|0666);
	int mq3_id = msgget(mq3,IPC_CREAT|0666);

	// Get its own pid
	pid_t process_pid = getpid();
	
	if(debug)
		cout<<"Process:- The ref string is "<<pr_str<<endl;
	if(debug)
		cout<<"Process: with pid "<<process_pid<<" started"<<endl;
	
	// Install the signal handler
	signal(SIGUSR1, sig_handler);

	// Create a vector of page numbers from the \
	page reference string for ease of use
    vector<int> prs;
	char* temp_str;
    while(pr_str.length()>0){
		int pos = pr_str.find("|");
        
        if(pos!=string::npos)
        {
            prs.push_back(atoi((pr_str.substr(0,pos)).c_str()));
            pr_str.erase(0, pr_str.find("|")+1);
			if(debug)
            cout<<"Process "<<process_pid<<" prs["<<prs.size()-1<<"] = "<<\
                    prs[prs.size()-1]<<endl;
        }
        else
            break;
    }

	if(debug)
		cout<<"Process "<<process_pid<<":- has initialized the prs"<<endl;
	
	prs.push_back(atoi(pr_str.c_str()));

	int n = prs.size();
	page_req_node p_req;
	page_response_node p_res;
	
	if(debug)
		cout<<"Process "<<process_pid<<": is abput to pause for the first time"<<endl;
	
	/*******Enough preprocessing, wait for the signal form scheduler**************/
	pause();
	
	//Now start
	for(int i=0;i<n;)
	{
		// A page request node is of mtype 1
		p_req.mtype = 1;
		p_req.pid = process_pid;
		p_req.page_no = prs[i];


		if(debug)
			cout<<"Process "<<process_pid<<": is sending the page no "\
				<<prs[i]<<endl;
		
		// Send the request to the mmu using queue mq2
		msgsnd(mq3_id, &p_req, sizeof(p_req.pid)+sizeof(p_req.page_no),0);

		p_res.mtype = 2;// Page response is of mtype 2

		// Receive response from MMU using mq3
		msgrcv(mq3_id, &p_res, sizeof(p_res.frame_no), 2, 0);
		
		// Get the frame number from the response
		int frame_recv = p_res.frame_no;
		
		if(debug)
			cout<<"Process "<<process_pid<<": has received frame no "<<frame_recv<<endl;

		// If a valid frame number is received, continue
		if(frame_recv>=0)
		{
			i++;
			continue;
		}

		// If a page fault indicator is received, pause
		if(frame_recv == -1)
		{
			pause();
			continue;
		}

		// If a terminate order is received, exit
		if(frame_recv == -2)
		{
			exit(0);
		}
	}

	// If all frames numbers have correctly been received
	// Send -9 as a termination signal to the MMU using queue 1
	p_req.mtype = 1;
	p_req.pid = process_pid;
	p_req.page_no = -9;
	if(debug)
		cout<<"Process "<<process_pid<<": is sending the page no "\
			<<p_req.page_no<<endl;
	msgsnd(mq3_id, &p_req, sizeof(p_req.pid)+sizeof(p_req.page_no),0);

}