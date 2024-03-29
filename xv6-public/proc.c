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
struct mlfqnode* headers[4];
struct proc* fixedmin = 0;

double getminpath();
struct mlfqnode* choosebymlfq();
struct proc* choosebystride();
int dealloc_thread(struct proc*);
struct proc* get_highest_master(struct proc*);
struct proc* getminproc(struct proc* a, struct proc* b);

uint nexttid = 1;
struct spinlock memlock;

void
initmlfq()
{
  int i;
  if(!headers[0]){
    for(i=0; i<4; i++){
      headers[i] = &mlfqnodeslabs[NPROC + i];
      headers[i]->state = -1;
    }

    for(i=0; i<4; i++){
      if(i!=3)
        headers[i]->next = headers[i+1];
      if(i!=0)
        headers[i]->prev = headers[i-1];
    }
    
    mlfqvmp.allottime[0] = 5;
    mlfqvmp.slice[0] = 1;
    mlfqvmp.allottime[1] = 10;
    mlfqvmp.slice[1] = 2;
    mlfqvmp.allottime[2] = 1000;
    mlfqvmp.slice[2] = 4;
    mlfqvmp.highpr = 0;
    mlfqvmp.path = 0;
    mlfqvmp.stride = 5;
    mlfqvmp.slice_cnt = 0;
    mlfqvmp.tick_cnt = 0;

    mlfqvmp.path = getminpath();
  }
}

struct mlfqnode*
allocmlfqnode(struct proc* p)
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

void
insertafter(struct mlfqnode* target, struct mlfqnode* mover)
{
  mover->prev = target;
  if(target->next)
    target->next->prev = mover;
  mover->next = target->next;
  target->next = mover;
}

void
insertBefore(struct mlfqnode* target, struct mlfqnode* mover)
{
  mover->next = target;
  if(target->prev)
    target->prev->next = mover;
  mover->prev = target->prev;
  target->prev = mover;
}

void
remove(struct mlfqnode* m)
{
  if(m->prev)
    m->prev->next = m->next;
  if(m->next)
    m->next->prev = m->prev;
  m->next =0;
  m->prev = 0;
}

void
delete(struct mlfqnode* m)
{
  remove(m);
  m->state = 0;
  m->level = 0;
  m->exec_time = 0;
  m->self = 0;
}

void
lowerlevel(struct mlfqnode* m)
{
  if(m->level == 2)
    return;
  remove(m);
  insertBefore(headers[m->level + 2],m);
  m->level+=1;
}

void
priorityboost()
{
  struct mlfqnode* m;
  if(headers[0]){
    int i;
    for(i=2; i>0; i--){
      remove(headers[i]);
      insertBefore(headers[i+1], headers[i]);
    }
    m = headers[0];
    while(m){
      if(m->state == 1 && m->level!= 1){
        m->level = 0;
        m->exec_time = 0;
      }
      m = m->next;
    }
  }
}

// This function should be called with ptable.lock held
void 
updatevals()
{
  struct proc *p, *hmaster;
  double min = -1;
  double min2 = -1;
  int totalfixedshare = 0;
  defaultvmp.highpr = 0;
  fixedmin = 0;
  int mlfqcnt = 0;
  int mastercnt = 0;
  // int pathlevel;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == RUNNABLE){
      if(!p->master){
        mastercnt++;
      }
      hmaster = get_highest_master(p);
      if(hmaster->schedmode == 0){
        if(min2 == -1 || hmaster->pathlevel< min2)
        {
          min2 = hmaster->pathlevel;
          defaultvmp.highpr = hmaster;
        }
      }
      else if(hmaster->schedmode == 1){
        if(!p->master)
          totalfixedshare += p->fixedshare;
        if(min == -1 || hmaster->path < min){
          fixedmin = hmaster;
          min = hmaster->path;
        }
      }
      else{
        mlfqcnt++;
      }
    }
  }
  // if(mastercnt > 0)
  //   cprintf("# of master: %d\n", mastercnt);
  if(defaultvmp.highpr){
    defaultvmp.highpr->path = defaultvmp.path;
  }
  if(mlfqcnt){
    if(mlfqvmp.path == -1){
      p = getminproc(fixedmin, defaultvmp.highpr);
      mlfqvmp.path  = 0;
      if(p)
        mlfqvmp.path = p->path;
    }
    defaultvmp.stride = (double)100 / (double)(80 - totalfixedshare);
  }
  else{
    mlfqvmp.path = -1;
    defaultvmp.stride = (double)100 / (double)(100 - totalfixedshare);
  }
  mlfqvmp.highpr = choosebymlfq();
}

double 
min(double a, double b)
{
  if(a > b)
    return b;
  else
    return a;
}

double
getminpath()
{
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
    return defaultvmp.highpr->pathlevel;
  return 0;
  
}

int 
cpu_share(int n)
{
  struct proc *p;
  int totalfixedshare = 0;
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->schedmode == 1 && (p->state == RUNNABLE || p->state == SLEEPING) && !p->master)
      totalfixedshare += p->fixedshare;
  }

  if(n <= 0 || totalfixedshare  + n > 20){
    release(&ptable.lock);
    return -1;
  }
  p = get_highest_master(myproc());
  p->fixedshare = n;
  p->path = getminpath();
  while(p){
    if(p->schedmode == 2)
      delete(p->mnode);
    p->schedmode = 1;
    p = p->next_thread;
  }
  release(&ptable.lock);
  return 0;
}

int
run_MLFQ()
{
  // struct mlfqnode* m;
  struct proc* p = myproc();
  struct proc* hmaster;

  acquire(&ptable.lock);
  if(!headers[0])
    initmlfq();
  if(mlfqvmp.path == -1)
    mlfqvmp.path = getminpath();
  hmaster = get_highest_master(p);
  //put all the threads in the queue
  while(hmaster){
    hmaster->schedmode = 2;
    insertafter(headers[0], allocmlfqnode(hmaster));
    hmaster = hmaster->next_thread;
  }
  release(&ptable.lock);
  return 0;
}

int getlev(){
  struct proc* p = myproc();
  acquire(&ptable.lock);
  if(get_highest_master(p)->schedmode!=2)
    return -1;
  acquire(&ptable.lock);
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
  p->schedmode = 0;
  p->turn = p;
  updatevals();
  if(defaultvmp.highpr)
    p->pathlevel = defaultvmp.highpr->pathlevel;
  else
    p->pathlevel = 0;
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
  struct proc* master;

  acquire(&memlock);
  master = get_highest_master(curproc);
  sz = master->sz;

  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  if(curproc->master)
    master->sz = sz;
  release(&memlock);
  switchuvm(curproc);
  return sz;
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
  struct proc* hmaster; //, *p;

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }
  acquire(&ptable.lock);

  hmaster = get_highest_master(curproc);

  curproc->sz = hmaster->sz;
  for(i=0; i<hmaster->emptystackcnt; i++){
    np->emptystacks[i] = hmaster->emptystacks[i];
  }
  np->emptystackcnt = hmaster->emptystackcnt;

  // Copy process state from proc.
  if((np->pgdir = copyuvm2(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }

  // switchuvm(np);

  // p = hmaster;
  // while(p){
  //   if(p!=curproc){
  //     deallocuvm(np->pgdir, p->originalbase + 2 * PGSIZE, p->originalbase);
  //     if(p->originalbase + 2* PGSIZE < hmaster->sz){
  //       np->emptystacks[np->emptystackcnt++] = p->originalbase;
  //     }
  //     else{
  //       np->sz -= 2 * PGSIZE;
  //     }
  //   }
  //   p = p->next_thread;
  // }

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


  np->state = RUNNABLE;
  np->turn = np;
  np->schedmode = 0;
  np->pathlevel = getminpathlevel();
  np->tid = 0;
  np->prev_thread = 0;
  np->next_thread = 0;
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
  int found = 0;
  // cprintf("pid: %d in exit, name: %s\n", curproc->pid, curproc->name);
  if(curproc == initproc)
    panic("init exiting");

  acquire(&ptable.lock);

  // If there is worker thread not joined by master thread,
  // then terminate it and deallocate resources
  if(curproc->master)
    release(&ptable.lock);
  else{
    for(;;){
      p = curproc->next_thread;
      found = 0;
      while(p){
        if(p->state == ZOMBIE)
          dealloc_thread(p);
        else{
          found = 1;
          p->killed = 1;
          if(p->state == SLEEPING)
            p->state = RUNNABLE;
        }
        p = p->next_thread;
      }
      if(!found){
        release(&ptable.lock);
        break;
      }
      sleep(curproc, &ptable.lock);
    }
  }

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
  if(curproc->parent)
    wakeup1(curproc->parent);
  
  //master thread might be sleeping
  if(curproc->master){
    if(!curproc->killed)
      get_highest_master(curproc)->killed = 1;
    wakeup1(get_highest_master(curproc));
  }

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
        p->emptystackcnt = 0;
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
  if(headers[0] == 0)
    return 0;
  struct mlfqnode* m = headers[0];
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
  if(!headers[0]->next)
    headers[0] = 0;
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
  struct proc *p, *q;
  struct cpu *c = mycpu();
  c->proc = 0;
  int found;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    //for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    while((p = choosebystride()) !=0){
      found = 0;
      //if(p->state != RUNNABLE)
        //continue;
      if(p->schedmode == 0){
        // cprintf("Chosen: %d\n", p->pid);
        q = p->turn;
        // cprintf("Really chosen: %d\n", q->pid);
        while(1){
          if(!q)
            q = p;
          if(q->state == RUNNABLE){
            found = 1;
            p->turn = q->next_thread ? q->next_thread : p;
            break;
          }
          q = q->next_thread;
          if(q == p->turn)
            break;
        }
        if(!found)
          continue;
        defaultvmp.path += defaultvmp.stride;
        p->pathlevel+=1;
        p = q;
        // cprintf("default: %d\n", (int)(defaultvmp.path));
      }
      else if(p->schedmode ==1){
        // cprintf("11111\n");
        q = p->turn;
        while(1){
          if(!q)
            q = p;
          if(q->state == RUNNABLE){
            found = 1;
            p->turn = q->next_thread ? q->next_thread : p;
            break;
          }
          q = q->next_thread;
          if(q == p->turn)
            break;
        }
        if(!found)
          continue;
        p->path += (double)(100/(double)p->fixedshare);
        p = q;
        // cprintf("fixed: %d\n", (int) (p->path));
      }
      else if(p->schedmode == 2){
        mlfqvmp.path+= (double)mlfqvmp.stride;
        mlfqvmp.slice_cnt++;
        p->mnode->exec_time+=1;
        mlfqvmp.tick_cnt++;
        // cprintf("tick: %d %d\n", mlfqvmp.tick_cnt, (int)mlfqvmp.path);
        if(mlfqvmp.slice_cnt >= mlfqvmp.slice[p->mnode->level] || p->state != RUNNABLE){
          if(p->mnode->exec_time >= mlfqvmp.allottime[p->mnode->level]){
            lowerlevel(p->mnode);
            p->mnode->exec_time = 0;
          }
          else{
            remove(p->mnode);
            insertBefore(headers[p->mnode->level +1], p->mnode);
          }
          mlfqvmp.slice_cnt = 0;
          mlfqvmp.highpr = 0;
        }
        if(mlfqvmp.slice_cnt== 100){
          mlfqvmp.slice_cnt= 0;
          priorityboost();
        }
      }

      //Handling for deallocation in case of kill
      // for(q = ptable.proc; q < &ptable.proc[NPROC]; q++){
      //   if(get_highest_master(q) == p && q->state == ZOMBIE && q->killed)
      //     dealloc_thread(q);
      // }


      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;
      swtch(&(c->scheduler), p->context);
      switchkvm();

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
      // cprintf("Wake up: %d\n", (int)p->pid);
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
  struct proc *p, *hmaster;
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      hmaster = get_highest_master(p); 
      hmaster->killed = 1;

      // Wake process from sleep if necessary.
      if(hmaster->state == SLEEPING)
        hmaster->state = RUNNABLE;
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

// Craete new thread of the calling process
int
thread_create(thread_t * thread, void * (start_routine)(void *), void *arg)
{
  struct proc *np;
  struct proc *curproc = myproc();
  struct proc *hmaster;
  int i = 0;
  uint sz = 0;
  uint sp = 0;
  
  //Allocate proc struct & kernel spaces
  if((np = allocproc()) == 0){
    return -1;
  }
  --nextpid;
    
  hmaster = get_highest_master(curproc);
  acquire(&memlock);
  curproc->sz  = hmaster->sz;
  sz = curproc->sz;
  if(hmaster->emptystackcnt > 0){
    // Found empty stack space
    sz = hmaster->emptystacks[--hmaster->emptystackcnt];
  }

  //allocates two pages of memory, one for thread user stack and one for guard page
  if((sz = allocuvm(curproc->pgdir, sz, sz + 2*PGSIZE)) == 0)
    goto bad;
  if(curproc->sz < sz){
    curproc->sz = sz;
    hmaster->sz = sz;
  }
  release(&memlock);
  //share page table of master thread
  np->pgdir = curproc->pgdir;

  //set other values
  np->master = curproc;
  *np->tf = *curproc->tf;
  np->sz = sz;
  np->originalbase = sz - 2 * PGSIZE;
  np->pid = curproc->pid;
  switchuvm(curproc);

  // //make guard page
  clearpteu(np->pgdir, (char*)(sz - 2*PGSIZE));

  // //put arguments and fake return address
  sp = np->sz;
  *((uint*)(sp - sizeof(uint))) = (uint)arg;
  *((uint*)(sp - 2 * sizeof(uint))) = 0xffffffff;
  sp-= 2 * sizeof(uint);
  
  np->tf->esp = sp;
  np->tf->ebp = sz;
  np->tf->eip = (uint)start_routine;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));
  *thread = nexttid++;
  np->tid = *thread;
  
  //thread Linkedlist operation
  acquire(&ptable.lock);
  
  if(curproc->next_thread){
    curproc->next_thread->prev_thread = np;
  }
  np->prev_thread = curproc;
  np->next_thread = curproc->next_thread;
  curproc->next_thread = np;

  np->state = RUNNABLE;
  np->schedmode = curproc->schedmode;
  if(np->schedmode == 2){
    // If scheduling mode is mlfq, then put it in the queue
    insertafter(headers[0], allocmlfqnode(np));
  }
  np->pathlevel = getminpathlevel();
  np->retval = 0; 

  release(&ptable.lock);
  return 0;

  bad:
    return -1;
}

//exit the calling thread
//return value is stored in the struct proc and passed to master by calling thread_join
void
thread_exit(void *retval)
{
  struct proc *curproc = myproc();
  int fd;

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

  // Save return value in the proc struct
  curproc->retval = retval;
  // cprintf("retval: %d\n", (int)(retval));
  acquire(&ptable.lock);
  wakeup1(curproc->master);
  wakeup1(curproc->parent);

  // Jump into the scheduler, never to return
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}


// Wait for the thread to call thread_exit.
// And when the thread is terminated, clean the resources fot it.
int
thread_join(thread_t thread, void **retval)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int found = 0;
  
  acquire(&ptable.lock);
  for(;;){
    p = curproc->next_thread;
    while(p){
      if(p->tid == thread){
        // cprintf("found tid: %d\n", (int)p->tid);
        found = 1;
        if(p->state == ZOMBIE){
          dealloc_thread(p);

          //pass return value of the thread to the argument retval
          *retval = p->retval;
          p->retval = 0;
          release(&ptable.lock);
          return 0;
        }
      }
      p = p->next_thread;
    }
    if(!found || curproc->killed){
      // Not found the thread of such a id, or master thread has already been killed.
      release(&ptable.lock);
      return -1;
    } 

    // Wait for the worker thread to exit
    sleep(curproc, &ptable.lock);
  }
  return 0;
}

int
dealloc_thread(struct proc* worker){
  struct proc* hmaster;
  // cprintf("%d willl be deallocated\n", (int)worker->pid);
  if(!worker->master)
    return -1;

  hmaster = get_highest_master(worker);
  
  kfree(worker->kstack);
  worker->kstack = 0;
  worker->pid = 0;
  worker->parent = 0;
  worker->master = 0;
  worker->name[0] = 0;
  worker->killed = 0;
  worker->state = UNUSED;
  worker->emptystackcnt = 0;
  worker->turn = 0;
  worker->tid = 0;
  worker->mnode = 0;
  worker->fixedshare = 0;
  deallocuvm(worker->pgdir, worker->originalbase + 2*PGSIZE, worker->originalbase);

  acquire(&memlock);
  if(worker->originalbase + 2* PGSIZE < hmaster->sz){
    hmaster->emptystacks[hmaster->emptystackcnt++] = worker->originalbase;
  }
  else{
    hmaster->sz -= 2 * PGSIZE;
  }
  worker->originalbase = 0;
  release(&memlock);
  
  //delete from the thread linked list
  if(worker->prev_thread)
    worker->prev_thread->next_thread = worker->next_thread;
  if(worker->next_thread)
    worker->next_thread->prev_thread = worker->prev_thread;

  return 0;
}

struct proc*
get_highest_master(struct proc* p)
{
  struct proc * q;
  q = p;
  while(q->master){
    q = q->master;
  }
  return q;
}

void
dealloc_other_threads(struct proc* hmaster){
  struct proc* p;
  int found;

  acquire(&ptable.lock);
  if(hmaster->master){
    p = get_highest_master(hmaster);
    hmaster->parent = p->parent;
    p->parent = hmaster;
    if(hmaster->next_thread)
      hmaster->next_thread->prev_thread = hmaster->prev_thread;
    if(hmaster->prev_thread)
      hmaster->prev_thread->next_thread = hmaster->next_thread;
    hmaster->prev_thread = hmaster->next_thread = 0;
    hmaster->master = 0;
    p->killed = 1;
    if(p->state == SLEEPING)
      p->state = RUNNABLE;
    release(&ptable.lock);
    wait();
  }
  else{
    for(;;){
      p = hmaster->next_thread;
      found = 0;
      while(p){
        p->state = ZOMBIE;
        if(p->state == ZOMBIE)
          dealloc_thread(p);
        else{
          found = 1;
          p->killed = 1;
          if(p->state == SLEEPING)
            p->state = RUNNABLE;
        }
        p = p->next_thread;
      }
      if(!found){
        release(&ptable.lock);
        break;
      }
      sleep(hmaster, &ptable.lock);
    }
  }
}

void
sleep_other_threads(struct proc* self)
{
  struct proc * p;
  acquire(&ptable.lock);
  p = get_highest_master(self);
  while(p){
    if(p!=self && p->state == RUNNABLE){
      p->state = SLEEPING;
    }
    p = p->next_thread;
  }
  release(&ptable.lock);
}