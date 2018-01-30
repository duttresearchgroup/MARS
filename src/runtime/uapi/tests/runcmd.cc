/*******************************************************************************
 * Copyright (C) 2018 Tiago R. Muck <tmuck@uci.edu>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

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
