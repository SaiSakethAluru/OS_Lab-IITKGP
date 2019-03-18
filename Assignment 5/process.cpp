#include <iostream>
#include <bits/stdc++.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <signal.h>

using namespace std;

typedef struct{
	long mtype;
	int pid;
	int page_no;
}page_req_node;

typedef struct{
	long mtype;
	// int pid;
	int frame_no;
}page_response_node;

void sig_handler(int signo)
{
	if(signo == SIGUSR1){
		// do nothing
	}
}

int main(int argc, char*argv[])
{
    string pr_str = string(argv[1]);
    int mq1_id = atoi(argv[2]);
	int mq3_id = atoi(argv[3]);

    vector<int> prs;
	signal(SIGUSR1, sig_handler);
	char* temp_str;
    while(pr_str.length()>0){
		int pos = pr_str.find("|");
        
        if(pos!=string::npos)
        {
            prs.push_back(atoi((pr_str.substr(0,pos)).c_str()));
            pr_str.erase(0, pr_str.find("|")+1);
            // cout<<"prs_str["<<prs.str_size()-1<<"] = "<<\
                    prs[prs.str_size()-1]<<endl;
        }
        else
            break;
    }
	prs.push_back(atoi(pr_str.c_str()));

	int n = prs.size();
	page_req_node p_req;
	page_response_node p_res;
	pause();

	for(int i=0;i<n;)
	{
		p_req.mtype = 1;
		p_req.pid = getpid();
		p_req.page_no = prs[i];
		msgsnd(mq3_id, &p_req, sizeof(page_req_node),0);
		
		msgrcv(mq3_id, &p_res, sizeof(page_response_node), 2, 0);
		int frame_recv = p_res.frame_no;

		if(frame_recv>=0)
		{
			i++;
			continue;
		}
		if(frame_recv == -1)
		{
			pause();
			continue;
		}
		if(frame_recv == -2)
		{
			exit(0);
		}
	}

}