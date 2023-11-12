#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    char chBuf[1];
    int pipeParentToChild[2];
    int pipeChildToParent[2];
    int pid;

    if(argc > 1){
        fprintf(2, "Usage: pingpong...\n");
        exit(1);
    }

    /*Create pipes to connect between parent and child  */
    if ((pipe(pipeParentToChild) < 0) || (pipe(pipeChildToParent) < 0)) {
        fprintf(2, "pipe: Fail...\n");
    }
#ifdef DEBUG_PINGPONG
    fprintf(1, "pipeParentToChild: %d, %d, \n", pipeParentToChild[0], pipeParentToChild[1]);
    fprintf(1, "pipeChildToParent: %d, %d, \n", pipeChildToParent[0], pipeChildToParent[1]);
#endif

    if ((pid = fork()) < 0) {
        fprintf(2, "fork: Fail...\n");
    } else if (pid == 0) {
        /* Child Process */
        close(pipeParentToChild[1]); /* Close unnecessary file */
        close(pipeChildToParent[0]); /* Close unnecessary file */
        if (read(pipeParentToChild[0], chBuf, sizeof(chBuf)) > 0) {
            fprintf(1, "%d: received ping\n", getpid());
            write(pipeChildToParent[1], chBuf, sizeof(chBuf));
        } else {
            fprintf(1, "read: End of File...\n");
        }
        close(pipeParentToChild[0]); /* No need anymore */
        close(pipeChildToParent[1]); /* No need anymore */
    } else {
        /* Parent Process */
        close(pipeParentToChild[0]); /* Close unnecessary file */
        close(pipeChildToParent[1]); /* Close unnecessary file */
        write(pipeParentToChild[1], chBuf, sizeof(chBuf));
        close(pipeParentToChild[1]);
        if (read(pipeChildToParent[0], chBuf, sizeof(chBuf)) > 0) {
            fprintf(1, "%d: received pong\n", getpid());
        } else {
            fprintf(1, "read: End of File...\n");
        }
        close(pipeParentToChild[1]); /* Close unnecessary file */
        close(pipeChildToParent[0]); /* Close unnecessary file */
        wait(0);
    }

    exit(0);
}