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

#define MAX_ARGUMENTS 6
#define TAMMSG 128

int main (int argc, char** argv) {

	char** argVector = (char**)malloc(6*sizeof(char*));
	char bufferOut[TAMMSG];
	char bufferIn[TAMMSG]; //stats
	char* namedPipe;

	if(argv[1]==NULL){
		printf("Run par-shell with a pipe for input.\n");
		return 0;
	}
	else if(strcmp(argv[1], "par-shell-in")){
		printf("Run par-shell with correct pipe for input.\n");
		return 0;
	}
	else namedPipe = "/tmp/par-shell-in";


	int pipe = open(namedPipe, O_WRONLY);
	if(pipe == -1)
		perror("Pipe opening error");
	
	//--------------------------------------Sends pid to be used in SIGINT from par-shell.
	sprintf(bufferOut,"pid %d\n",getpid());
	int l = write(pipe, bufferOut, strlen(bufferOut));	//CHECK ERR
	memset(bufferOut, 0, TAMMSG);	

	while(1){
	
		fgets(bufferOut, sizeof(bufferOut), stdin);

		if(bufferOut==NULL){
			continue;
		}
		if(!strncmp(bufferOut, "exit-global", 11)){

			memset(bufferOut, 0, TAMMSG);
			strcpy(bufferOut, "exit\n");

			int m = write(pipe, bufferOut, strlen(bufferOut)+1); //CHECK ERR
			memset(bufferOut, 0, TAMMSG);

			break;
		}
		else if (!strncmp(bufferOut, "exit", 4)){
			printf("Exit from par-shell-terminal.\n");	//Par-shell deve matar o pipe desse terminal?
			break;
		}
		else if(!strncmp(bufferOut, "stats", 5)){

			char statsPipe[128];
			sprintf(statsPipe,"/tmp/par-shell-in-%d",getpid());
			unlink(statsPipe);

			if(mkfifo(statsPipe, S_IWUSR | S_IRUSR) == -1)
				perror("Pipe creation error");


			//--------------------------------------Sends stats request
			memset(bufferOut, 0, TAMMSG);
			sprintf(bufferOut,"stats %d\n",getpid());
			int n = write(pipe, bufferOut, strlen(bufferOut)+1); //CHECK ERR
			memset(bufferOut, 0, TAMMSG);
	

			//--------------------------------------Reads response
			int pipeS = open(statsPipe, O_RDONLY);
			if(pipeS == -1)
				perror("Pipe opening error");

			for(;;){
				int o = read(pipeS, bufferIn, strlen(bufferIn));
				if(o<=0) break;
				printf("%s",bufferIn);
			}	
	
			//close(pipeS);
			unlink(statsPipe);

			continue;
		}
		else{
			int p = write(pipe, bufferOut, strlen(bufferOut));	//CHECK ERR
			continue;
			

		}
	}
	close(pipe);	//CHECK ERR
	return 0;
}
