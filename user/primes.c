#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define PRIMES_MAX_NUM (35)

static void primeProc(int fdPipeIn)
{
    int firstNum = -1;
    int p[2];
    int readNum;
    int redRes;
    int pid;

    while ((redRes = read(fdPipeIn, &readNum, sizeof(readNum))) > 0) {
        if (firstNum == -1) { /* first time read number from pipe */
            firstNum = readNum;
            /* Create a pipe */
            if (pipe(p) < 0) {
                fprintf(2, "Pipe: Fail...\n");
                exit(1);
            }
            /* Fork */
            if ((pid = fork()) > 0) {
                /* Parent */
                close(p[0]); /* Close the read end of pipe.*/
                fprintf(1, "prime %d\n", readNum);
            } else if (pid == 0) {
                /* Child */
                close(p[1]); /* Close the write end of pipe.*/
                primeProc(p[0]); /* Pass the read end of pipe to primeProc */
                exit(0); /* End the child process */
            } else {
                fprintf(2, "Fork: Fail...\n");
                exit(1);
            }
            
        } else { /* Not the first time read number from pipe */
            if ((readNum % firstNum) !=  0) {
                /* Feed to the pipe */
                write(p[1], &readNum, sizeof(readNum));
            }
        }
    }
    /* Pipe is closed. */
    close(p[1]);
    wait(0);
}


int main(int argc, char *argv[])
{
    int p[2];
    int pid;

    if(argc > 1){
        fprintf(2, "Usage: primes...\n");
        exit(1);
    }

    if (pipe(p) < 0) {
        fprintf(2, "Pipe: Fail...\n");
        exit(1);
    }

    if ((pid = fork()) > 0) {
        /* Parent */
        close(p[0]); /* Close the read end of pipe.*/
        for (int i = 2; i <= PRIMES_MAX_NUM; i++) {
            write(p[1], &i, sizeof(i));
        }
        close(p[1]); /* Close the write end of pipe.*/
        wait(0);
    } else if (pid == 0) {
        /* Child */
        close(p[1]); /* Close the write end of pipe.*/
        primeProc(p[0]); /* Pass the read end of pipe to primeProc */
    } else {
        fprintf(2, "Fork: Fail...\n");
        exit(1);
    }

    exit(0);
}