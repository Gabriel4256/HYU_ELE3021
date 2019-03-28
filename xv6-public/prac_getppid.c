#include "types.h"
#include "defs.h"
#include "proc.h"
#include "x86.h"
#include "date.h"
#include "param.h"
#include "mmu.h"
#include "memlayout.h"
#include "spinlock.h"

int main(){
	return myproc()->parent->pid; 
}
