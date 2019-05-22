#include "types.h"
#include "stat.h"
#include "user.h"

thread_t thread[15];
thread_t tl = 150;
int result = 0;

void *thread_main(void *arg)
{
	int i;
	// double result=0.0;

	//printf(1, "therad: %d, %d\n", (int)arg, getpid());

	// while (1)
	// {
        	for (i=0; i < 1000; i++)
   		{
     			result++;
   		}
   		//printf(1, "thread: %d, result = %d\n", (int)arg, (int)result);
	// }
	if((int)arg == 4){
		//sleep(1000);
	}
	thread_exit((void *) 3000);
	// while(1){}
	return 0;
}

int
main(int argc, char *argv[])
{
    int i = 10;
		int ret;
		// thread_t* tmp;
		for(i=0; i<10; i++){
    	thread_create(&(thread[i]), &thread_main, (void *)i);
			printf(1, "tid: %d\n", (int)(thread[i]));
		}
		// thread_create(&tl, &thread_main, (void*)i);
		// tmp = &tl;
		// *tmp = 300;
		// printf(1, "tid: %d\n", (int)tl);
		for(i=0; i<10; i++){
			thread_join(thread[i], (void**)&ret);
		}
		// thread_create(&thread[9], &thread_main, (void *)i);
		// thread_create(&thread[11], &thread_main, (void*)i);
		for(i=9; i>-1; i--){
			// thread_join(thread[i], (void**)&ret);
			// printf(1, "result: %d\n", ret);
		}
		while(1){};
		return 0;
}

