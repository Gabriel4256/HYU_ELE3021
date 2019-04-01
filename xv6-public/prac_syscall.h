#include "types.h"
#include "defs.h"

// Simple system call
int
my_syscall(char *str)
{
	cprintf("%s\n", str);
	return 0xABCDABCD;
}
