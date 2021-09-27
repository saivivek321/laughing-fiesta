#include<stdio.h>     //Standard Library
#include<stdlib.h>    //Standard Library
#include<string.h>    //To perform string operations
#include<sys/types.h> //for fork()
#include<unistd.h>    //for fork()
#include <stdbool.h>  // for boolean variable usage
#include<sys/wait.h>  // for wait() function
#include<sys/stat.h>  
#include<fcntl.h>     //pipe() function call

#define INITIAL_MSG "\n\n-------------------------Sai Vivek - Own Terminal-------------------------\n\n" //Inital Promt Message 
#define MSG "\033[36msai_vivek_mini_terminal $ "//for msg to be printed in green color
#define MSGSIZE 128 //sizeof shell prompt



void execute(char **inputcommands){ // Main execution of code is done using execvp command
    pid_t pid = fork();  //Creating a child process
    if (pid == -1) {
        printf("\nFailed forking child..\n");
        return;
     } 
    else if (pid == 0) { //Inside the Child Process
        if (execvp(inputcommands[0], inputcommands) < 0) {
            printf("\nPlease Enter a Valid command..\n");
        }
        exit(0);
    } 
    else { //Parent Process
        wait(NULL); // waiting for child to terminate
        return;
    }
}

int FileManager(char **inputcommands,int n){ // I/O redirections are done here 
  
   pid_t pid;
  
        if(strcmp(inputcommands[2],">") == 0 && n==4){ //Output Redirection

            dup2(1,3);//Storing the standard output on ID 3
            int fd = open(inputcommands[3],O_WRONLY); //create a file for writing only
            dup2(fd,1); //standard input is replaced with our file
            close(fd); //close file
            inputcommands[2]='\0';  //removing >
            inputcommands[3]='\0';  // removing destination file

                execute(inputcommands);// Execute the command
          
            dup2(3,1);//Bring back the standard output to terminal
            printf("--Output Redirection Successful--\n");
            return 1;
        }

         
        else if(strcmp(inputcommands[1],"<")==0 && strcmp(inputcommands[0],"cat")==0 && n==3){  // Input Redirection

            // Can be Done directly by removing "<" symbol and execute it

            // char *cmd[3];
            // cmd[0]=inputcommands[0];
            // cmd[1]=inputcommands[2];
            // cmd[2]='\0';
            // execute(cmd);

            //Other way:
                FILE * file;
                if (file = fopen(inputcommands[2], "r"))  //opeing the file needed to be printed
                {
                 char c = fgetc(file);
                while (c != EOF) // Printing char by char
                  {
                   printf ("%c", c);
                   c = fgetc(file);
                   }
                    fclose(file);
                 
                 }
                else
                {
                 printf("--File doesn't exist--\n");
                }
            return 1;
            
        }
        else{
            printf("Use > or < for redirection with correct parameters \n");// If paraments are not assignmed properly
        }

}
 
int execute_pipe_commands(char **first,char ** second)// Executes the 2 arrays by piping 
 {
    int pipefd[2]; 
    pid_t p1, p2;
  
    if (pipe(pipefd) < 0) {
        printf("\nPipe could not be Created \n");
        return -1;
    }
    p1 = fork();
    if (p1 < 0) {
        printf("\nSorry Could not fork \n");
        return -1;
    }
  
    if (p1 == 0) { 
        // First array (i.e. Child 1 executing )
        // It only needs to write at the write end
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
  
        if (execvp(first[0], first) < 0) { //Executing 1st Part of Pipe
            printf("\nCould not execute command 1st part of Pipe\n");
            return -1;

        }

    } else {
        //Second part of pipe (i.e. Parent executing)
        p2 = fork();
  
        if (p2 < 0) {
            printf("\nCould not fork 2nd part of pipe \n");
            return -1;
        }
  
        //Child 2 executing
        //It only needs to read at the read end
        if (p2 == 0) {
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
       
             if (execvp(second[0], second) < 0) { //Executing 2nd part of pipe by taking 1st part as input
            printf("\nCould not execute 2nd part of pipe \n");
            return -1;
            }

        } else {
            // parent executing, waiting for two children
            wait(NULL);
           
        }
    }
 }

int pipeManager(char **inputcommands,int n){ //Divides the Input commands into 2 arrays based on pipe position 

      char *first[MSGSIZE];
      char *second[MSGSIZE];
       memset(first, '\0', MSGSIZE);
       memset(second, '\0', MSGSIZE);
      int i=0;
      for(i=0;i<n;i++){ //1st part of pipe
        if(strcmp(inputcommands[i],"|")==0){i++;break;}
            first[i]=inputcommands[i];
      }
      int j=0;
      for(;i<n;i++){ //Second part of pipe
        second[j++]=inputcommands[i];
      }
      
       execute_pipe_commands(first,second); // Executes the 2 arrays by piping 

}

int Execute_the_Commands(char **inputcommands,int n,bool pipePresent){  // Main Execution Code
    if(strcmp(inputcommands[0], "clear") == 0) system("clear"),printf("%s", INITIAL_MSG); // To clear the terminal
    else if(strcmp(inputcommands[0], "exit") == 0) {printf("Have a nice day !\n");exit(0);}  // To exit from terminal 
    else if(strcmp(inputcommands[0], "cd") == 0) chdir(inputcommands[1]);  // To change the present working directory
    else if(pipePresent){
         pipeManager(inputcommands,n); //If Any Pipe is there in InputCommands
    }    
    else{      
        bool contains=0;
        for(int i=0;i<n;i++){
            if(strcmp(inputcommands[i],"<")==0 || strcmp(inputcommands[i],">")==0){contains=1;break;}
        }
        if(!contains){execute(inputcommands);return 1;} // For formal codes without redirection and piping 
        else {
            FileManager(inputcommands,n); //If redirection is Envolved 
            return 1;
        }
        
    } 
}
int main(int *argc, char **argv[]){

  char inputstring[MSGSIZE];//input from user is stored
  char *inputcommands[MSGSIZE]; //input is split and stored in array

  
  printf("%s",INITIAL_MSG);//Inital message 
  while(1) {
    
    printf("%s",MSG);//print default msg(promt) 
    printf("%s","\033[0m");//make the color of msg to white
    memset(inputstring, '\0', MSGSIZE); //Clear the inputstring Array
    fgets(inputstring, MSGSIZE, stdin); //stores user input into inputstring
    
    if((inputcommands[0] = strtok(inputstring, " \n")) == NULL) continue;//If input given is empty
    

    int n=1;
    bool isPipeThere=0;
    while((inputcommands[n] = strtok(NULL, " \n")) != NULL){
        if(strcmp(inputcommands[n], "|")==0)isPipeThere=1; //flag raised if pipe is present
         n++;
         }
     
    Execute_the_Commands(inputcommands,n,isPipeThere);   // Main Execution Code
   
  }
  exit(0);
}
