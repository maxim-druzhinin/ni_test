#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return getpid_by_proc(myproc());
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int getppid_by_proc(struct proc* p) {
  if (p->no_parent) return 0;
  acquire(&p->parent->lock);
  uint64 ppid = getpid_by_proc(p->parent);
  release(&p->parent->lock);
  return ppid;
}

uint64 sys_getppid(void) {
  return getppid_by_proc(myproc());
}

uint64
sys_ps_list(void)
{
//    printf("ps_list()\n");
    int limit;
    argint(0, &limit);
    uint64 pids;
    argaddr(1, &pids);

    if (limit < 0) {
        printf("ps_list: limit cannot be less than 0\n");
        return -1;
    }

    int count = 0;
    extern struct proc proc[NPROC];
    extern struct nmspace nmspaces[NNMSPACES];
    extern struct spinlock wait_lock;
    extern struct spinlock pid_lock;

    int my_pids[limit];
    acquire(&pid_lock);
    acquire(&wait_lock);

    for (struct proc* p = proc; p < proc + NPROC; ++p) {
        acquire(&(p->lock));

        if (p->state == UNUSED) {
          release(&(p->lock));
          continue;
        }

        for (int current = p->nmspace_id; current >= 0;
             current = nmspaces[current].parent_id) {
          if (current == myproc()->nmspace_id) {
            if (count < limit) my_pids[count] = p->pid;
            ++count;
          }
        }

        release(&(p->lock));
    }

    release(&wait_lock);
    release(&pid_lock);

    if (copyout(myproc()->pagetable, pids, (void*)my_pids, (count > limit ? limit : count) * sizeof(int)) < 0) {
        printf("ps_list: error in executing copyout\n");
        return -1;
    }

    return count;
}

int
sys_ps_info(void) {

  int pid;
  argint(0, &pid);

  uint64 psinfo;
  argaddr(1, &psinfo);

  if (pid < 0) {
    printf("ps_info: limit cannot be less than 0\n");
    return -1;
  }

  extern struct proc proc[NPROC];
  extern struct spinlock wait_lock;
  extern struct spinlock pid_lock;
  struct process_info my_info;
  acquire(&pid_lock);
  acquire(&wait_lock);
  struct proc *p;
  for (p = proc; p < proc + NPROC; ++p) {
    acquire(&(p->lock));
    if (p->pid != pid) {
      release(&(p->lock));
      continue;
    }
    break;
  }

  if (p == proc + NPROC) {
    release(&wait_lock);
    release(&pid_lock);
    printf("ps_info: process with given pid (%d) not found\n", pid);
    return -1;
  }


  strncpy(my_info.name, p->name, sizeof(my_info.name));
  my_info.state = p->state;

  if (p->parent == 0)
    my_info.ppid = 0;
  else {
    acquire(&p->parent->lock);
    my_info.ppid = p->parent->pid;
    release(&p->parent->lock);
  }

  my_info.nmspace_id = p->nmspace_id;
  my_info.getppid = getppid_by_proc(p);
  my_info.getpid = getpid_by_proc(p);

  release(&(p->lock));
  release(&wait_lock);
  release(&pid_lock);


  if (copyout(myproc()->pagetable, psinfo, (void *)(&my_info),
              sizeof(my_info)) < 0) {
    printf("ps_info: error while executing copyout\n");
    return -1;
  }
  return 0;
}
