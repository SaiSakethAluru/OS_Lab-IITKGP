#include <iostream>
#include <bits/stdc++.h>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <fstream>
#define LAMBDA 0.4
using namespace std;
vector<int> cpu_bursts(int n);
vector<int> generate_exponential(int n);

int main()
{
	int n;
	cin>>n;
	vector<int> burst_times = cpu_bursts(n);
	vector<int> arrival_times = generate_exponential(n);
	vector<int> hist_uni (20,0);
	vector<int> hist_exp (10,0);
	ofstream fout;
	fout.open("cpu_times.txt");
	for(int i=0;i<n;i++){
		fout<<arrival_times[i]<<"\t"<<burst_times[i]<<endl;
	}
	fout.close();
	for(int i=0;i<n;i++){
		hist_uni[burst_times[i]-1]++;
		hist_exp[arrival_times[i]]++;
	}
	for (int i=0; i<20; ++i) {
    	cout << i+1<<" "<<hist_uni[i]<<" :"<< std::string(hist_uni[i],'*') << std::endl;
  	}
	for (int i=0; i<10; ++i) {
    	cout << i+1<<" "<<hist_exp[i]<<" :"<<std::string(hist_exp[i],'*') << std::endl;
  	}
  	return 0;
}

vector<int> cpu_bursts(int n)
{
	vector<int> v;
	srand(time(NULL));
	for(int i=0;i<n;i++){
		v.push_back(rand()%20+1);
	}
	return v;
}

// vector<int> generate_exponential(int n)
// {
// 	default_random_engine generator;
// 	exponential_distribution<double> distribution(LAMBDA);
// 	vector<int> p(n);
// 	float max_val = -1;
// 	for (int i=0; i<n; ++i) {
// 	    double number = distribution(generator);
// 	    cout<<(int)(number*100)<<endl;
// 	    p.push_back((int)(number*100));
// 	    if(number > max_val)
// 	    	max_val = (number*100);
//   	}
//   	for(int i=0;i<n;i++){
//   		p[i] = ((float)p[i]/max_val)*10 ;
//   	}
//   	return p;
// }
vector<int> generate_exponential(int n)
{
	vector<int> exp_nums;
	srand(time(NULL));
	int i=0;
	while(i<n){
		double uni_rand = (float)rand() / (RAND_MAX + 1.0);
		if(uni_rand != 0){
			double val = -log(uni_rand) / LAMBDA;
			if(val <=10){
				exp_nums.push_back(val);
				i++;
			}
		}
	}
	return exp_nums;
}