#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

#define MAX_POOL_SIZE (100)

int
main(int argc, char *argv[])
{
    char c;
    int pid;
    int idxArg;
    int idxCh;
    char *myArgv[MAXARG];
    char argPool[MAX_POOL_SIZE];
    char *pArg;

    if(argc < 2){
        fprintf(2, "Usage: [cmd] | xargs [cmd] [arg]...\n");
        exit(1);
    }

    /* Initialize a new argv for exec */
    for (idxArg = 0; idxArg < (argc - 1); idxArg++) {
        myArgv[idxArg] = argv[idxArg + 1];
    }

    idxArg = argc - 1;
    myArgv[argc - 1] = 0;
    idxCh = 0;
    pArg = argPool;
    while(read(0, &c, 1) > 0) {
        if (c == ' ') {
            argPool[idxCh++] = '\0'; /* Null-terminated */
            idxArg++; /* Proceed to next arg */
            myArgv[idxArg] = 0; /* Default as 0 */
            pArg = &argPool[idxCh]; /* pArg point to the beginning of the next string in the pool */
            if (idxArg >= MAXARG) {
                fprintf(2, "too many args...\n");
                exit(1);
            }
        } else if (c == '\n') {
            argPool[idxCh] = '\0'; /* Null-terminated */
            myArgv[idxArg + 1] = 0; /* Next arg as 0 */
            if ((pid = fork()) > 0) {
                /* Parent */
                /* Recover the argv back to original one */
                idxArg = argc - 1;
                myArgv[argc - 1] = 0;
                idxCh = 0;
                pArg = argPool;
                wait(0);
            } else if (pid == 0) {
                /* Child */
                if (exec(myArgv[0], myArgv) < 0) {
                    fprintf(2, "exec fail...\n");
                    exit(1);
                }
            } else {
                fprintf(2, "fork fail...\n");
                exit(1);
            }
        } else {
            myArgv[idxArg] = pArg;
            argPool[idxCh++] = c;
            if (idxCh >= MAX_POOL_SIZE) {
                fprintf(2, "arg too long...\n");
                exit(1);
            }
            
        }
    }


    exit(0);
}