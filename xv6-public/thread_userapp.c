#include "types.h"
#include "stat.h"
#include "user.h"

thread_t thread;
int result = 0;

void *thread_main(void *arg)
{
	// int i;
	// double result=0.0;

	printf(1, "therad: %d, %d\n", (int)arg, getpid());

	while (1)
	{
        	// for (i=0; i < 1000000; i++)
   		// {
     			result = result + 1;
   		// }
   		printf(1, "thread: %d, result = %d\n", (int)arg, (int)result);
	}

	//pthread_exit((void *) 0);
}

int
main(int argc, char *argv[])
{
    int i = 10;
    thread_create(&thread, &thread_main, (void *)i);
    while(1){
		result = result + 1;
		printf(1, "main: %d\n", result);
	}
}

