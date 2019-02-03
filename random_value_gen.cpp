#include <iostream>
#include <bits/stdc++.h>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <fstream>
#define LAMBDA 0.4
using namespace std;
vector<int> generate_uniform(int n);
vector<int> generate_exponential(int n);
double fcfs_awt(vector<pair<int,int> > &processes);
double non_preemptive_sjf_atn(vector<pair<int,int> > p);
double round_robin(vector<pair<int,int> > &processes);

int main()
{
	int n;
	cin>>n;
	vector<int> burst_times = generate_uniform(n);
	vector<int> arrival_times = generate_exponential(n);
	ofstream fout;
	fout.open("cpu_times.txt");
	for(int i=0;i<n;i++){
		fout<<arrival_times[i]<<"\t"<<burst_times[i]<<endl;
		// cout<<arrival_times[i]<<"\t"<<burst_times[i]<<endl;
	}
	fout.close();
	vector<pair<int,int> > processes;
	for(int i=0;i<n;i++){
		processes.push_back(pair<int,int>(arrival_times[i],burst_times[i]));
		// int a,b;
		// cin>>a>>b;
		// processes.push_back(pair<int,int>(a,b));
	}
	fout.open("data.dat");
	double fcfs_avg_awt = fcfs_awt(processes);
	// cout<<"FCFS average turn around time = "<<fcfs_avg_awt<<endl;
	// fout<<0<<" FCFS "<<fcfs_avg_awt<<endl;
	cout<<fcfs_avg_awt<<" ";
	double sjf_avg = non_preemptive_sjf_atn(processes);
	// cout<<"Non-preemptive SJF ATT = "<<sjf_avg<<endl;
	// fout<<1<<" NPSJF "<<sjf_avg<<endl;
	cout<<sjf_avg<<" ";
	double round_robin_avg_awt = round_robin(processes);
	// cout<<"Round robin with time quantum 2 = "<<round_robin_avg_awt<<endl;
	// fout<<2<<" RRB "<<round_robin_avg_awt<<endl;
	cout<<round_robin_avg_awt<<endl;
	fout.close();
  	return 0;
}

vector<int> generate_uniform(int n)
{
	vector<int> v;
	srand(time(NULL));
	for(int i=0;i<n;i++){
		v.push_back(rand()%20+1);
	}
	return v;
}

vector<int> generate_exponential(int n)
{
	vector<int> arrival_times;
	arrival_times.push_back(0);
	// vector<int> exp_nums;
	srand(time(NULL));
	int i=1;
	while(i<n){
		double uni_rand = (float)rand() / (RAND_MAX + 1.0);
		if(uni_rand != 0){
			double val = -log(uni_rand) / LAMBDA;
			if(val <=10){
				// exp_nums.push_back(val);
				int new_val = arrival_times[arrival_times.size()-1]+val;
				arrival_times.push_back(new_val);
				i++;
			}
		}
	}
	return arrival_times;
}

double fcfs_awt(vector<pair<int,int> > &processes)
{
	int n = processes.size();
	vector<int> wait_times;
	// vector<int> turnaround_times;
	int total_turnaround_time = processes[0].second;
	wait_times.push_back(0);
	for(int i=1;i<n;i++){
		/*
			For a given process i, waittime
			wt[i] = (bt[0] + bt[1] +...... bt[i-1]) - at[i]
			i.e, wt[i+1] = wt[i] + at[i] + bt[i] - at[i+1]
			or, wt[i] = wt[i-1] + at[i-1] + bt[i-1] -at[i]
			Turn around time of a process = wait time + burst time.
		*/
		int wait_time = wait_times[i-1] + processes[i-1].first + processes[i-1].second - processes[i].first;
		wait_times.push_back(wait_time);
		total_turnaround_time += (wait_time + processes[i].second);
	}
	// for(int i=0;i<n;i++){
	// 	cout<<wait_times[i]<<" ";
	// }
	// cout<<endl<<total_turnaround_time<<endl;
	return (double)total_turnaround_time/(double)n;

}

// bool operator<(pair<int,int> a,pair<int,int> b)
// {
// 	return a.second>b.second;
// }
bool compare(pair<int,int> a,pair<int,int> b)
{
	return a.second>b.second;
}

double non_preemptive_sjf_atn(vector<pair<int,int> > p)
{
	// sort(p.begin(),p.end(),sort_sjf);

	int turnaround_time = 0;

	int current_time = 0;

	priority_queue<pair<int,int>, vector<pair<int,int> >,bool(*)(pair<int,int>,pair<int,int>)> heap(compare);
	// priority_queue<pair<int,int> > heap;
	/*For any given prcess, the turnaround time is deined as the total
	time from the arrival till the end of completion
	The variable current_time is essentially the sum of all the CPU bursts
	and for each process, we subtarct the completion time of the ith process
	from the arrival time to finally end up with the turnaround time for that 
	particular process.*/
	int n = p.size();
	int i=-1;
	int j=0;
	while(++i<n)
	{
		
		for(;j<n && p[j].first<=current_time;j++)
		{
			heap.push(p[j]);
		}

		pair<int,int> curr = heap.top();
		heap.pop();
		current_time += curr.second;
		turnaround_time += current_time - curr.first;
	}

	//Then we return the average
	return (double)turnaround_time/(p.size());
}

// double round_robin(vector<int> &arrival_times, vector<int> &burst_times)
double round_robin(vector<pair<int,int> > &processes)
{
	int delta = 2;
	queue<pair<pair<int,int>, int> > jobs;
	int n = processes.size();
	int current_time = 0;
	int i=n;
	int j=0;
	int total_turnaround_time = 0;
	while(i){
		for(;j<n && processes[j].first <= current_time;j++){
			// cout<<"pushing "<<j+1<<" at "<<current_time<<endl;
			jobs.push(pair<pair<int,int>, int> (processes[j],processes[j].second));
		}
		if(!jobs.empty()){
			pair<pair<int,int>, int> curr = jobs.front();
			jobs.pop();
			if(curr.second > delta){
				// cout<<"current_time = "<<current_time<<endl;
				// cout<<curr.first.first<<" "<<curr.first.second<<" "<<curr.second<<endl;
				curr.second -= delta;
				current_time += delta;
				for(;j<n && processes[j].first <= current_time;j++){
					// cout<<"pushing "<<j+1<<" at "<<current_time<<endl;
					jobs.push(pair<pair<int,int>, int> (processes[j],processes[j].second));
				}
				jobs.push(curr);
			}
			else if(curr.second > 0){
				// cout<<"comp "<< curr.second<<endl;
				// cout<<"current_time = "<<current_time<<endl;
				// cout<<curr.first.first<<" "<<curr.first.second<<" "<<curr.second<<endl;
				current_time += curr.second;
				total_turnaround_time += (current_time - curr.first.first);
				curr.second = 0;
				i--;
			}
		}
		else current_time++;
	}
	return (double)total_turnaround_time/(double)n;
}