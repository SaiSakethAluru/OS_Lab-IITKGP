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
	int frame_no;
	int valid;
}page_table_node;

typedef struct{
	int pid;
	int num_pages;
}process_page_map_node;

typedef struct{
	long mtype;
	// int pid;
	int frame_no;
}page_response_node;

int main(int argc, char* argv[])
{
	int mq2_id = atoi(argv[1].c_str());
	int mq3_id = atoi(argv[2].c_str());
	int sm1_id = atoi(argv[3].c_str());
	int sm2_id = atoi(argv[4].c_str());
	int sm3_id = atoi(argv[5].c_str());
	int k = atoi(argv[6].c_str());
	int m = atoi(argv[7].c_str());
	// init tlb

	page_table_node* page_tables = (page_table_node*)shmat(sm1_id,NULL,0);
	int* free_frame_list = (int *)shmat(sm2_id,NULL,0);
	process_page_map_node* process_page_map = (process_page_map_node*)shmat(sm3_id,NULL,0);

	vector<int> pid_index_map;
	page_req_node page_req;
	while(1){
		int frame_sent = 0;
		msgrcv(mq3_id,&page_req,sizeof(page_req_node),1,0);
		int index;
		if(find(pid_index_map.begin(),pid_index_map.end(),page_req.pid) == pid_index_map.end()){
			pid_index_map.append(page_req.pid);
			index = pid_index_map.size();
		}
		else{
			index = find(pid_index_map.begin(),pid_index_map.end(),page_req.pid) - pid_index_map.begin();
		}
		page_response_node page_response;
		page_response.mtype = 2;
		if(page_req.page_no==-9){
			int status = 0;
			msgsnd(mq2_id,&status,sizeof(int),0);
			continue;
		}
		for(int i=0;i<k;i++){
			if(process_page_map[i].pid == page_req.pid && page_req.page_no >= process_page_map[i].num_pages){
				page_response.frame_no = -2;
				msgsnd(mq3_id,&page_response,sizeof(page_response_node),0);
				frame_sent = 1;
				break;
			}
		}
		if(frame_sent)
			continue;

		// access tlb

		if(page_tables[index*m+page_req.page_no] == -1){
			page_response.frame_no = -1;
			msgsnd(mq3_id,&page_response,sizeof(page_response_node),0);
			// HandlePageFault();
			int status = 1;
			msgsnd(mq2_id,&status,sizeof(int),0);
		}
		else{
			page_response.frame_no = page_tables[index*m+page_req.page_no];
			msgsnd(mq3_id,&page_response,sizeof(page_response_node),0);
		}
	}

}