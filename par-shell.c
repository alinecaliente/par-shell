//
// Fibonacci application, version 1
// Sistemas Operativos, DEI/IST/ULisboa 2015-16
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include "list.h"
#include "commandlinereader.h"

#define MAX_ARGUMENTS 6
#define MAXPAR 4 //max child processes running simultaneously
#define TAMMSG 128

typedef struct Args{
	list_t 	*list;
	int 	numChildren;
	bool	exitCall;
} Args;

Args args;
pthread_mutex_t semArgs;
pthread_cond_t wakeMonitor, countChild;

char line[1024];
int nProc;
int totalTime, executionTime;
time_t starttime, endtime;
FILE *fp;
pthread_t tid;
int pipeIn;
char* inPipe;
list_t *terminals;

void shell_exit(){
	//Destruir todos os terminais abertos.
	close(pipeIn);
	unlink(inPipe);
	//kill_terminals(terminals);

	pthread_mutex_lock(&semArgs);
	args.exitCall = true;
	my_cond_signal(&wakeMonitor);
	pthread_mutex_unlock(&semArgs);

	if(pthread_join(tid, NULL) == 0){
		printf("Monitoring thread terminated.\n");
	}
	else {
		printf("Could not join thread!\n");
		exit(EXIT_FAILURE);
	}

	if(pthread_mutex_destroy(&semArgs) != 0){
		printf("Mutex destruction error!");
		exit(EXIT_FAILURE);
	}

	if(pthread_cond_destroy(&wakeMonitor) != 0){
		printf("Condition destruction error!");
		exit(EXIT_FAILURE);
	}

	if(pthread_cond_destroy(&countChild) != 0){
		printf("Condition destruction error!");
		exit(EXIT_FAILURE);
	}

	lst_print(args.list);
	lst_destroy(args.list);

	fclose(fp);


	exit(0);

}

int my_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mut){
	if(pthread_cond_wait(cond, mut) != 0){
		printf("Condition wait error!\n");
		exit(EXIT_FAILURE);
	}
}

int my_cond_signal(pthread_cond_t *cond){
	if(pthread_cond_signal(cond) != 0){
		printf("Condition signal error!\n");
		exit(EXIT_FAILURE);
	}
}

void *childMonitor(){

	int status;
	int i = 0;
	int savePID;

	while(1){
		pthread_mutex_lock(&semArgs);
		if(args.numChildren == 0 && args.exitCall == false){
			my_cond_wait(&wakeMonitor, &semArgs);
			pthread_mutex_unlock(&semArgs);	
			continue;
		} else if(args.numChildren == 0 && args.exitCall == true){
			pthread_mutex_unlock(&semArgs);
			break;
		} else {
			pthread_mutex_unlock(&semArgs);
			savePID=wait(&status);
			endtime = time(NULL);

			if(savePID<=0)					
				perror("Child process terminated anormally."); 
			else{
				pthread_mutex_lock(&semArgs);
				update_terminated_process(args.list, savePID, WEXITSTATUS(status), endtime);
		
				nProc++;
				executionTime = search_exec_time(args.list,savePID);
				totalTime = totalTime + executionTime;
				fprintf(fp, "iteracao %d\n", nProc);
				fprintf(fp, "pid: %d execution time: %d s\n", savePID, executionTime);
				fprintf(fp, "total execution time: %d s\n", totalTime);
				fflush(fp);

				args.numChildren--;
				my_cond_signal(&countChild);
				pthread_mutex_unlock(&semArgs);
			}
		}
	}
	pthread_exit(NULL);
}

int main (int argc, char** argv) {

	char** argVector = (char**)malloc(6*sizeof(char*));
	int numargs, status;
	int i = 0;
	//pthread_t tid; //tarefa monitora
	char* arg;

	args.list = lst_new();
	args.numChildren = 0;
	args.exitCall = false;

	//--------------------------Reads file and seeks n(iter) and total time
	fp = fopen ("log.txt", "a+");
	if(fp == NULL){
		perror("Error opening the file!");
	}

	while(fgets(line, 80, fp) != NULL) {		
		sscanf (line, "iteracao %d\n", &nProc);
		sscanf (line, "total execution time: %d s\n", &totalTime);		
	}

	if(pthread_cond_init (&wakeMonitor, 0) != 0){
		printf("Condition initilization error!\n");
		exit(EXIT_FAILURE);
	}

	if(pthread_cond_init (&countChild, 0) != 0){
		printf("Condition initilization error!\n");
		exit(EXIT_FAILURE);
	}

	if(pthread_mutex_init(&semArgs, 0) != 0){
		printf("Mutex initilization error!\n");
		exit(EXIT_FAILURE);
	}
	
	if(pthread_create(&tid, 0, childMonitor, 0) == 0){
		printf("Monitoring thread created.\n");
	}
	else {
		printf("Thread creation error!\n");
		exit(EXIT_FAILURE);
	}

	//------------------------Pipe creation and opening - stdin
	inPipe = "/tmp/par-shell-in";
	char bufferIn[TAMMSG];	//receptor pipe
	char bufferOut[TAMMSG]; //for stats
	unlink(inPipe);

	if(mkfifo(inPipe, S_IWUSR | S_IRUSR) == -1)
		perror("Pipe creation error");

	pipeIn = open(inPipe, O_RDONLY);
	if(pipeIn == -1)
		perror("Pipe opening error");

	//-------------------------Redirecting stdin to pipe
	close(0);	//CHECK ERR
	dup(pipeIn);	//CHECK ERR


	while(1){	
		numargs = readLineArguments(argVector, MAX_ARGUMENTS, bufferIn, sizeof(bufferIn));

		signal(SIGINT, shell_exit);

		if(numargs<=0){ 
			//printf("Empty command!\n");
			continue;
		}
		if (!strcmp(argVector[0], "pid")){
			int pid_term = atoi(argVector[1]);
			//insert_new_terminal(terminals, pid_term);
			continue;
		}
		if (!strcmp(argVector[0], "exit")){
			shell_exit();
			break;
		}
		if(!strcmp(argVector[0], "stats")){
			char statsPipe[128];
			sprintf(statsPipe,"/tmp/par-shell-in-%s",argVector[1]);
			
			//--------------------------------------Opens stats pipe
			int pipeS = open(statsPipe, O_WRONLY);
			if(pipeS == -1)
				perror("Pipe opening error");

			memset(bufferOut, 0, TAMMSG);
			//--------------------------------------Gets response
			pthread_mutex_lock(&semArgs);
			int nIter, totalExec;

			sscanf (line, "total execution time: %d s\n", &totalExec);		

			sprintf(bufferOut,"Child processes executing:%d\nTotal time of execution:%d\n", args.numChildren, totalExec);
			pthread_mutex_unlock(&semArgs);

			int m = write(pipeS, bufferOut, strlen(bufferOut)+1);	//CHECK ERR
			memset(bufferOut, 0, TAMMSG);
			close(pipeS);
			unlink(statsPipe);
			continue;
		}
		else {		
			pthread_mutex_lock(&semArgs);
			while(!(args.numChildren < MAXPAR))
				my_cond_wait(&countChild,&semArgs);
			pthread_mutex_unlock(&semArgs);
			
					
			int PID = fork();
			starttime = time(NULL);
			if(PID == -1)
				perror("Fork Error!");
			else if(PID==0){
				
				char childFileStr[25];
				sprintf(childFileStr,"par-shell-out-%d.txt",getpid());

				int childFile = open(childFileStr, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
				if(childFile == -1){
					perror("Error opening child file!");
				}
				close(1);			//closes stdout
				dup(childFile);
				
				execv(argVector[0],argVector); 	
				perror("Executable not found!");
				exit(-1);

			} else {
				pthread_mutex_lock(&semArgs);				
				insert_new_process(args.list, PID, starttime);
				args.numChildren++;
				my_cond_signal(&wakeMonitor);
				pthread_mutex_unlock(&semArgs);
				continue;			
		
			}			
		 }

	}


}


