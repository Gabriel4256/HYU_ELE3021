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

typedef struct CommandInfo {
	char *command;
	char **arguments;
} CommandInfo;

CommandInfo** CommandParser(char* input){
	//줄 단위로 파싱한다.
	char *big_parser, *small_parser;
    int i=0, j=0, command_counter = 0, arg_counter = 0;
    CommandInfo** command_infos = (CommandInfo**)malloc(sizeof(CommandInfo*) * INITIAL_COMMAND_SIZE);
	
	big_parser = strtok(input, " ; ");
	while(big_parser!=NULL) {
		//big parser는 한 줄을 " ; "을 기준으로 잘라 command+argument 단위로 분리
		if(small_parser = strtok(big_parser, " ") == NULL) {
            exit(0);
        }
        CommandInfo tmp;
        tmp.command = small_parser;
		tmp.arguments = (char**)malloc(INITIAL_ARG_NUM * sizeof(char*));
        command_infos[command_counter] = &tmp;
		arg_counter = 0;
		
		while(small_parser = strtok(NULL, " ") !=NULL) {
			//argument를 분리
			//TODO: do the rest part of this
			small_parser = strtok(NULL, " ");
			arg_counter++;
			if(arg_counter > sizeof(tmp.arguments) / sizeof(char*)){
				realloc(tmp.arguments, 2* (arg_counter - 1));
			}
			tmp.arguments[arg_counter-1] = small_parser;
		}
		big_parser = strtok(NULL, " ; ");
	}
	return command_infos;
}

void ForkAndExec(CommandInfo** commands) {
	int i=0;
	for(i=0; i<sizeof(commands)/sizeof(CommandInfo); i++){
		if(commands[i] == NULL) {
			break;
		}
		pid_t pid;
		pid = fork();
		if(pid < 0){
			//자식 프로세스 생성 실패
			printf("failed to create a child process");
		}
		else if(pid == 0) { 
			//자식 프로세스에서 수행할 작업
			i = 0;
			
			while(execvp(commands[i]->command, commands[i]->arguments) == -1){
				if(i == 5){ // 실행에 실패하면 4번 더 시도하고 계속 실패하면 포기
					printf("failed to execute command");
					break;
				}
				i++;
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
                realloc(tmp_store, 2*cur_length); 
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


