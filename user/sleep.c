#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int ticks_num;

  if(argc != 2){
    fprintf(2, "Usage: sleep times\n");
    exit(1);
  }
  
  ticks_num = atoi(argv[1]);
  sleep(ticks_num);

  exit(0);
}
