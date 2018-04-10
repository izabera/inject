#include <unistd.h>
#include <stdio.h>

int main() {
  printf("my pid is %d\n", getpid());
  while(1) sleep(5);
}
