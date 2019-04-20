#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
	int i,j;
	i = fork();
 	j = fork();
while(1){
	if(i > 0){
		printf(1, "Parent\n");
		yield();
	}
	else if(i == 0 ){
		printf(1, "Child\n");
		yield();
	}
}
	exit();
}

