#include <iostream>
#include <bits/stdc++.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <signal.h>
using namespace std;

int global_count = 0;
int debug=0;

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

typedef struct {
	long mtype;
	int status;
}status_node;

typedef	map< pair<int,int>,pair<int,int> > tlb;
tlb tlb_table;
vector<int> pid_index_map;

void HandlePageFault(page_table_node* page_tables,vector<int> &free_frame_list,vector<vector<int> > &counters,int index,int pid,int page_no,int s);
void remove_from_tlb(int pid,int page_no);
void insert_into_tlb(int pid, int page_no, int frame_no, int s);

void sig_handler(int signo)
{
	if(signo == SIGUSR1)
		exit(0);
}

int main(int argc, char* argv[])
{
	signal(SIGUSR1,sig_handler);
	int mq2 = atoi(argv[1]);
	// int mq2_id = atoi(argv[1]);
	int mq3 = atoi(argv[2]);
	// int mq3_id = atoi(argv[2]);
	int sm1 = atoi(argv[3]);
	// int sm1_id = atoi(argv[3]);
	int sm2 = atoi(argv[4]);
	// int sm2_id = atoi(argv[4]);
	int sm3 = atoi(argv[5]);
	// int sm3_id = atoi(argv[5]);
	int mq2_id = msgget(mq2,IPC_CREAT|0666);
	int mq3_id = msgget(mq3,IPC_CREAT|0666);
	int k = atoi(argv[6]);
	int m = atoi(argv[7]);
	int f = atoi(argv[8]);
	int s = atoi(argv[9]);
	int sm1_id = shmget(sm1,m*k*sizeof(page_table_node),IPC_CREAT|0666);
	if(sm1_id<=0){
		perror("sm1_id");
	}
	int sm2_id = shmget(sm2,f*sizeof(int),IPC_CREAT|0666);
	if(sm2_id<=0){
		perror("sm2_id");
	}
	int sm3_id = shmget(sm3,k*sizeof(process_page_map_node),IPC_CREAT|0666);
	if(sm3_id<=0){
		perror("sm3_id");
	}
	if(debug)
	cout<<"MMU: "<<" mq2_id = "<<mq2_id<<" mq3_id = "<<mq3_id<<endl;
	// init tlb
	if(debug)
	cout<<"MMU: Received k="<<k<<" m="<<m<<" f="<<f<<" s="<<s<<endl;
	page_table_node* page_tables = (page_table_node*)shmat(sm1_id,NULL,0);
	int* free_frame_array = (int *)shmat(sm2_id,NULL,0);
	process_page_map_node* process_page_map = (process_page_map_node*)shmat(sm3_id,NULL,0);
	vector<vector<int> > counters(k,vector<int>(m,0));
	page_req_node page_req;
	vector<int> free_frame_list(f,1);
	while(1){
		int frame_sent = 0;
		page_req.mtype = 1;
		msgrcv(mq3_id,&page_req,sizeof(page_req.pid)+sizeof(page_req.page_no),1,0);
		if(debug)
		cout<<"MMU: Received page_req with pid "<<page_req.pid<<" and page_no "<<page_req.page_no<<endl;
		int index;
		if(find(pid_index_map.begin(),pid_index_map.end(),page_req.pid) == pid_index_map.end()){
			pid_index_map.push_back(page_req.pid);
			index = pid_index_map.size();
		}
		else{
			index = find(pid_index_map.begin(),pid_index_map.end(),page_req.pid) - pid_index_map.begin();
		}
		if(debug)
		cout<<"MMU: Index of the process "<<index<<endl;
		page_response_node page_response;
		page_response.mtype = 2;
		if(page_req.page_no==-9){
			// int status = 0;
			// if(debug)
			status_node st_node;
			st_node.mtype = 1;
			st_node.status = 0;
			cout<<"MMU: Process sent 9, terminated,status = "<<st_node.status<<endl<<endl;
			msgsnd(mq2_id,&st_node,sizeof(st_node.status),0);
			continue;
		}
		for(int i=0;i<k;i++){
			if(process_page_map[i].pid == page_req.pid && page_req.page_no >= process_page_map[i].num_pages){
				page_response.frame_no = -2;
				if(debug)
				cout<<"MMU: Invalid page no. This shoudn't happen\n";
				msgsnd(mq3_id,&page_response,sizeof(page_response.frame_no),0);
				frame_sent = 1;
				break;
			}
		}
		if(frame_sent)
			continue;

		// access tlb
		if(tlb_table.find(pair<int,int>(page_req.pid,page_req.page_no)) != tlb_table.end()){
			if(debug)
			cout<<"MMU: Found in tlb"<<endl;
			global_count++;
			cout<<"MMU: Global count "<<global_count<<" process id "<<
				page_req.pid<<" and page no "<<page_req.page_no<<endl;
			if(debug)
			cout<<"MMU: Current value of global_count = "<<global_count<<endl;
			page_response.frame_no = tlb_table[pair<int,int>(page_req.pid,page_req.page_no)].first;
			tlb_table[pair<int,int>(page_req.pid,page_req.page_no)].second = global_count;
			counters[index][page_req.page_no] = global_count;
		}

		else if(page_tables[index*m+page_req.page_no].frame_no == -1){
			// if(debug)
			cout<<"MMU: Page fault occured at process "<<page_req.pid<<
					" requesting page no "<<page_req.page_no<<endl;
			page_response.frame_no = -1;
			// msgsnd(mq3_id,&page_response,sizeof(page_response_node),0);
			global_count++;
			cout<<"MMU: Global count "<<global_count<<" process id "<<
				page_req.pid<<" and page no "<<page_req.page_no<<endl;
			HandlePageFault(page_tables,free_frame_list,counters,index,page_req.pid,page_req.page_no,s);
			// int status = 1;
			status_node st_node;
			st_node.mtype = 1;
			st_node.status = 1;
			msgsnd(mq2_id,&st_node,sizeof(st_node.status),0);
			if(debug)
			cout<<"MMU: sending status "<<st_node.status<<endl;
		}
		else{
			if(debug)
			cout<<"MMU: Page found in page table"<<endl;
			page_response.frame_no = page_tables[index*m+page_req.page_no].frame_no;
			if(debug)
			cout<<"MMU: Frame number assigned in message"<<endl;
			global_count++;
			cout<<"MMU: Global count "<<global_count<<" process id "<<
				page_req.pid<<" and page no "<<page_req.page_no<<endl;
			if(debug)
			cout<<"MMU: Current value of global_count = "<<global_count<<endl;
			
			// counters[index][page_req.page_no] = global_count;
			// cout<<"MMU: After the assignment counter["<<index<<\
				"]["<<page_req.page_no<<"] is "<<global_count<<endl;
			// msgsnd(mq3_id,&page_response,sizeof(page_response_node),0);
			// insert into tlb
			if(debug)
				cout<<"MMU: Enter into tlb"<<endl;
			insert_into_tlb(page_req.pid,page_req.page_no,page_response.frame_no,s);
			if(debug)
				cout<<"MMU: Enter into tlb returned successfully"<<endl;
		}
		msgsnd(mq3_id,&page_response,sizeof(page_response.frame_no),0);
		if(debug)
		cout<<"MMU: Sent message to process"<<endl;
	}
}

void HandlePageFault(page_table_node* page_tables,vector<int> &free_frame_list,vector<vector<int> > &counters,int index,int pid,int page_no,int s)
{
	if(debug)
	cout<<"MMU: In HandlePageFault"<<endl;
	int m = counters[0].size();
	if(find(free_frame_list.begin(),free_frame_list.end(),1) == free_frame_list.end()){
		if(debug)
		cout<<"MMU: Free frame not available"<<endl;
		int lru_row=0,lru_col=0;
		int lru_time = -1;
		for(int i=0;i<counters.size();i++){
			for(int j=0;j<counters[i].size();j++){
				if(page_tables[i*counters[i].size()+j].frame_no != -1 && global_count-counters[i][j]>lru_time){
					lru_row = i;
					lru_col = j;
					lru_time = global_count-counters[i][j];
				}
			}
		}
		if(debug)
		cout<<"MMU: lru_row = "<<lru_row<<" lru_col = "<<lru_col<<endl;
		int frame_no = page_tables[lru_row*m+lru_col].frame_no;
		if(debug)
		cout<<"MMU: frame_no = "<<frame_no<<endl;
		page_tables[index*m+page_no].frame_no = frame_no;
		page_tables[lru_row*m+lru_col].frame_no = -1;
		if(debug)
		cout<<"MMU: global_count = "<<global_count<<endl;
		counters[index][page_no] = global_count;
		remove_from_tlb(pid_index_map[lru_row],lru_col);
		insert_into_tlb(pid,page_no,frame_no,s);

	}
	else{
		if(debug)
		cout<<"MMU: Free frame available"<<endl;
		int frame_no = find(free_frame_list.begin(),free_frame_list.end(),1) - free_frame_list.begin();
		free_frame_list[frame_no] = 0;
		page_tables[index*m+page_no].frame_no = frame_no;
		insert_into_tlb(pid,page_no,frame_no,s);
	}
}

void insert_into_tlb(int pid, int page_no, int frame_no, int s)
{
	if(debug)
	cout<<"MMU: Inserting into tlb"<<endl;
	if(tlb_table.size()<s){
		pair< pair<int,int>, pair<int,int> > to_insert(pair<int,int>(pid,page_no),pair<int,int>(frame_no,global_count));
		tlb_table.insert(to_insert);
	}
	else{
		// typeof(tlb_table.begin()) lru_it = tlb_table.begin();
		auto lru_it = tlb_table.begin();
		int lru_time = -1;
		// for(typeof(tlb_table.begin()) it = tlb_table.begin();it != tlb_table.end();it++){
		for(auto it = tlb_table.begin();it != tlb_table.end();it++){
			if(global_count - it->second.second > lru_time) {
				lru_it = it;
				lru_time = global_count - it->second.second;
			}
		}
		// if(debug)
		// cout<<"MMU: To be deleted position "<<lru_it - tlb_table.begin()<<endl;
		tlb_table.erase(lru_it);
		pair< pair<int,int>, pair<int,int> > to_insert(pair<int,int>(pid,page_no),pair<int,int>(frame_no,global_count));
		tlb_table.insert(to_insert);
	}
}

void remove_from_tlb(int pid,int page_no)
{
	// check this
	tlb_table.erase(pair<int,int>(pid,page_no));
}