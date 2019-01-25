#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#define ARR_SIZE 50		// size of initial arrays in process A,B,C
// function forward declarations
void random_number_generator(int arr[],int n,int seed);
void merge_two_arrays(int* arr_1,int* arr_2, int* merged_arr,int size1,int size2);
void quickSort(int arr[],int l,int r);
int partition(int arr[],int l, int h);
void swap(int* a, int* b);

int main()
{
	// create pipe between processes A and D
	int p_AD[2];
	if(pipe(p_AD)<0)
		exit(1);
	// create pipe between processes B and D
	int p_BD[2];
	if(pipe(p_BD)<0)
		exit(1);
	// create fork for A and B
	if(fork()== 0){		
		// create fork for A
		if(fork()==0){
			// initialise array A with random numbers with base seed as 0
			int arr_A[ARR_SIZE];
			random_number_generator(arr_A,ARR_SIZE,0);
			// sort the random array A
			quickSort(arr_A,0,ARR_SIZE-1);
			// write the array in the pipe for D to read
			write(p_AD[1],arr_A,sizeof(arr_A));
		}
		// create process B
		else{
			// initialise array B with random numbers with base seed as 1
			int arr_B[ARR_SIZE];
			random_number_generator(arr_B,ARR_SIZE,1);
			// sort the array B
			quickSort(arr_B,0,ARR_SIZE-1);
			// write the array to pipe for D to read
			write(p_BD[1],arr_B,sizeof(arr_B));
		}		
	}
	// forks for processes C,D and E
	else{
		// create pipe between C and E
		int p_CE[2];
		if(pipe(p_CE)<0)
			exit(1);
		// create process C
		if(fork()==0){
			// initialise array C with random numbers with base seed 2
			int arr_C[ARR_SIZE];
			random_number_generator(arr_C,ARR_SIZE,2);
			// sort the array C
			quickSort(arr_C,0,ARR_SIZE-1);
			// write the array to pipe for E to read
			write(p_CE[1],arr_C,sizeof(arr_C));	
		}
		// forks for D and E
		else{
			// create pipe between processes D and E
			int p_DE[2];
			if(pipe(p_DE)<0)
				exit(1);
			// create process D
			if(fork()==0){
				// read arrays sent by process A and B
				int arr_DA[ARR_SIZE],arr_DB[ARR_SIZE];
				int nbytesA = read(p_AD[0],arr_DA,sizeof(arr_DA));
				if(nbytesA < 0)
					exit(1);
				int nbytesB = read(p_BD[0],arr_DB,sizeof(arr_DB));
				if(nbytesB<0)
					exit(1);
				// merge the two arrays in sorted order
				int arr_D[2*ARR_SIZE];
				merge_two_arrays(arr_DA,arr_DB,arr_D,ARR_SIZE,ARR_SIZE);
				// write the sorted array to pipe for E to read
				write(p_DE[1],arr_D,sizeof(arr_D));
			}
			// create process E
			else{
				// read arrays in pipe from processes C and D
				int arr_EC[ARR_SIZE],arr_ED[2*ARR_SIZE],arr_E[3*ARR_SIZE];
				int nbytesC = read(p_CE[0],arr_EC,sizeof(arr_EC));
				if(nbytesC<0)
					exit(1);
				int nbytesD = read(p_DE[0],arr_ED,sizeof(arr_ED));
				if(nbytesD<0)
					exit(1);
				// merge the two arrays in sorted order
				merge_two_arrays(arr_EC,arr_ED,arr_E,ARR_SIZE,2*ARR_SIZE);
				int i;
				// print the sorted array of size 150
				printf("Sorted Array\n");
				for(i=0;i<3*ARR_SIZE;i++){
					printf("%d ",arr_E[i]);
				}
				printf("\n");
			}
		}
	}
	return 0;
}
/*
Function to generate random numbers array of size n.
integer seed is added to time now to determine the seed. this is different for each process
to ensure different array creation
*/
void random_number_generator(int arr[],int n,int seed)
{
	srand(time(0)+seed);
	int i;
	for(i = 0;i<n;i++){
		arr[i] = rand()%100+1;
	}
}
// function to swap integers determined by their pointers, used in quicksort partition
void swap(int* a, int* b)
{
	int temp  = *a;
	*a = *b;
	*b = temp;
}
// Quick sort partition function with first element as pivot
int partition(int arr[],int l, int h)
{
	int pivot = arr[l];
	int i = l;
	int j;
	for(j=l+1;j<=h;j++){
		if(arr[j]<=pivot){
			i++;
			swap(&arr[i],&arr[j]);
		}
	}
	swap(&arr[l],&arr[i]);
	return i;
}
// Recursive function for quick sort
void quickSort(int arr[],int l,int r)
{
	if(l<r){
		int p = partition(arr,l,r);
		quickSort(arr,l,p-1);
		quickSort(arr,p+1,r);
	}
}

/*
Function to merge two sorted arrays into a sorted array.
The inputs arrays are arr1 and arr2 and their sizes are size1 and size2
The output array is written to merged_arr parameter.
*/
void merge_two_arrays(int arr_1[],int arr_2[], int merged_arr[],int size1,int size2)
{
	int i=0,j=0,k=0;
	while(i<size1 && j<size2)
	{
		if(arr_1[i] < arr_2[j]){
			merged_arr[k++] = arr_1[i++];
		}
		else{
			merged_arr[k++] = arr_2[j++];
		}
	}
	if(i==size1){
		while(j<size2){
			merged_arr[k++] = arr_2[j++];
		}
	}
	else if(j==size2){
		while(i<size1){
			merged_arr[k++] = arr_1[i++];
		}
	}
}
