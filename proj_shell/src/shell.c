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

typedef struct CommandInfo {
	char *command;
	char *arguments[];
} CommandInfo;

int main(int argc, char *argv[]) {
	char tmp_store1[256], tmp_store2[10];
	int i=0;
	if (argc == 1){
		//interactive mode
		while(true) { 
			fgets(tmp_store1, 256, stdin);
			if (tmp_store1[strlen(tmp_store1)-1] !== '\n') {
				//한 줄이 99자를 넘었을 경우
				for(i= 98; i>0; i--){
					if(tmp_command_store[i] == ';') {
						break;
					}
				}
			}
			else{
				ForkAndExec(CommandParser(tmp_store1));	
			}			
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

CommandInfo** CommandParser(char* input){
	//줄 단위로 파싱한다.
	char *big_parser, *small_parser;
	
	big_parser = strtok(input, " ; ");
	while(big_parser!=NULL) {
		//big parser는 한 줄을 " ; "을 기준으로 잘라 command+argument 단위로 분리
		big_parser = strtok(NULL, " ; ");
		small_parser = strtok(big_parser, " ");
		while(small_parser!=NULL) {
			//command와 argument를 분리
			small_parser = strtok(NULL, " ");
		}
	}
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
			
			while(execvp(command, arguments) == -1){
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
