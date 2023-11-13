#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define PRIMES_MAX_NUM (35)

int main(int argc, char *argv[])
{
    if(argc > 1){
        fprintf(2, "Usage: primes...\n");
        exit(1);
    }
    
    exit(0);
}