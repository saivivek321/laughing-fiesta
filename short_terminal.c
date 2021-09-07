#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include<stdbool.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<signal.h>
#include<sys/wait.h>

#define BUFFERSIZE 256 //Max amount allowed to read from input
#define INITIAL_PROMPT "\t\tLab2_Assignment- Mini Linux Terminal\n" //initial display message
#define PROMPT "lab2_assignment >> " //Shell prompt
#define PROMPTSIZE sizeof(PROMPT) //sizeof shell prompt
#define ERROR -1 //for when an error is encountered
pid_t pid;

void display_Prompt(){printf("%s", PROMPT);}

// Function to manage file I/O redirection
void fileIOManager(char **argv, char *source, char *destination, int option){
    int fd; //file descriptor
    if((pid == fork()) == ERROR) {
        perror("Error: Unable to create child process.\n"),return;
    }
    if(pid == 0){
        //file output redirection
        if(option == 0){
            fd = open(destination, O_CREAT | O_TRUNC | O_WRONLY, 0600); //create a file for writing only
            dup2(fd, STDOUT_FILENO); //standard input is replaced with our file
            close(fd); //close file
        }
        //file input and output redirection
        if(option == 1){
            fd = open(source, O_RDONLY, 0600); //create a file for reading only
            dup2(fd, STDIN_FILENO);
            close(fd);
            fd = open(destination, O_CREAT | O_TRUNC | O_WRONLY, 0600); //same process for output redirection
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        if (execvp(argv[0], argv) == ERROR) {
            perror("Error: Command not found.\n");
            kill(getpid(), SIGTERM);
        }
    }
    waitpid(pid, NULL, 0);
}

// Function to manage piping
void pipeManager(char **argv){
    int fd1[2], fd2[2], commands_count = 0, aux0 = 0, aux1 = 0, aux2 = 0, end_of_Command;
    char *commTok[BUFFERSIZE];
    //calculate number of commands separated by '|'
    for(int i = 0; argv[i] != NULL; i++)
        if(strcmp(argv[i], "|") == 0) commands_count++;
    commands_count++;
    while(argv[aux0] != NULL && end_of_Command != 1) {
        aux1 = 0;
        //using auxiliary variables as indices and a pointer array to store the commands
        while (strcmp(argv[aux0], "|") != 0) {
            commTok[aux1] = argv[aux0];
            aux0++;
            aux1++;
            if (argv[aux0] == NULL) {
                end_of_Command = 1;
                break;
            }
        }
        commTok[aux1] = NULL;   //to mark the end of the command before being executed
        aux0++;
        //connect two commands' inputs and outputs
        if (aux2 % 2 == 0) pipe(fd2);
        else pipe(fd1);
        pid = fork();
        //close files if error occurs
        if (pid == ERROR) {
            if (aux2 != commands_count - 1) {
                if (aux2 % 2 == 0) close(fd2[1]);
                else close(fd1[1]);
            }
            perror("Error: Unable to create child process.\n");
            return;
        }
        if (pid == 0) {
            //first command: replace standard input
            if (aux2 == 0) dup2(fd2[1], STDOUT_FILENO);
            //last command: replace standard input for one pipe
            else if (aux2 == commands_count - 1) {
                if (commands_count % 2 == 0) dup2(fd2[0], STDIN_FILENO);
                else dup2(fd1[0], STDIN_FILENO);
                //command in the middle: use two pipes for standard input and output (for even and odd number of commands)
            } 
            else {
                if (aux2 % 2 == 0) {
                    dup2(fd1[0], STDIN_FILENO);
                    dup2(fd2[1], STDOUT_FILENO);
                } else {
                    dup2(fd2[0], STDIN_FILENO);
                    dup2(fd1[1], STDOUT_FILENO);
                }
            }
            if (execvp(commTok[0], commTok) == ERROR) {
                perror("Error: Unknown command entered.\n");
                kill(getpid(), SIGTERM); //terminate signal if an error is encountered
            }
        }
        //close file descriptors
        if (aux2 == commands_count - 1) {
            if (commands_count % 2 == 0) close(fd2[0]);
            else close(fd1[0]);
        } 
        else if (aux2 == 0) close(fd2[1]); 
        else {
            if (aux2 % 2 == 0) {
                close(fd1[0]);
                close(fd2[1]);
            }
            else {
                close(fd2[0]);
                close(fd1[0]);
            }
        }
        waitpid(pid, NULL, 0);
        aux2++;
    }
}

// Function to handle commands from user's input
// Functionality incomplete: cd only considers when nothing else is typed after cd (will change to home directory)
int Command_Execution(char *argv[]){
    bool flag = true;
    char *argvAux[BUFFERSIZE-1]; //since its a string and the last char in string is \n is removed from calc or command
    //SHELL COMMANDS
    if(strcmp(argv[0], "clear") == 0) system("clear"),printf("%s", INITIAL_PROMPT),flag = false;
    else if(strcmp(argv[0], "exit") == 0) exit(0),flag=false;
    else if(strcmp(argv[0], "cd") == 0) chdir(argv[1]),flag = false;
    if(flag){
      int i = 0, j = 0, background = 0, aux1, aux2, aux3;
      //puts the command into its own array by breaking from loop if '>', '<' or '&' is encountered
      while(argv[j] != NULL){
          if((strcmp(argv[j], ">") == 0) || (strcmp(argv[j], "<") == 0) || (strcmp(argv[j], "&") == 0))
              break;
          argvAux[j] = argv[j];
          j++;
      }
      while(argv[i] != NULL && background == 0){
          // check for any pipe in commands
          if(strcmp(argv[i], "|") == 0) {
              pipeManager(argv); //execute a command in the background
              return 1;
          }
          else if (strcmp(argv[i], "&") == 0) background = 1;//file I/O redirection
          else if (strcmp(argv[i], "<") == 0){
              aux1 = i+1;
              aux2 = i+2;
              aux3 = i+3;
              //if arguments after '<' are empty, return false
              if(argv[aux1] == NULL || argv[aux2] == NULL || argv[aux3] == NULL){
                  perror("Error: Insufficient amount of arguments are provided.\n");
                  return -1;
              }
              else{
                  //'>' would be two indices after '<'
                  if(strcmp(argv[aux2], ">") != 0) {
                      perror("Error: Did you mean '>' ?\n");
                      return -1;
                  }
              }
              fileIOManager(argvAux, argv[i+1], argv[i+3], 1);
              return 1;
              //file output redirection
          }
          else if(strcmp(argv[i], ">") == 0){
              if(argv[i+1] == NULL){
                  perror("Error: Insufficient amount of arguments are provided.\n");
                  return -1;
              }
              fileIOManager(argvAux, NULL, argv[i+1], 0);
              return 1;
          }
          i++;
      }
      argvAux[i] = NULL;
      if((pid = fork()) == ERROR) {
          perror("Error: Unable to create child process.\n");
          return -1;
      }
      //process creation (background or foreground)
      //CHILD
      if (pid == 0){
          signal(SIGINT, SIG_IGN); //ignores SIGINT signals
          //end process if non-existing commmands were used, executes command
          if (execvp(argvAux[0], argvAux) == ERROR) {
              perror("Error: Command not found.\n");
              kill(getpid(), SIGTERM);
          }
      }
      //PARENT
      if (background == 0) waitpid(pid, NULL, 0); //waits for child if the process is not in the background
      else printf("New process with PID, %d, was created.\n", pid);
    }
    return 1;
}

int main(int *argc, char **argv[]){
    char commandStr[BUFFERSIZE];//user input buffer
    char *commandTok[PROMPTSIZE]; //command tokens
    int numTok = 1;//counter for # of tokens
    pid = -10;  //a pid that is not possible
    printf("%s", INITIAL_PROMPT);
    while(1) {
        //print defined prompt
        display_Prompt();
        memset(commandStr, '\0', BUFFERSIZE); //memset will fill the buffer with null terminated characters, emptying the buffer
        fgets(commandStr, BUFFERSIZE, stdin); //stores user input into commandStr
        //considers the case if nothing is typed, will loop again
        if((commandTok[0] = strtok(commandStr, " \n\t")) == NULL) continue;
        //reset token counter to 1, then count all command tokens
        numTok = 1;
        while((commandTok[numTok] = strtok(NULL, " \n\t")) != NULL) numTok++;
        Command_Execution(commandTok);
    }
    exit(0);
}

/*
 **************  fork()  ********************************
 creates a new child process, takes no parameters and returns integer
 -ve return value = creation of child process is unsuccessful
 0 return value = returns the new child process
 +ve return value = returns the process id of new child process created

********************* strtok()  ***********************
 splits the given string based on the second parameter that is passed into the function

************************************************
strcmp() -> returns 0 if both the parameters passed are same else return something else

****************** chdir() ************************
chdir() -> changes the working directory with the path mentioned to it as a parameter
0 return value if the command is successful
-1 return value if the command execution fails
*/
