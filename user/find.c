#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char*
fmtname(char *path)
{
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;
  return p;
}

void
find(char *path, char *target)
{
  struct stat st;
  char buf[512], *p;
  int fd;
  struct dirent de;

  if (stat(path, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    return;
  }

  switch(st.type){
  case T_FILE:
    if (strcmp(fmtname(path), target) == 0) {
      printf("%s\n", path);
    }
    break;

  case T_DIR:
    if((fd = open(path, 0)) < 0){
      fprintf(2, "find: cannot open %s\n", path);
      return;
    }

    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';

    // many records
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      
      if (!strcmp(".", de.name) || !strcmp("..",  de.name)) {
        continue;
      }

      memmove(p, de.name, strlen(de.name));
      p[strlen(de.name)] = 0;
      find(buf, target);
    }
    close(fd);
    break;
  }
}

int
main(int argc, char *argv[])
{
  if(argc != 3){
    fprintf(2, "Usage: find path keyword\n");
    exit(1);
  }

  find(argv[1], argv[2]);

  exit(0);
}