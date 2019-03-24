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
#include <ctype.h>

#define INITIAL_MAX_INPUT_SIZE 20
#define INITIAL_MAX_COMMAND_NUM 4
#define INITIAL_MAX_COMMAND_LENGTH 16
#define INITIAL_MAX_ARG_NUM 2

int command_num = 0;
char** args;
int *command_index;

char* LeftRightTrim(char* input) {
	int i = 0, start_index = 0, end_index = 0;
	if(input == NULL) {
		//input has no string
		printf("Trim error: empty input");
		return NULL;
	}

	//left side trm
	for(i=0; i<strlen(input); i++) {
		if(isspace(input[i])) {
			start_index++;
		}
		else{
			break;
		}
	}

	//right side trim
	end_index = strlen(input) - 1;
	for (i=strlen(input) - 1; i>=0; i--) {
		if(isspace(input[i])) {
			input[i] = '\0';
			end_index -= 1;
		}
		else {
			break;
		}
	}
	return input + start_index;
}

int* CommandParser(char* input){
	//줄 단위로 파싱한다.
	char *parser;
    int commands_counter = 0, args_counter=0, index_counter = 0, i=0;
	char** commands = (char**)malloc(INITIAL_MAX_COMMAND_NUM * sizeof(char*));
	char** arguments = (char**)malloc(INITIAL_MAX_ARG_NUM * sizeof(char*));
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
				printf("Failed to allocate enough memory: Too many commands");
				exit(0);
			};
		}
		commands[commands_counter] = (char*)malloc((strlen(parser) + 1) * sizeof(char));
		strncpy(commands[commands_counter], parser, strlen(parser));
		commands[commands_counter][strlen(parser)] = '\0';
		commands_counter++;
		parser = strtok(NULL, ";");
	}
	//free(input);
	command_num = commands_counter;
	command_index = (int*)malloc(commands_counter * sizeof(int));
	for(i=0; i<commands_counter; i++) {
		strncpy(tmp_command_store, commands[i], strlen(commands[i]));
		tmp_command_store[strlen(commands[i])] = '\0';
		if((parser = strtok(tmp_command_store, "\n\t ")) == NULL){
			printf("Error: meaningless space detected\n");
			exit(0);
		}
		arguments[args_counter] = (char*)malloc((strlen(commands[i]) + 1) * sizeof(char));
		strncpy(arguments[args_counter], parser, strlen(parser));
		arguments[args_counter][strlen(parser)] = '\0';
		command_index[index_counter] = args_counter;
		index_counter++;

		args_counter++;
		while((parser = strtok(NULL, "\n\t ")) != NULL) {
			if(args_counter >= sizeof(arguments) / sizeof(char*)) {
				if((arguments = (char**)realloc(arguments,  (100 +args_counter) * sizeof(arguments))) == NULL) {
					//error handling when failed to reallocate
					printf("Failed to allocate enough memory: Too many arguments");
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
		if(sizeof(commands[i]) > 0) {
			free(commands[i]);
		}
	}
	if(sizeof(tmp_command_store) > 0){
		free(tmp_command_store);
	}
	free(commands);
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
				printf("failed to execute command: %s\n", args[command_index[i]]);
				exit(0);
			}
			exit(0);
		}
	}
	for(i=0; i<sizeof(args) / sizeof(char*); i++){
		if(args[i]){
			free(args[i]);
		}
	}
	free(args);
	free(command_index);
	//부모 프로세스에서는 모든 자식 프로세스가 끝나기를 기다린다.
	for(i=0; i<command_num; i++){
		wait(NULL);
	}
	return;	
}

int main(int argc, char *argv[]) {
	char tmp_store[512];
	char j;
	int i=0;
	int cur_length = INITIAL_MAX_INPUT_SIZE;
	//tmp_store = (char*)malloc(INITIAL_MAX_INPUT_SIZE * sizeof(char));


	if (argc == 1){
		//interactive mode
		while(1) {
			printf("prompt> ");
			command_num = 0;
            tmp_store[0] = '\0'; //initialization
			if(fgets(tmp_store, sizeof(tmp_store) / sizeof(char), stdin) == NULL){
		        //error handling for IO
				printf("error occured while getting user input in interacive mode\n");	
    			return -1;
			}
			
			while (tmp_store[strlen(tmp_store)-1] != '\n') {
				//tmp_store에 한 줄을 다 저장하지 못했을 경우
                //tmp_store = (char*)realloc(tmp_store, 2 * sizeof(tmp_store));
			    if(fgets(&tmp_store[strlen(tmp_store)], cur_length * sizeof(char) , stdin) == NULL) {
				
                    return -1;
                }
                cur_length *= 2;
			}
			if(strcmp(LeftRightTrim(tmp_store), "quit") == 0) {
				return 0;
			}
			//free(tmp_store);
			ForkAndExec(CommandParser(tmp_store));	
		}
	}
	else if (argc == 2){
		//batch mode
		FILE *fp = fopen(argv[1], "r");
		
		//파일을 여는데 실패했을 때
		/*while(!feof(fp)){
			printf("%s",fgets(tmp_store, sizeof(tmp_store), fp));		
		}*/

		while(1){
		//for(i=0; i<3; i++){
			if(fgets(&tmp_store[strlen(tmp_store)], sizeof(tmp_store) , fp) == NULL) {
				//파일을 읽다가 에러 발생
				if(feof(fp)){
					return 0;
				}
				printf("error occured while reading a file in batch mode\n");
				return -1;
			}
			while(tmp_store[strlen(tmp_store)-1] != '\n') {
				if(strcmp(LeftRightTrim(tmp_store), "quit") == 0) {
					return 0;
				}
				//tmp store에 file의 한 줄을 다 저장하지 못했을 경우
				//tmp_store = (char*)realloc(tmp_store, (cur_length + 100)  );
				if(fgets(&tmp_store[strlen(tmp_store)], cur_length * sizeof(char) , fp) == NULL) {
					if(feof(fp)){
						return 0;
					}
					printf("error occured while reading a file in batch mode\n");
                    return -1;
                }
				cur_length *= 2;
			}
			printf("%s", tmp_store);
			ForkAndExec(CommandParser(LeftRightTrim(tmp_store)));
			tmp_store[0] = '\0';
		}
		fclose(fp);
	}
	else {
		printf("Invalid format");
	}
	if(command_index){
		free(command_index);
	}
	//free(tmp_store);
	return 0;
}


