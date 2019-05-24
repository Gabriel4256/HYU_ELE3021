#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "defs.h"
#include "x86.h"
#include "elf.h"

int
exec(char *path, char **argv)
{
  char *s, *last;
  int i, off;
  uint argc, sz, sp, ustack[3+MAXARG+1];
  struct elfhdr elf;
  struct inode *ip;
  struct proghdr ph;
  pde_t *pgdir, *oldpgdir;
  struct proc *curproc = myproc();
  struct proc *p;
  // struct proc* p;
  oldpgdir = 0;
  cprintf("Fwwe: %d\n",curproc->pid);
  // for(p = ptable.proc; p < &ptable.proc[NPROC]; t++){
  //     if(p->master == curproc){
  //       kfree(p->kstack);
  //       t->kstack = 0;
  //       t->pid = 0;
  //       t->parent = 0;
  //       t->master = 0;
  //       t->name[0] = 0;
  //       t->killed = 0;
  //       t->state = UNUSED;
  //       deallocuvm(t->pgdir, t->sz, t->originalbase);
  //       if(t->sz < p->sz){
  //         p->emptystacks[p->emptystackcnt++] = t->originalbase;
  //       }
  //       else{
  //         p->sz -= 2 * PGSIZE;
  //       }   
  //   }
  // }
  if(curproc->next_thread || curproc->master){
    p = curproc->master;
    curproc->parent = p->parent;
    if(curproc->prev_thread)
      curproc->prev_thread->next_thread = curproc->next_thread;
    if(curproc->next_thread)
      curproc->next_thread->prev_thread = curproc->prev_thread;
    // curproc->master = 0;
    while(p){
      if(p != curproc){
        p->killed = 1;
      }
        // dealloc_thread(p);
      p = p->next_thread;
    }
  }
  //If it's thread, then change it to master thread and deallocate rest of them.
  // if(curproc->master){
  //   p = curproc->master;
  //   next = p->next_thread;
  //   cprintf("new master: %d\n", next->pid);
  //   p->master = next;
  //   next->master = 0;

  //   p->next_thread = next->next_thread;
  //   p->prev_thread = next;
  //   if(p->next_thread)
  //     p->next_thread->prev_thread = p;
  //   next->next_thread = p;
  //   next->prev_thread = 0;
  //   next->thread_kill_cnt = 1;
  //   next->parent = p->parent;
  //   // p->state = ZOMBIE;
  //   next->sz = p->sz;
  //   int pid = p->pid;
  //   p->pid = next->pid;
  //   next->pid = pid;
  //   p->parent = 0;

  //   q = next->next_thread;
  //   while(q){
  //     q->master = next;
  //     q = q->next_thread;
  //   }
  // }
  
  

  begin_op();

  if((ip = namei(path)) == 0){
    end_op();
    cprintf("exec: fail\n");
    return -1;
  }
  ilock(ip);
  pgdir = 0;

  // Check ELF header
  if(readi(ip, (char*)&elf, 0, sizeof(elf)) != sizeof(elf))
    goto bad;
  if(elf.magic != ELF_MAGIC)
    goto bad;

  if((pgdir = setupkvm()) == 0)
    goto bad;

  // Load program into memory.
  sz = 0;
  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
    if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
      goto bad;
    if(ph.type != ELF_PROG_LOAD)
      continue;
    if(ph.memsz < ph.filesz)
      goto bad;
    if(ph.vaddr + ph.memsz < ph.vaddr)
      goto bad;
    if((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0)
      goto bad;
    if(ph.vaddr % PGSIZE != 0)
      goto bad;
    if(loaduvm(pgdir, (char*)ph.vaddr, ip, ph.off, ph.filesz) < 0)
      goto bad;
  }
  iunlockput(ip);
  end_op();
  ip = 0;

  // Allocate two pages at the next page boundary.
  // Make the first inaccessible.  Use the second as the user stack.
  // And take note of the original base address of space for stack
  sz = PGROUNDUP(sz);
  curproc->originalbase = sz;
  if((sz = allocuvm(pgdir, sz, sz + 2*PGSIZE)) == 0)
    goto bad;
  clearpteu(pgdir, (char*)(sz - 2*PGSIZE));
  sp = sz;

  // Push argument strings, prepare rest of stack in ustack.
  for(argc = 0; argv[argc]; argc++) {
    if(argc >= MAXARG)
      goto bad;
    sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
    if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;
    ustack[3+argc] = sp;
  }
  ustack[3+argc] = 0;

  ustack[0] = 0xffffffff;  // fake return PC
  ustack[1] = argc;
  ustack[2] = sp - (argc+1)*4;  // argv pointer

  sp -= (3+argc+1) * 4;
  if(copyout(pgdir, sp, ustack, (3+argc+1)*4) < 0)
    goto bad;

  // Save program name for debugging.
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  safestrcpy(curproc->name, last, sizeof(curproc->name));

  // Commit to the user image.
  oldpgdir = curproc->pgdir;
  curproc->pgdir = pgdir;
  curproc->sz = sz;
  curproc->tf->eip = elf.entry;  // main
  curproc->tf->esp = sp;
  switchuvm(curproc);
  if(!curproc->master){
    freevm(oldpgdir);
  }
  else{
    cprintf("sfdfdf\n");
    curproc->master = 0;
  }
  return 0;

 bad:
  if(pgdir)
    freevm(pgdir);
  if(ip){
    iunlockput(ip);
    end_op();
  }
  return -1;
}
