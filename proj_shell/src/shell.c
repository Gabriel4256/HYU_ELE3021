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
#include <sys/types.h>
#include <sys/wait.h>

#define INITIAL_MAX_INPUT_SIZE 20
#define INITIAL_MAX_COMMAND_NUM 4
#define INITIAL_MAX_COMMAND_LENGTH 16
#define INITIAL_MAX_ARG_NUM 2

int command_num = 0;
char** args;
int *command_index;

// typedef struct CommandInfo {
// 	char* command;
// 	char** args;
// } CommandInfo;

int* CommandParser(char* input){
	//줄 단위로 파싱한다.
	char *parser;
    int commands_counter = 0, args_counter=0, index_counter = 0, i=0;
	char** commands = (char**)malloc(INITIAL_MAX_INPUT_SIZE * sizeof(char*));
	char** arguments = (char**)malloc(INITIAL_MAX_INPUT_SIZE * sizeof(char*));
	//int* command_index;
    //CommandInfo** command_infos = (CommandInfo**)malloc(INITIAL_MAX_COMMAND_NUM * sizeof(CommandInfo*));
	char *tmp_command_store = (char*)malloc((strlen(input) + 1) * sizeof(char));
	if(commands == NULL) {
		//error handling when failed to allocate memory
		exit(0);
	}
	parser = strtok(input, ";");
	while(parser != NULL) {
		if(commands_counter > sizeof(commands) / sizeof(char*)) {
			if((commands = (char**)realloc(commands, 2 * sizeof(commands))) == NULL) {
				printf("failed to allocate memory");
				exit(0);
			};
		}
		//if(commands[commands_counter] == NULL){
			commands[commands_counter] = (char*)malloc((strlen(parser) + 1) * sizeof(char));
		//} 
		//else if(sizeof(commands[commands_counter]) <= strlen(parser) * sizeof(char)) {
		//	realloc(commands[commands_counter], strlen(parser) * sizeof(char)); 
		//} 
		strcpy(commands[commands_counter], parser);
		//command_infos[command_counter]->args = (char**)malloc(INITIAL_MAX_ARG_NUM * sizeof(char*));
		commands_counter++;
		parser = strtok(NULL, ";");
	}
	for(i=0; i<commands_counter; i++) {
		//commands[i] = strtok(commands[i], " \n\t");
	}
	//free(input);
	command_num = commands_counter;
	command_index = (int*)malloc(commands_counter * sizeof(int));
	for(i=0; i<commands_counter; i++) {
		strcpy(tmp_command_store, commands[i]);
		if((parser = strtok(tmp_command_store, "\n\t ")) == NULL){
			printf("Error: meaningless space detected\n");
			exit(0);
		}
		arguments[args_counter] = (char*)malloc((strlen(commands[i]) + 1) * sizeof(char));
		//strcpy(arguments[args_counter],commands[i]);
		strcpy(arguments[args_counter], parser);
		command_index[index_counter] = args_counter;
		index_counter++;

		args_counter++;
		//strcpy(commands[i],parser);co
		//strcpy(command_infos[i]->args[0], parser);
		while((parser = strtok(NULL, "\n\t ")) != NULL) {
			if(args_counter >= sizeof(arguments) / sizeof(char*)) {
				if((arguments = (char**)realloc(arguments,  (100 +args_counter) * sizeof(arguments))) == NULL) {
					//error handling when failed to reallocate
					exit(0);
				}
			}
			arguments[args_counter] = (char*)malloc((strlen(parser)+1) * sizeof(char));
			
			strcpy(arguments[args_counter], parser);
			args_counter++;
		}
		arguments[args_counter] = NULL;
		args_counter++;
	}
	for(i=0; i<commands_counter; i++){
		free(commands[i]);
	}
	free(tmp_command_store);
	//free(commands);
	args = arguments;
	return command_index;
}

void ForkAndExec(int* command_index) {
	int i = 0,j = 0;
	pid_t pid;

	for(i=0; i<command_num; i++){
		pid = fork();
		if(pid < 0) {
			// 자식 프로세스 생성에 실패했을 때
			printf("failed to create a child process\n");
		}
		else if(pid == 0) {
			// 자식 프로세스에서 수행할 작업 
			if(execvp(args[command_index[i]], args + command_index[i]) == -1) {
				// 명령 실행에 실패했을 때
				printf("failed to execute command\n");
				exit(0);
			}
			exit(0);
		}
	}
	for(i=0; i<sizeof(args) / sizeof(char*); i++){
		free(args[i]);
	}
	//free(args);
	//free(command_index);
	//부모 프로세스에서는 모든 자식 프로세스가 끝나기를 기다린다.
	wait(NULL);
	return;	
}

int main(int argc, char *argv[]) {
	char *tmp_store;
	char j;
	int i=0;
	int cur_length = INITIAL_MAX_INPUT_SIZE;
	//CommandInfo** command_infos = (CommandInfo**)malloc(INITIAL_MAX_COMMAND_NUM * sizeof(CommandInfo*));
	//command_infos[i] = (CommandInfo*)malloc(sizeof(CommandInfo));
	tmp_store = (char*)malloc(INITIAL_MAX_INPUT_SIZE * sizeof(char));


	if (argc == 1){
		//interactive mode
		while(1) {
			command_num = 0;
            tmp_store[0] = '\0'; //initialization
			if(fgets(tmp_store, sizeof(tmp_store) / sizeof(char), stdin) == NULL){
		        //error handling for IO
				printf("error occured while getting user input\n");	
    			return -1;
			}
			// while((j = getchar()) != '\n') {
			// 	if(strlen(tmp_store) > )
			// }
			while (tmp_store[strlen(tmp_store)-1] != '\n') {
				//tmp_store에 한 줄을 다 저장하지 못했을 경우
                tmp_store = (char*)realloc(tmp_store, 2 * sizeof(tmp_store));
			    if(fgets(&tmp_store[strlen(tmp_store)], cur_length * sizeof(char) , stdin) == NULL) {
				
                    return -1;
                }
                cur_length *= 2;
			}
			//free(tmp_store);
			ForkAndExec(CommandParser(tmp_store));	
		}
	}
	else if (argc == 2){
		//batch mode
	}
	else {
		printf("Invalid format");
	}
	free(command_index);
	free(tmp_store);
	return 0;
}


