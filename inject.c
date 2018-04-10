#define _GNU_SOURCE
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// yeah this doesn't do error checking, sue me
int main(int argc, char *argv[]) {
  int pid = atoi(argv[1]);

  /* - put the process in a freezer cgroup
   * - get the program counter from /proc/pid/syscall
   * - read /proc/pid/maps to find an executable map where to put some code
   * - save the content of that map with process_vm_readv
   * - replace it with process_vm_writev and put something in there that can move maps around and restore the original map
   * - replace the next instruction after the program counter with a jump to that area you just tampered with
   * - pull the process out of the freezer cgroup
   */

  /* update:
   * turns out linux won't let you write to non-writable pages with process_vm_writev
   * but you can happily write there via /proc/pid/mem
   * this makes so much sense
   */





#define cgroup "/sys/fs/cgroup/freezer/" USER "/"
  int fd = open(cgroup "tasks", O_WRONLY);
  dprintf(fd, "%d", pid);
  close(fd);

  fd = open(cgroup "freezer.state", O_WRONLY);
  dprintf(fd, "FROZEN");




  char path[255];
  sprintf(path, "/proc/%d/syscall", pid);
  FILE *f = fopen(path, "r");

  /* if this process was in userspace, the content looks like
   * -254 0x7ffec979a150 0x4000b0
   *   ^        ^            ^
   *   |        |            +---- rip
   *   |        +---- rsp
   *   +---- -1
   *
   * if it was in a syscall, the content looks like
   * 0 0x3 0x7ff6c00c9000 0x20000 0x22 0xffffffffffffffff 0x0 0x7ffe683e68d8 0x7ff6bfbf6701
   * ^  ^        ^            ^     ^        ^             ^        ^              ^
   * |  |        |            |     |        |             |        |              +---- rip
   * |  |        |            |     |        |             |        +---- rsp
   * |  |        |            |     |        |             +---- r9
   * |  |        |            |     |        +---- r8
   * |  |        |            |     +---- r10
   * |  |        |            +---- rdx
   * |  |        +---- rsi
   * |  +---- rdi
   * +---- rax
   */

  long regs[9];
  // we only care about rip
  long rip = regs[fscanf(f, "%ld %lx %lx %lx %lx %lx %lx %lx %lx",
                         &regs[0], &regs[1], &regs[2],
                         &regs[3], &regs[4], &regs[5],
                         &regs[6], &regs[7], &regs[8])-1];
  fclose(f);







  /* ok so there's a bunch of problems with this
   *
   * 1. rip could point to something close to the page boundary, and the next page could
   *    be non executable, in which case if we insert the jump we get a sigill
   * 2. rip could point to even beyond the page boundary, and the program could handle
   *    the sigsegv, and it wouldn't ever execute my jump
   * 3. assuming none of the above happens, because at least 2 looks very unlikely,
   *    we already /have/ an executable map, and we could just write our code here.
   *    but this opens yet another can of worms, like you start executing in the middle
   *    of the map and then you either write your code right after rip, or jump to the
   *    beginning and then you have to avoid the jump the next time you're here.
   * 4. you can legitimately have only one executable page in your entire program
   *
   * i don't want to deal with that, so let's just assume that we can get a nice page
   * elsewhere and jump there
   */
  sprintf(path, "/proc/%d/maps", pid);
  f = fopen(path, "r");
  long begin, end;
  char perm[] = "rwxp";
  while (fscanf(f, "%lx-%lx %s %*[^\n]", &begin, &end, perm) > 0)
    if (perm[2] == 'x' && !(begin <= rip && rip <= end))
      break;
  fclose(f);







  char saved[4096];
  struct iovec local = { .iov_base = saved,         .iov_len = 4096 },
              remote = { .iov_base = (void *)begin, .iov_len = 4096 };
  process_vm_readv(pid, &local, 1, &remote, 1, 0);
  puts("i just read");






  extern char code[4096];
  local.iov_base = code;
  sprintf(path, "/proc/%d/mem", pid);
  int mem = open(path, O_WRONLY);
  pwritev(mem, &local, 1, begin);
  puts("i just wrote");





  char buf[] =
    "\x48\xb8" "01234567" // movabs $whatever, %rax
    "\xff\xe0"            // jmp *%rax
    ;

  memcpy(buf+2, &begin, 8);

  local.iov_base = buf;
  local.iov_len = 12;
  pwritev(mem, &local, 1, rip);
  puts("i just wrote again");




  dprintf(fd, "THAWED");
}
