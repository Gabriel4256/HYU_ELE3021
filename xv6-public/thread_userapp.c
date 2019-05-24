#include "types.h"
#include "stat.h"
#include "user.h"

thread_t thread[15];
thread_t tl = 150;
int result = 0;
char *argv[] = { "ls", 0 };

void *thread_main(void *arg)
{
	int i;
	int* ad;
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
	ad = (int*)sbrk(sizeof(int) * (int)arg);
	for(i=0; i<(int)arg; i++){
		ad[i] = (int)arg;
	}
	for(i=0; i<(int)arg; i++){
		printf(1, "arg: %d, value: %d\n", (int)arg, ad[i]);
	}
	if((int)arg == 4){
		//sleep(1000);
		// kill(3);
		// sleep(1000);
		// kill(8);
		// exec("ls", argv);
		// thread_create(&(thread[3]), &thread_main, (void *)15);
		//exit();
	}
	if((int)arg == 15){
		// exit();
	}
	while(1){}
	if(getpid()!=3)
		thread_exit(arg);
	return 0;
}

int
main(int argc, char *argv[])
{
    int i = 10;
		// int ret = 0;
		// thread_t* tmp;
		for(i=0; i<10; i++){
    	thread_create(&(thread[i]), &thread_main, (void *)i);
			printf(1, "tid: %d\n", (int)(thread[i]));
		}
		// exec("ls", argv);
		// thread_create(&tl, &thread_main, (void*)i);
		// tmp = &tl;
		// *tmp = 300;
		// printf(1, "tid: %d\n", (int)tl);
		// kill(6);
		// sleep(100);
		// kill(7);
		// for(i=0; i<10; i++){
		// 	thread_join(thread[i], (void**)&ret);
		// 	printf(1, "retval: %d\n", ret);
		// }
		// thread_create(&thread[9], &thread_main, (void *)i);
		// thread_create(&thread[11], &thread_main, (void*)i);
		for(i=9; i>-1; i--){
			// thread_join(thread[i], (void**)&ret);
			// printf(1, "result: %d\n", ret);
		}
		while(1){};
		return 0;
}

