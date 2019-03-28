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

char **ParseCommand(char *input);
void ForkAndExec(char **commands_and_arguments);
char* LeftRightTrim(char *input);

int g_commands_counter = 0; // ParserCommand함수에서 처리한 input 안에 들어있는 command의 개수
int *g_commands_index = NULL;// ParseCommand함수에서 return하는 commands_and_arguments 배열에서 command가 들어있는 index들을 저장

/**
 *	이 함수는 char배열 문자열의 앞, 뒤 공백('', '\n', '\t', '\r', '\f', '\v')을 없애는 함수입니다.
 *  공백 여부를 판단하기 위해 isspace 함수를 사용했으며 속도를 빠르게 하기 위해 원본 문자열을 변형시키는 방식을 택했습니다.
 *	@param[in] input 앞, 뒤 공백을 없애고자 하는 문자열
 *  @return	input을 trim한 문자열을 담고 있는 배열의 주소값을 반환
 */
char* LeftRightTrim(char* input) {
	int i = 0, start_index = 0;
	if (input == NULL) {
		//input has no string
		printf("Trim error: empty input");
		return NULL;
	}

	//left side trim
	//문자열의 앞쪽부터 시작하여 공백이 있는 부분을 넘어서부터 문자열이 시작되도록 start_index 값을 증가시킨다.
	for (i=0; i<strlen(input); i++) {
		if (isspace(input[i])) {
			start_index++;
		}
		else {
			break;
		}
	}

	//right side trim
	//문자열의 뒤쪽부터 시작하여 공백문자가 끝날때까지 공백문자를 NULL값으로 바꾼다.
	for (i=strlen(input) - 1; i>=0; i--) {
		if(isspace(input[i])) {
			input[i] = '\0';
		}
		else {
			break;
		}
	}
	return input + start_index;
}

/**
 *	이 함수는 char 배열에 담겨있는 문자열에서 command와 argument들을 parsing하여 execvp 함수가 처리하기
 * 	쉬운 형태로 변환시켜 return 한다.
 *	@param[in] input parsing하고자 하는 command와 argument 정보를 포함하는 문자열
 *	@return 
 */ 

char** ParseCommand(char* input){
	char *parser; // strtok함수에서 parser로 쓸 변수
    int args_counter=0, index_counter = 0, i=0; 
	int num_of_commands, num_of_commands_and_args;
	char** commands = NULL; // 1차적으로 parsing된 command chunks (command + arguments) 들에 대한 정보를 임시적으로 저장
	char** commands_and_arguments = NULL; // command들과 argument들을 담을 배열
	//char *tmp_command_store = (char*)malloc((strlen(input) + 1) * sizeof(char)); 
	int semicolon_flag = 0, space_or_semicolon_flag = 0; //input의 전체 command, argument 수를 세는 작업에서 필요한 변수들
	g_commands_counter = 0;

	if (input == NULL) {
		//error handling when input is NULL
		exit(0);
	}

	// input의 전체 command, argument 수를 세는 작업
	// 연속해서 나타나는 공백과 세미콜론을 체크하면서 num_of commands(총 command 개수) , num_of_commands_and_args(총 argument 개수) 값을 구한다.
	input = LeftRightTrim(input);
	num_of_commands = 1;
	num_of_commands_and_args = 1;
	for(i=0; i<strlen(input); i++) {

		if (isspace(input[i] || input[i] == ';')) {
			space_or_semicolon_flag = 1;
		}
		while (isspace(input[i]) || input[i] == ';') {
			if (input[i] == ';') {
				semicolon_flag = 1;
			}
			i++;
		}
		if(semicolon_flag) {
			semicolon_flag = 0;
			space_or_semicolon_flag = 0;
			num_of_commands++;
			num_of_commands_and_args++;
		}
		else if (space_or_semicolon_flag) {
			space_or_semicolon_flag = 0;
			num_of_commands_and_args++;
		}
	}
	
	g_commands_index = (int*)malloc(num_of_commands * sizeof(int)); //command 수 만큼 할당
	commands_and_arguments = (char**)malloc(num_of_commands_and_args * sizeof(char*)); //command + argument 수 만큼 할당
	commands = (char**)malloc(num_of_commands * sizeof(char*)); //command 개수 만큼의 공간을 할당
	for (i=0; i<num_of_commands_and_args; i++) {
		commands_and_arguments[i] = NULL;
	} 

	// input에서 (command  + arguments) 단위를 parsing해서 commands배열에 순서대로 저장
	parser = strtok(input, ";");
	while (parser != NULL) {
		commands[g_commands_counter] = (char*)malloc((strlen(parser) + 1) * sizeof(char));
		strncpy(commands[g_commands_counter], parser, strlen(parser));
		commands[g_commands_counter][strlen(parser)] = '\0';
		g_commands_counter++;
		parser = strtok(NULL, ";");
	}

	// 
	for (i=0; i<g_commands_counter; i++) {
		if ((parser = strtok(commands[i], "\n\t\r\f\v ")) == NULL) {
			//printf("Error: meaningless space detected between semicolons\n");
			g_commands_counter--;
			continue;
		}
		commands_and_arguments[args_counter] = (char*)malloc((strlen(parser) + 1) * sizeof(char));
		strncpy(commands_and_arguments[args_counter], parser, strlen(parser));
		commands_and_arguments[args_counter][strlen(parser)] = '\0';
		g_commands_index[index_counter] = args_counter;
		index_counter++;

		args_counter++;
		while ((parser = strtok(NULL, "\n\t\r\f\v ")) != NULL) {
			commands_and_arguments[args_counter] = (char*)malloc((strlen(parser) + 1) * sizeof(char));
			
			strncpy(commands_and_arguments[args_counter], parser, strlen(parser));
			commands_and_arguments[args_counter][strlen(parser)] = '\0';
			args_counter++;
		}
		commands_and_arguments[args_counter] = NULL;
		args_counter++;
	}
	for (i=0; i<g_commands_counter; i++) {
		//if (strlen(commands[i]) > 0) {
			free(commands[i]);
		//}
	}
	//free(tmp_command_store);
	//free(commands);
	//g_commands_counter = num_of_commands-1;
	return commands_and_arguments;
}

void ForkAndExec(char **commands_and_arguments) {
	int i = 0,j = 0;
	pid_t pid;

	for (i=0; i<g_commands_counter; i++) {
		pid = fork();
		if (pid < 0) {
			// 자식 프로세스 생성에 실패했을 때
			printf("failed to create a child process\n");
		}
		else if (pid == 0) {
			// 자식 프로세스에서 수행할 작업 
			if (execvp(commands_and_arguments[g_commands_index[i]], commands_and_arguments + g_commands_index[i]) == -1) {
				// 명령 실행에 실패했을 때
				printf("failed to execute command: %s\n", commands_and_arguments[g_commands_index[i]]);
			}
			exit(0);
		}
	}
	//free(commands_and_arguments);
	//free(command_index);
	//부모 프로세스에서는 모든 자식 프로세스가 끝나기를 기다린다.
	for (i=0; i<g_commands_counter; i++) {
		wait(NULL);
	}
	for (i=0; i<2 * g_commands_counter; i++) {
		if (commands_and_arguments[i]!=NULL){
			free(commands_and_arguments[i]);
		}
	}
	return;	
}

int main(int argc, char *argv[]) {
	//char *tmp_store;
	char j;
	int i=0;
	int cur_length = INITIAL_MAX_INPUT_SIZE;
	//char *tmp_store = (char*)malloc(INITIAL_MAX_INPUT_SIZE * sizeof(char));
	char tmp_store[1000];
	if (argc == 1){
		//tmp_store =  (char*)malloc(INITIAL_MAX_INPUT_SIZE * sizeof(char));
		//interactive mode
		while (1) {
			printf("prompt> ");
			g_commands_counter = 0;
            tmp_store[0] = '\0'; //initialization
			if (fgets(tmp_store, cur_length, stdin) == NULL) { //tmp_store에 사용자 입력을 저장
		        //error handling for IO
				printf("error occured while getting user input in interacive mode\n");	
    			return -1;
			}
			
			while (tmp_store[strlen(tmp_store)-1] != '\n') {
				//tmp_store에 한 줄을 다 저장하지 못했을 경우
                //tmp_store = (char*)realloc(tmp_store, 2 * cur_length * sizeof(char));
			    if (fgets(&tmp_store[strlen(tmp_store)], cur_length * sizeof(char) , stdin) == NULL) {
				
                    return -1;
                }
                cur_length *= 2;
			}
			if (strcmp(LeftRightTrim(tmp_store), "quit") == 0) { 
				//quit을 입력받으면 종료; 앞뒤 공백은 trim해서 비교
				return 0;
			}
			//free(tmp_store);
			ForkAndExec(ParseCommand(tmp_store));	
		}
	}
	else if (argc == 2) {
		//batch mode
		FILE *fp = fopen(argv[1], "r");
		
		//파일을 여는데 실패했을 때
		/*while(!feof(fp)){
			printf("%s",fgets(tmp_store, sizeof(tmp_store), fp));		
		}*/

		
		while (!feof(fp)) {
			char tmp_store[10000];
			cur_length = INITIAL_MAX_INPUT_SIZE;
			//tmp_store = (char*)malloc(INITIAL_MAX_INPUT_SIZE * sizeof(char));
			tmp_store[0] = '\0';
			if (fgets(tmp_store, sizeof(tmp_store), fp) == NULL) {
				//파일을 읽다가 에러 발생
				if (feof(fp)) {
					//break;
				}
				printf("error occured while reading a file in batch mode\n");
				return -1;
			}
			while (tmp_store[strlen(tmp_store)-1] != '\n' && !feof(fp)) {
				if (strcmp(LeftRightTrim(tmp_store), "quit") == 0) {
					return 0;
				}
				if(feof(fp)){
					return 0;
				}
				//tmp store에 file의 한 줄을 다 저장하지 못했을 경우
				//tmp_store = (char*)realloc(tmp_store, 2*cur_length * sizeof(char));
				if (fgets(&tmp_store[cur_length-1], cur_length * sizeof(char) , fp) == NULL) {
					if(feof(fp)) {
						break;
					}
					else {
						printf("error occured while reading a file in batch mode\n");
                    	return -1;						
					}
                }
				cur_length *= 2;
			}
			//printf("%s", tmp_store);
			/*if (strcmp(LeftRightTrim(tmp_store), "quit") == 0) {
				return 0;
			}*/
			printf("%s\n", tmp_store);
			ForkAndExec(ParseCommand(tmp_store));
			free(g_commands_index);
			//free(tmp_store);
		}
		fclose(fp);
	}
	else {
		printf("Invalid format");
	}
	// if(command_index){
	// 	free(command_index);
	// }
	//free(tmp_store);
	return 0;
}