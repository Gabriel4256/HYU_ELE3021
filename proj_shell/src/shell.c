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

int *CommandParser(char* input);
void ForkAndExec(int* command_index);


int	g_commands_num = 0;
char** g_commands_and_arguments;

/**
 *	이 함수는 char배열 문자열의 앞, 뒤 공백('', '\n', '\t', '\r', '\f', '\v')을 없애는 함수입니다.
 *  공백 여부를 판단하기 위해 isspace 함수를 사용했으며 속도를 빠르게 하기 위해 원본 문자열을 변형시키는 방식을 택했습니다.
 *	@param[in] input 앞, 뒤 공백을 없애고자 하는 문자열
 *  @return	input을 trim한 문자열을 담고 있는 배열의 주소값을 반환
 */
char* LeftRightTrim(char* input) {
	int i = 0, start_index = 0, end_index = 0;
	if (input == NULL) {
		//input has no string
		printf("Trim error: empty input");
		return NULL;
	}

	//left side trm
	//문자열의 앞쪽부터 시작하여 공백이 있는 부분을 넘어서부터 문자열이 시작되도록 start_index 값을 증가시킨다.
	for (i = 0; i < strlen(input); i++) {
		if(isspace(input[i])) {
			start_index++;
		}
		else{
			break;
		}
	}

	//right side trim
	//문자열의 뒤쪽부터 시작하여 공백문자가 끝날때까지 공백문자를 NULL값으로 바꾼다.
	end_index = strlen(input) - 1;
	for (i = strlen(input) - 1; i >= 0; i--) {
		if (isspace(input[i])) {
			input[i] = '\0';
			end_index -= 1;
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
 *	@return g_commands_and_arguments 배열에서 command들의 index정보를 담고있는 int형 배열
 */ 
int* CommandParser(char* input){
	//줄 단위로 파싱한다.
	char *parser; // strtok함수에서 parser로 쓸 변수
    int commands_counter = 0, args_counter=0, index_counter = 0, i=0;
	int num_of_commands = 1, num_of_commands_and_args = 1;
	char** commands = NULL; // 1차적으로 parsing된 command chunks (command + arguments) 들에 대한 정보를 임시적으로 저장
	char** commands_and_arguments = NULL; // command들과 argument들을 담을 배열
	int* command_index;
	char *tmp_command_store = (char*)malloc((strlen(input) + 1) * sizeof(char));
	int semicolon_flag = 0, space_or_semicolon_flag = 0; //input의 전체 command, argument 수를 세는 작업에서 필요한 변수들
	if (input == NULL) {
		//error handling when failed to allocate memory
		exit(0);
	}

	// input의 전체 command, argument 수를 세는 작업
	// 연속해서 나타나는 공백과 세미콜론을 체크하면서 num_of commands(총 command 개수) , num_of_commands_and_args(총 argument 개수) 값을 구한다.
	input = LeftRightTrim(input);
	num_of_commands = 1;
	num_of_commands_and_args = 1;
	for (i = 0; i < (int)strlen(input); i++) {

		if (isspace(input[i]) || input[i] == ';') {
			space_or_semicolon_flag = 1;
		}
		while (isspace(input[i]) || input[i] == ';') {
			if (input[i] == ';') {
				semicolon_flag = 1;
			}
			i++;
		}
		if (semicolon_flag) {
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
	
	//command 개수 만큼의 공간을 할당
	commands = (char**)malloc(num_of_commands * sizeof(char*));

	//command + argument 수 만큼 할당
	commands_and_arguments = (char**)malloc(num_of_commands_and_args * sizeof(char*)); 

	// input에서 (command  + arguments) 단위를 parsing해서 commands배열에 순서대로 저장
	parser = strtok(input, ";");
	while (parser != NULL) {
		commands[commands_counter] = (char*)malloc((strlen(parser) + 10) * sizeof(char));
		strncpy(commands[commands_counter], parser, strlen(parser));
		commands[commands_counter][strlen(parser)] = '\0';
		commands_counter++;
		parser = strtok(NULL, ";");
	}

	g_commands_num = commands_counter;
	
	//commands_and_arguments 배열 안에서 command 문자열이 있는 곳의 index들을 담기 위한 공간을 할당
	command_index = (int*)malloc(num_of_commands * sizeof(int));  
	
	//commands안에 있는 (command + arguments) 덩어리를 하나씩 꺼내서 tmp_command_store에 임시적으로 옮기고
	//파싱하여 command들과 argument들을 분리하여 commands_and_argunents에 규칙대로 넣는다.
	for (i = 0; i < commands_counter; i++) {
		strncpy(tmp_command_store, commands[i], strlen(commands[i]));
		tmp_command_store[strlen(commands[i])] = '\0';
		if ((parser = strtok(tmp_command_store, "\n\t ")) == NULL) {
			//세미콜론들 사이에 공백이 있을 때
			continue;
			g_commands_num--;
		}
		commands_and_arguments[args_counter] = (char*)malloc((strlen(parser) + 10) * sizeof(char));
		strncpy(commands_and_arguments[args_counter], parser, strlen(parser));
		commands_and_arguments[args_counter][strlen(parser)] = '\0';
		command_index[index_counter] = args_counter;
		index_counter++;

		args_counter++;
		while ((parser = strtok(NULL, "\n\t ")) != NULL) {
			commands_and_arguments[args_counter] = (char*)malloc((strlen(parser) + 10) * sizeof(char));
			
			strncpy(commands_and_arguments[args_counter], parser, strlen(parser));
			commands_and_arguments[args_counter][strlen(parser)] = '\0';
			args_counter++;
		}
		commands_and_arguments[args_counter] = NULL;
		args_counter++;
	}
	free(tmp_command_store);
	free(commands);
	g_commands_and_arguments = commands_and_arguments;
	return command_index;
}

/**
 *	이 함수는 execvp함수가 처리하기 쉽도록 command와 argument들이 담겨있는 g_commands_and_arguments 배열을 이용하여
 *  command의 수만큼 자식 프로세스를 만들고 그들이 command를 동시다발적으로 수행하도록 한다.
 *	@param[in] command_index commands_and_arguments배열에서 command가 시작되는 index 값들을 가지고 있는 배열
 *	@return 없습니다
 */ 
void ForkAndExec(int* command_index) {
	int i = 0;
	pid_t pid;

	for (i = 0; i < g_commands_num; i++) {
		pid = fork();
		if (pid < 0) {
			// 자식 프로세스 생성에 실패했을 때
			printf("failed to create a child process\n");
			exit(0);
		}
		else if (pid == 0) {
			// 자식 프로세스에서 수행할 작업 
			if (execvp(g_commands_and_arguments[command_index[i]], g_commands_and_arguments + command_index[i]) == -1) {
				// 명령 실행에 실패했을 때
				printf("failed to execute command: %s\n", g_commands_and_arguments[command_index[i]]);
				exit(0);
			}
			exit(0);
		}
	}
	for (i = 0; i < sizeof(g_commands_and_arguments) / sizeof(char*); i++) {
		if (g_commands_and_arguments[i] != NULL) {
			//free([i]);
		}
	}
	free(command_index);
	//부모 프로세스에서는 모든 자식 프로세스가 끝나기를 기다린다.
	for (i = 0; i < g_commands_num; i++) {
		wait(NULL);
	}
	return;	
}

int main(int argc, char *argv[]) {
	int i=0;
	char tmp_store[10000]; //사용자 입력을 받을 문자열

	if (argc == 1) {
		//interactive mode
		while(1) {
			printf("prompt> ");
			g_commands_num = 0;
            tmp_store[0] = '\0'; //initialization
			if (fgets(tmp_store, sizeof(tmp_store) - 1, stdin) == NULL) {
		        //error handling for IO
				printf("error occured while getting user input in interacive mode\n");	
    			return -1;
			}
			
			if (tmp_store[strlen(tmp_store)-1] != '\n') {
				//tmp_store에 한 줄을 다 저장하지 못했을 경우
				printf("Too large input!!\n");
				return -1;
			}
			if (strcmp(LeftRightTrim(tmp_store), "quit") == 0) {
				//quit 명령어가 들어오면 종료
				return 0;
			}
			ForkAndExec(CommandParser(tmp_store));	
		}
	}
	else if (argc == 2){
		//batch mode
		FILE *fp = fopen(argv[1], "r");
		int line_counter = 0;

		//파일의 line 수를 센다.
		while(fgets(tmp_store, sizeof(tmp_store), fp)!=NULL) { 
			line_counter++;
		}

		fp = fopen(argv[1], "r");

		for(i=0; i<line_counter; i++){
			if(fgets(tmp_store, sizeof(tmp_store) , fp) == NULL) {
				if(feof(fp)){
					//파일의 끝
					return 0;
				}
				else if(ferror(fp)){
					//파일을 읽다가 에러 발생
					printf("error occured while reading a file in batch mode\n");
					return -1;
				}
			}
			printf("%s", tmp_store);
			if (i == line_counter-1 && tmp_store[strlen(tmp_store) -1] != '\n') {
				printf("\n");
			}
			if (strcmp(LeftRightTrim(tmp_store), "quit") == 0) {
				//quit 명령어가 들어오면 종료
				return 0;
			}
			ForkAndExec(CommandParser(LeftRightTrim(tmp_store)));
			tmp_store[0] = '\0';
		}
		fclose(fp);
	}
	else {
		printf("Invalid format");
	}
	return 0;
}