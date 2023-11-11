#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int numSleepTick;

  if(argc < 2){
    fprintf(2, "Usage: sleep time[s]...\n");
    exit(1);
  }

  numSleepTick = atoi(argv[1]);
  if (sleep(numSleepTick) < 0) {
    fprintf(2, "Fail to sleep...\n");
    exit(1);
  }

  exit(0);
}