#include "types.h"
#include "stat.h"
#include "user.h"

thread_t thread[15];
thread_t tl = 150;
int result = 0;
char *argv[] = { "ls", 0 };
int fd[2];

void *reader(){
	while(1){
		char ch;
		int result;

		result = read(fd[0], &ch, 1);
		if (result != 1){
			printf(1,"Reading Error\n");
		}

		printf(1,"Reader: %c\n", ch);
	}

	return 0;
}

void *writer(){
	int result;
	char ch = 'A';

	while(1){
		result = write(fd[1], &ch, 1);
		if(result != 1){
			printf(1,"Write error\n");
		}

		printf(1,"Writer: %c\n", ch);
		if(ch == 'Z')
			ch = 'A' - 1;

		ch++;
	}

	return 0;
}

void *fun2(void *arg){
	sleep(10);
	cpu_share(20);
	thread_exit(arg);
	// exit();
	return 0;
}

void *thread_main(void *arg)
{
	int i;
	int* ad;
	// int ret;
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
	// if(getpid() % 2 == 0){
		// fork();
		printf(1, "sdff\n");
		// sleep( getpid() * 100);
		// printf(1, "Sleep end\n");
	// }
	// else{
		// thread_exit((void*)3);
	// }
	ad = (int*)sbrk(sizeof(int) * (int)arg);
	for(i=0; i<(int)arg; i++){
		ad[i] = (int)arg;
	}
	for(i=0; i<(int)arg; i++){
		printf(1, "arg: %d, value: %d\n", (int)arg, ad[i]);
	}
	// exec("ls", argv);
	if((int)arg == 4){
		//sleep(1000);
		// kill(3);
		// sleep(1000);
		// kill(8);
		// thread_create(&(thread[3]), &fun2, (void *)15);
		// thread_join(thread[3], (void**)&ret);
		// printf(1, "retval: %d\n", ret);
		//exit();
	}
	if((int)arg == 15){
		// exit();
	}
	while(1){}
	// if(getpid()!=3)
		thread_exit(arg);
	return 0;
}

int
main(int argc, char *argv[])
{
    int i = 10;
		int result;

		result = pipe(fd);
		if(result < 0){
			printf(1, "Error on creating pipe\n");
		}
		// thread_create(&(thread[0]), &reader, 0);
		// thread_create(&(thread[1]), &writer, 0);

		// thread_join(thread[0], 0);
		// thread_join(thread[1], 0);
		// int ret = 0;
		// thread_t* tmp;
		for(i=0; i<2; i++){
    	thread_create(&(thread[i]), &thread_main, (void *)i);
			printf(1, "tid: %d\n", (int)(thread[i]));
		}
		// thread_join(thread[0], 0);
		// fork();
		// thread_create(&(thread[0]), &thread_main, (void *)i);
		// sleep(10000);
		printf(1, "Main sleep end\n\n\n");
		exec("ls", argv);
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

