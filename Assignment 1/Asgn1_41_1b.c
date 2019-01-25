#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAXSIZE 100// MAXSIZE of input argument and the number of arguments
int main()
{
	char cmd[MAXSIZE];//String to store the entered text
	while(1){

		fgets(cmd,MAXSIZE,stdin);//obtain the command to be run

		cmd[strlen(cmd)-1] = '\0';// Modify the \n at the end of fgets to \0

		if(strcmp(cmd,"quit")==0){// If quit is entered break the loop and exit the parent
			break;
		}

		// Assign the args array. args array will store all the arfuments as a list of strings
		char ** args = (char**)malloc((MAXSIZE+1)*sizeof(char*));
		int i;

		char* token = strtok(cmd," ");// separate the ' ' separated arguments in the input \
		string to create a list of arguments
		i=0;
		// At each point, if token is not null, assign the length of the string to the args[i] and \
		then copy the token to args[i] and assign the next token to the variable 'token'
		while(token!=NULL){
			args[i] = (char*)malloc((strlen(token)+1)*sizeof(char));
			sprintf(args[i],"%s",token);
			token = strtok(NULL," ");// This keeps setting token to the next argument
			i++;
		}

		args[i] = NULL;// the list of arguments must be NULL terminated

		if(fork()==0){// create child process
			execvp(args[0],(char * const *)args);// assign the child process the new commands
			printf("Invalid command\n");// if the new process fails
			return 0;
		}
		else{// the parent process
			int status;// variable to store the status of the child's return
			pid_t pid;// variable to store the pid of the child process terminated
			pid = wait(&status);// wait for the child to terminate
		}
	}
	return 0;
}

