#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

static void searchDir(char *dir, char *target)
{
    char curPath[64];
    char *p;
    int fdDir;
    struct dirent de;
    struct stat st;

    /* Make the dir suffix with '/' */
    strcpy(curPath, dir);
    p = curPath + strlen(dir);
    *p++ = '/';

    if ((fdDir = open(dir, 0)) >= 0) {
        while(read(fdDir, &de, sizeof(de)) == sizeof(de)) {
            /* Traverse the entry in the directory */
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = '\0';
            if (de.inum != 0) {
                if (stat(curPath, &st) == 0) {
                    if (st.type == T_DIR) {
                        if ((strcmp(p, ".") != 0) && (strcmp(p, "..") != 0)) {
                            searchDir(curPath, target);
                        }
                    } else {
                        /* T_FILE, T_DEVICE */
                        if (strcmp(p, target) == 0) {
                            fprintf(1, "%s\n", curPath);
                        }
                    }
                } else {
                    fprintf(1, "stat Fail...\n");
                    exit(1);
                }
            }
        }
    } else {
        fprintf(2, "open Fail...\n");
        exit(1);
    }


}

int
main(int argc, char *argv[])
{

  if(argc != 3){
    fprintf(2, "usage: find [dir] [file]...\n");
    exit(0);
  }
  searchDir(argv[1], argv[2]);

  exit(0);
}