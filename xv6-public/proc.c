#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;


////////////////////
#define TOTAL_TICKETS 10000

struct
{
  double stride, path;
  struct proc* highpr;
} defaultvmp;

struct
{
  double stride, path;
  struct mlfqnode* highpr;
  int slice_cnt, tick_cnt;
  int allottime[3], slice[3];
} mlfqvmp;

struct mlfqnode mlfqnodeslabs[NPROC + 4];
struct mlfqnode* mlfqheaders[4];
struct proc* fixedmin = 0;
double getminpath();
struct mlfqnode* choosebymlfq();
struct proc* choosebystride();
// function to create a queue of given capacity.  
// It initializes size of queue as 0 

void initmlfq(){
  int i;
  if(!mlfqheaders[0]){
    for(i=0; i<4; i++){
      mlfqheaders[i] = &mlfqnodeslabs[NPROC + i];
      mlfqheaders[i]->state = -1;
    }

    for(i=0; i<4; i++){
      if(i!=3)
        mlfqheaders[i]->next = mlfqheaders[i+1];
      if(i!=0)
        mlfqheaders[i]->prev = mlfqheaders[i-1];
    }

    mlfqvmp.slice[0] = 1;
    mlfqvmp.slice[1] = 2;
    mlfqvmp.slice[2] = 4;
    mlfqvmp.allottime[0] = 5;
    mlfqvmp.allottime[1] = 10;
    mlfqvmp.allottime[2] = 10000;    
    mlfqvmp.path = getminpath();
    mlfqvmp.stride = 5;
    mlfqvmp.slice_cnt = 0;
    mlfqvmp.highpr = 0;
  }
}

struct mlfqnode* allocmlfqnode(struct proc* p)
{
  struct mlfqnode* m = 0;
  for(m = mlfqnodeslabs; m<&mlfqnodeslabs[NPROC+4]; m++){
    if(!m->state)
      break;
  }
  m->self = p;
  p->mnode = m;
  m->next =0; m->prev = 0; m->level = 0; m->exec_time = 0;
  m->state = 1;
  return m;
}

void insertafter(struct mlfqnode* target, struct mlfqnode* mover){
  mover->prev = target;
  if(target->next)
    target->next->prev = mover;
  mover->next = target->next;
  target->next = mover;
}

void insertBefore(struct mlfqnode* target, struct mlfqnode* mover){
  mover->next = target;
  if(target->prev)
    target->prev->next = mover;
  mover->prev = target->prev;
  target->prev = mover;
}

void remove(struct mlfqnode* m){
  if(m->prev)
    m->prev->next = m->next;
  if(m->next)
    m->next->prev = m->prev;
  m->next =0;
  m->prev = 0;
}

void delete(struct mlfqnode* m){
  remove(m);
  m->state = 0;
  m->level = 0;
  m->exec_time = 0;
  m->self = 0;
}

void lowerlevel(struct mlfqnode* m){
  if(m->level == 2)
    return;
  remove(m);
  insertBefore(mlfqheaders[m->level + 2],m);
  m->level+=1;
}

void priorityboost(){
  struct mlfqnode* m;
  if(mlfqheaders[0]){
    int i;
    for(i=2; i>0; i--){
      remove(mlfqheaders[i]);
      insertBefore(mlfqheaders[i+1], mlfqheaders[i]);
    }
    m = mlfqheaders[0];
    while(m){
      if(m->state == 1 && m->level!= 1){
        m->level = 0;
        m->exec_time = 0;
      }
      m = m->next;
    }
  }
}

void 
updatevals(){
  struct proc* p = 0;
  double min = -1;
  double min2 = -1;
  int totalfixedshare = 0;
  defaultvmp.highpr = 0;
  fixedmin = 0;
  int mlfqcnt = 0;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == RUNNABLE){
      if(p->schedmode == DEFAULT){
        if(min2 == -1 || p->path< min2)
        {
          min2 = p->path;
          defaultvmp.highpr = p;
        }
      }
      else if(p->schedmode == FIXEDSHARE){
        totalfixedshare += p->fixedshare;
        if(min == -1 || p->path < min){
          fixedmin = p;
          min = p->path;
        }
      }
      else{
        mlfqcnt++;
      }
    }
  }
  if(defaultvmp.highpr){
    defaultvmp.highpr->path = defaultvmp.path;
  }
  if(mlfqcnt){
    defaultvmp.stride = (double)100 / (double)(80 - totalfixedshare);
  }
  else{
    defaultvmp.stride = (double)100 / (double)(100 - totalfixedshare);
  }
  mlfqvmp.highpr = choosebymlfq();
}

double 
min(double a, double b){
  if(a > b)
    return b;
  else
    return a;
}

double
getminpath(){
  //minimum path를 반환
  struct proc* p;
  p = choosebystride();
  if(p)
    return p->path;
  return 0;
}

int
getminpathlevel()
{
  if(defaultvmp.highpr)
    return defaultvmp.highpr->path;
  return 0;
  
}

int 
cpu_share(int n)
{
  struct proc* p;
  int totalfixedshare = 0;
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->schedmode == FIXEDSHARE && (p->state == RUNNABLE || p->state == SLEEPING))
      totalfixedshare += p->fixedshare;
  }

  if(n <= 0 || totalfixedshare  + n > 20){
    release(&ptable.lock);
    return -1;
  }
  p = myproc();
  p->schedmode = FIXEDSHARE;
  p->fixedshare = n;
  p->path = getminpath();
  //cprintf("topreoperjureiorpe: %d\n", totalfixedshare);
  release(&ptable.lock);
  return 0;
}

int run_MLFQ(){
  struct mlfqnode* m;
  if(!mlfqheaders[0])
    initmlfq();
  m = allocmlfqnode(myproc());
  myproc()->schedmode = MLFQ;
  insertafter(mlfqheaders[0], m);
  return 0;
}

int getlev(){
  struct proc* p = myproc();
  if(p->schedmode!=MLFQ)
    return -1;
  return p->mnode->level;
}
/////////////////////

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);
  
  p->state = RUNNABLE;
  p->schedmode = DEFAULT;
  updatevals();
  if(defaultvmp.highpr)
    p->path = defaultvmp.highpr->path;
  else
    p->path = 0;
     //p->path = getminpath();

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;
  np->schedmode = DEFAULT;
  np->path = getminpathlevel();
  //np->path = getminpath();
  
  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);
  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

struct proc*
getminproc(struct proc* a, struct proc* b){
  if(a == 0)
    return b;
  if(b == 0)
    return a;
  if(a->path > b->path)
    return b;
  else
    return a;
}


struct mlfqnode*
choosebymlfq(){
  if(mlfqheaders[0] == 0)
    return 0;
  struct mlfqnode* m = mlfqheaders[0];
  struct mlfqnode* tmp = 0;
  if(mlfqvmp.highpr && mlfqvmp.highpr->self->state == RUNNABLE)
    return mlfqvmp.highpr;
  while(m){
    if(m->state != 1){
      
      m = m->next;
      continue;
    }
    if(m->self->state != RUNNABLE && m->self->state != SLEEPING){
      tmp = m;
      m = m->next;
      delete(tmp);
      continue;
    }
    if(m->self->state == RUNNABLE)
      break;
    m = m->next;
  }
  if(!mlfqheaders[0]->next)
    mlfqheaders[0] = 0;
  return m;
}

struct proc*
choosebystride(){
  updatevals();
  //double minpath;
  // struct proc* tmp;
  if(defaultvmp.highpr)
    defaultvmp.highpr->path = defaultvmp.path;
  if(mlfqvmp.highpr){
    mlfqvmp.highpr->self->path = mlfqvmp.path;
    return getminproc(fixedmin, getminproc(mlfqvmp.highpr->self, defaultvmp.highpr));
  }
  else{
    return getminproc(fixedmin, defaultvmp.highpr);
  }
}


//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    //for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    while((p = choosebystride()) !=0){
      if(p->state != RUNNABLE)
        continue;
      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;
      swtch(&(c->scheduler), p->context);
      switchkvm();

      if(p->schedmode == DEFAULT){
        defaultvmp.path += defaultvmp.stride;
        p->path+=1;
        // cprintf("default: %d\n", (int)(defaultvmp.path));
      }
      else if(p->schedmode ==FIXEDSHARE){
        p->path += (double)(100/(double)p->fixedshare);
        // cprintf("fixed: %d\n", (int) (p->path));
      }
      else if(p->schedmode == MLFQ){
        mlfqvmp.path+= mlfqvmp.stride;
        mlfqvmp.slice_cnt++;
        p->mnode->exec_time+=1;
        mlfqvmp.tick_cnt++;
        // cprintf("tick: %d %d\n", mlfqtickcount, (int)mlfqvmp.path);
        if(mlfqvmp.slice_cnt >= mlfqvmp.slice[p->mnode->level] || p->state != RUNNABLE){
          if(p->mnode->exec_time >= mlfqvmp.allottime[p->mnode->level]){
            lowerlevel(p->mnode);
            p->mnode->exec_time = 0;
          }
          else{
            remove(p->mnode);
            insertBefore(mlfqheaders[p->mnode->level +1], p->mnode);
          }
          mlfqvmp.slice_cnt = 0;
          mlfqvmp.highpr = 0;
        }
        if(mlfqvmp.tick_cnt == 100){
          mlfqvmp.tick_cnt = 0;
          priorityboost();
        }
      }
      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan){
      p->state = RUNNABLE;
      //p->path = getminpath();
    }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}
