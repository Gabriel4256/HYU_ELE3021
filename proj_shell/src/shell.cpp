/**
 * this file is a simple & lightweight linux shell implementation
 * 
 * @author SungHwan Shim
 * @since 2019-03-21
 *
 */

#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <sstream>
#include <cstring>
#include <stdlib.h>
#include <wait.h>

using namespace std;

vector<int> command_indexes(1);

void ForkAndExec(char** commands_and_arguments){
	int i = 0;
	pid_t pid;
    vector<int>::iterator it;

	for(it=command_indexes.begin(); it != command_indexes.end(); it++){
		pid = fork();
		if(pid < 0) {
			// 자식 프로세스 생성에 실패했을 때
			cout<<"failed to create a child process"<<endl;
		}
		else if(pid == 0) {
			// 자식 프로세스에서 수행할 작업 
			if(execvp(commands_and_arguments[*it], commands_and_arguments + *it) == -1) {
				// 명령 실행에 실패했을 때
                cout<<"failed to execute command: "<<commands_and_arguments[*it]<<endl;
				exit(0);
			}
			exit(0);
		}
	}
	//부모 프로세스에서는 모든 자식 프로세스가 끝나기를 기다린다.
	for(i=0; i<command_indexes.size(); i++){
		wait(NULL);
	}
	return;
}

char** CommandParser(string& input){
    /*size_t num_of_commands = 0;
    int i=0, j=0;
    char char_input[input.length()];
    vector<char*> commands_and_args;
    //vector<int> commands_index;
    char* parser;
    strcpy(char_input, input.c_str());
    vector<char*>::iterator it;

    parser = strtok(char_input, ";");
    while(parser != NULL){
        char tmp[strlen(parser) + 1];

        strncpy(tmp, parser, strlen(parser));
        commands_and_args.push_back(tmp);
        parser = strtok(NULL, ";");
    }
    
    for(i=0; i<num_of_commands; i++) {
        char* tmp = (char*)malloc(strlen(commands_and_args.front()) + 1);
        strcpy(tmp, commands_and_args.front());
        parser = strtok(tmp, " ");
        if(parser == NULL){
            cout << "meaningless space between semicolons exists" << endl;
            continue;
        }
        //command_indexes.push_back(j);
        while(parser!=NULL) {
            char tmp[strlen(parser) + 1];
            strcpy(tmp, parser);
            commands_and_args.push_back(tmp);
            j++;
            parser = strtok(NULL, " ");
        }
        commands_and_args.push_back(NULL);
        j++;
        commands_and_args.erase(commands_and_args.begin());
        free(tmp);
    }

    return commands_and_args.data();*/

    stringstream input_stream(input);
    string big_token, small_token;  
    int i=0;
    vector<string> commands_and_args;
    

    while (getline(input_stream, big_token, ';')){
        stringstream big_token_stream(big_token);
        command_indexes.push_back(i);
        while(getline(big_token_stream, small_token, ' ')){
            commands_and_args.push_back(small_token);
            i++;
        }
        commands_and_args.push_back(NULL);
        i++;
    }

    char *result[commands_and_args.size()];
    vector<string>::iterator it;
    for(it = commands_and_args.begin(), i=0; it != commands_and_args.end(); it++, i++){
         strcpy(result[i], (*it).c_str());
    }
    return result;
}

string& LeftRightTrim(string& input) {
    const string& spaces = "\t\n\r\f\v ";
    input.erase(0, input.find_first_not_of(spaces));
    input.erase(input.find_last_not_of(spaces) + 1);
    return input;
}

int main(int argc, char* argv[]){
    int i=0;
    string input;

    if(argc == 1){
        while(1) {
            cout<< "prompt> ";
            getline(cin, input);
            string quit = "quit";
            if(quit.compare(LeftRightTrim(input)) == 0){
                return 0;
            }
            ForkAndExec(CommandParser(input));
        }
    }
}



