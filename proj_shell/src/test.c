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

char** args;
int command_num =0;

int CommandParser(char * input);

int main() {
	char a[10] = "a b;b;d;g";
    CommandParser(a);
    return 0;
}

int CommandParser(char* input){
	//줄 단위로 파싱한다.
	char *parser = NULL;
    int commands_counter = 0, args_counter=0, index_counter = 0, i=0;
	char** commands = (char**)malloc(INITIAL_MAX_COMMAND_NUM * sizeof(char*));
	int commands_size = INITIAL_MAX_COMMAND_NUM;
	char** arguments = (char**)malloc(INITIAL_MAX_ARG_NUM * sizeof(char*));
	int arguments_size = INITIAL_MAX_ARG_NUM;
	//int* command_index;
	char *tmp_command_store = (char*)malloc((strlen(input) + 10) * sizeof(char));
	//char tmp_command_store[100];
	if(commands == NULL) {
		//error handling when failed to allocate memory
		exit(0);
	}
	parser = strtok(input, ";");
	while(parser != NULL) {
		if(commands_counter > commands_size) {
			if((commands = (char**)realloc(commands, 2 * commands_size * sizeof(char*))) == NULL) {
				printf("Failed to allocate enough memory: Too many commands");
				exit(0);
			};
			commands_size *= 2;
		}
		commands[commands_counter] = (char*)malloc((strlen(parser) + 10) * sizeof(char));
		strncpy(commands[commands_counter], parser, strlen(parser));
		commands[commands_counter][strlen(parser)] = '\0';
		commands_counter++;
		parser = strtok(NULL, ";");
	}
	//this is for test
	commands_counter = 4;
	//char *commands[4] = {"a b", "b", "c", "d"};
	//

	//free(input);
	command_num = commands_counter;
	int *command_index = (int*)malloc(commands_counter * sizeof(int));
	//int command_index[] = {0,2,4,6};

	for(i=0; i<commands_counter; i++) {
		strcpy(tmp_command_store, commands[i]);
		tmp_command_store[strlen(commands[i])] = '\0';
		if((parser = strtok(tmp_command_store, "\n\t ")) == NULL){
			printf("Error: meaningless space detected\n");
			exit(0);
		}
		arguments[args_counter] = (char*)malloc((strlen(parser) + 10) * sizeof(char));
		strncpy(arguments[args_counter], parser, strlen(parser));
		arguments[args_counter][strlen(parser)] = '\0';
		command_index[index_counter] = args_counter;
		index_counter++;

		args_counter++;
		while((parser = strtok(NULL, " ")) != NULL) {
		//for(i=0; i<4; i++){
			//parser = strtok(NULL, "\n\t ");
			printf("%s\n", parser);
			if(args_counter >= arguments_size) {
				if((arguments = (char**)realloc(arguments,  2 * arguments_size * sizeof(char*))) == NULL) {
					//error handling when failed to reallocate
					printf("Failed to allocate enough memory: Too many arguments");
					//exit(0);
				}
				arguments_size *= 2;
			}
			arguments[args_counter] = (char*)malloc((strlen(parser) + 10) * sizeof(char));
			
			strncpy(arguments[args_counter], parser, strlen(parser));
			arguments[args_counter][strlen(parser)] = '\0';
			args_counter++;
		}
		arguments[args_counter] = NULL;
		args_counter++;
		tmp_command_store[0] = '\0';
	}
	for(i=0; i<commands_counter; i++){
		if(commands[i]!=NULL) {
			//free(commands[i]);
		}
	}
	if(strlen(tmp_command_store) > 0){
		//free(tmp_command_store);
	}
	//free(commands);
	args = arguments;
	return 1;
}