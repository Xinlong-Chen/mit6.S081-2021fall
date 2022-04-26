#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MAX_SEQ 35
#define BUFSIZE 100

int getline(int fd, char *buf, int max)
{
    int i, cc;
    char c;

    for (i = 0; i + 1 < max;)
    {
        cc = read(fd, &c, 1);
        if (cc < 1)
        {
            return 0;
        }
        if (c == '\n' || c == '\r')
            break;
        buf[i++] = c;
    }
    buf[i] = '\0';
    return i;
}

int getnum(char *buf, int *pos)
{
    int num = 0, i = *pos;
    while(buf[i] >= '0' && buf[i] <= '9') {
        num = num * 10 + buf[i] - '0';
        i++;
    }
    *pos = i - 1;
    return num;
}

int primer(int read_fd)
{
    char buf[BUFSIZE];
    int len = getline(read_fd, buf, BUFSIZE);
    // printf("%s\n", buf);
    close(read_fd);

    int pipe_fd[2];
    pipe(pipe_fd);

    int i = 0, first_print_flag = 1;
    int is_have = 0;
    int first_num = getnum(buf, &i), num_tmp;
    printf("prime %d\n", first_num);
    for (i = 0; i < len; ++i) {
        if (buf[i] >= '0' && buf[i] <= '9') {
            num_tmp = getnum(buf, &i);
            if (num_tmp % first_num == 0) {
                continue;
            }
            is_have = 1;
            break;
        }
    }
    if (is_have) {
        if (fork() == 0) { // child
            close(pipe_fd[1]);
            primer(pipe_fd[0]);
            exit(0);
        } else {
            close(pipe_fd[0]);
            for (i = 0; i < len; ++i) {
                if (buf[i] >= '0' && buf[i] <= '9') {
                    num_tmp = getnum(buf, &i);
                    if (num_tmp % first_num == 0) {
                        continue;
                    }
                    if (first_print_flag) {
                        fprintf(pipe_fd[1], "%d", num_tmp);
                        first_print_flag = 0;
                    } else {
                        fprintf(pipe_fd[1], " %d", num_tmp);
                    }
                }
            }
            fprintf(pipe_fd[1], "\n", num_tmp);
            close(pipe_fd[1]);
            wait(0);
        }
    } else {
        close(pipe_fd[0]);
        close(pipe_fd[1]);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    int pipe_fd[2]; // read, write
    if (argc != 1)
    {
        fprintf(2, "Usage: primes\n");
        exit(1);
    }

    pipe(pipe_fd);
    if (fork() == 0)
    { // child
        close(pipe_fd[1]);
        primer(pipe_fd[0]);
        exit(0);
    }
    else
    { // father
        close(pipe_fd[0]);
        for (int i = 2; i <= MAX_SEQ; ++i)
        {
            fprintf(pipe_fd[1], "%d", i);
            if (i != MAX_SEQ)
            {
                fprintf(pipe_fd[1], " ", i);
            }
            else
            {
                fprintf(pipe_fd[1], "\n", i);
            }
        }
        close(pipe_fd[1]);
        wait(0);
    }

    exit(0);
}