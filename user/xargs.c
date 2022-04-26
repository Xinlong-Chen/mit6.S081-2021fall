#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

#define MAX_LEN 512

int getline(char *buf, int max)
{
  int i, cc;
  char c;

  for(i=0; i+1 < max; ){
    cc = read(0, &c, 1);
    if(cc < 1) {
      return 0;
    }
    if(c == '\n' || c == '\r')
      break;
    buf[i++] = c;
  }
  buf[i] = '\0';
  printf("str: %s\n", buf);
  return 1;
}

int
main(int argc, char *argv[])
{
  // echo hello too | xargs echo bye
  char buffer[MAX_LEN];
  char* argv_tmp[MAXARG];
  // minus "xargs"
  memcpy(argv_tmp, argv + 1, (argc - 1) * sizeof(char*));
  while (getline(buffer, MAX_LEN))
  {
    if (fork() == 0) {
      argv_tmp[argc - 1] = buffer;
      exec(argv_tmp[0], argv_tmp);
      exit(0);
    } else {
      wait(0);
    }
  }
  exit(0);
}