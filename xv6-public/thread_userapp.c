#include "types.h"
#include "stat.h"
#include "user.h"

thread_t thread[10];
int result = 0;

void *thread_main(void *arg)
{
	int i;
	// double result=0.0;

	//printf(1, "therad: %d, %d\n", (int)arg, getpid());

	// while (1)
	// {
        	for (i=0; i < 10000000* (10 - (int)arg); i++)
   		{
     			result++;
   		}
   		//printf(1, "thread: %d, result = %d\n", (int)arg, (int)result);
	// }

	thread_exit((void *) 3000);
	// while(1){}
	return 0;
}

int
main(int argc, char *argv[])
{
    int i = 10;
		int ret;
		for(i=0; i<10; i++)
    	thread_create(&thread[i], &thread_main, (void *)i);
		for(i=9; i>-1; i--){
			thread_join(thread[i], (void**)&ret);
			printf(1, "result: %d\n", ret);
		}
		return 0;
}

