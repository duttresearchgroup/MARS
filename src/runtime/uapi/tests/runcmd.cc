
/*
 *
 * This helper program can be used to invoke other programs such that
 * we can call wait on it and it will not return just because the invoked
 * program received SIGCONT/SIGSTOP
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>


int main(int argc, char * argv[]){
    pid_t child_pid;
    int child_status;
    
    if(argc <= 1){
        return 0;
    }
    
    child_pid = fork();
    if(child_pid == 0) {
        execvp(argv[1], &argv[1]);
        printf("Error\n");
        return -1;
    }
    else {
        waitpid(child_pid,&child_status,0);
        return child_status;
  }
}
