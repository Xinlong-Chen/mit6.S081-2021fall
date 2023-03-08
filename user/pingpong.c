#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int p[2]; // read, write
  char buf[2];
  if(argc != 1){
    fprintf(2, "Usage: pingpong\n");
    exit(1);
  }

  pipe(p);
  if (fork() == 0) { // child
    read(p[0], buf, 1);
    fprintf(1, "%d: received ping\n", getpid());
    write(p[1], "c", 1);
    exit(0);
  } else { // father
    sleep(1);
    write(p[1], "c", 1);
    sleep(1);
    read(p[0], buf, 1);
    fprintf(1, "%d: received pong\n", getpid());
  }

  exit(0);
}