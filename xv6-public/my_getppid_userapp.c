#include "types.h"
#include "stat.h"
#include "user.h"


int main(){
	printf(1, "process id is %d\n", getpid());
	printf(1, "parent id is %d\n", getppid()); 
	exit();
}
