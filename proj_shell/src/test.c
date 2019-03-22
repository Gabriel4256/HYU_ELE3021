#include <stdio.h>
#include <unistd.h>

int main(){
	char a[] = "ls";
	char* b[3];
	b[0] = "ls";
	b[1] = "-al";
	b[2] = NULL;
	execvp("ls",b);
}
