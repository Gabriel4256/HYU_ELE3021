/**
 * this file is a simple & lightweight linux shell implementation
 * 
 * @author SungHwan Shim
 * @since 2019-03-21
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define INITIAL_STORE_SIZE 256
#define INITIAL_COMMAND_SIZE 16
#define INITIAL_ARG_NUM 2

char** CommandParser(char* input){
	//줄 단위로 파싱한다.
	char *big_parser, *small_parser;
    int counter = 0;
    char** command_infos = (char**)malloc(INITIAL_COMMAND_SIZE * sizeof(char*));
	if(command_infos == NULL) {
		//error handling when failed to allocate memory
		exit(0);
	}

	big_parser = strtok(input, " ; ");
	while(big_parser!=NULL) {
		//big parser는 한 줄을 " ; "을 기준으로 잘라 command+argument 단위로 분리
		if(small_parser = strtok(big_parser, " ") == NULL) {
            exit(0);
        }
        command_infos[counter] = big_parser;
		counter++;
		command_infos[counter] = small_parser;
		counter++;

		while(small_parser = strtok(NULL, " ") !=NULL) {
			//argument를 분리
			//TODO: do the rest part of this
			command_infos[counter] = small_parser;
			counter++;
			if(counter >= sizeof(command_infos) / sizeof(char*)){
				if(realloc(command_infos, 2* sizeof(command_infos)) == NULL) {
					//error handling when failed to reallocate
					exit(0);
				}
			}
		}
		command_infos[counter] = NULL;
		counter++;
		big_parser = strtok(NULL, " ; ");
	}
	return command_infos;
}

void ForkAndExec(char** commands) {
	int i=0;
	pid_t pid;
	int counter = 0;
	int* command_index = (int*)malloc(256*sizeof(int));
	for(i=0; i<256; i++){
		command_index[i] = -1;
	}
	for(i=0; i<sizeof(commands)/sizeof(char*); i++){
		if(commands[i] == NULL) {
			if(counter >= sizeof(command_index) / sizeof(int)){
				if(realloc(command_index, 2 * counter * sizeof(int)) == NULL) {
					//error handling when failed to reallocate memory
					exit(0);
				}
			}
			command_index[counter] = i+1;
			counter++;
		}
	}
	for(i = 0; i<counter; i++;) {
		pid = fork();
		if(pid < 0){
			//자식 프로세스 생성 실패
			printf("failed to create a child process");
		}
		else if(pid == 0) {
			//자식 프로세스에서 수행할 작업
			i = 0;
			if(execvp(commands[command_index[i]], &commands[command_index[i]]) == -1) {
				// error handling when failed to execute command
				exit(0);
			}
			exit(0);
		}
	}

	//부모 프로세스에서는 모든 자식 프로세스가 끝나기를 기다린다.
	wait(NULL);
	return;	
}

int main(int argc, char *argv[]) {
	char *tmp_store;
	int i=0;
    tmp_store = (char*)malloc(INITIAL_STORE_SIZE * sizeof(char));
    int cur_length = INITIAL_STORE_SIZE; 
	if (argc == 1){
		//interactive mode
		while(1) {
            tmp_store[0] = '\0'; //initialization
			if(fgets(tmp_store, sizeof(tmp_store) / sizeof(char), stdin) == NULL){
		        //error handling for IO	
    			return NULL;
			}
			while (tmp_store[strlen(tmp_store)-1] != '\n') {
				//tmp_store에 한 줄을 다 저장하지 못했을 경우
                realloc(tmp_store, 2 * cur_length * sizeof(char)); 
			    if(fgets(tmp_store[cur_length-1], cur_length+1, stdin) == NULL) {
                    return NULL;
                }
                cur_length *= 2;
			}
			ForkAndExec(CommandParser(tmp_store));
            free(tmp_store);	
		}
	}
	else if (argc == 2){
		//batch mode
	}
	else {
		printf("Invalid format");
	}
	return 0;
}


