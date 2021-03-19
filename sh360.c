/*
 * Rodrigo He
 * CSC 360 - Spring 2021
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wait.h>

/******************************* D E F I N E S *******************************/
#define MAX_NUM_ARGS 7      // Max number of arguments
#define MAX_INPUT_LEN 80    // Max length of input line
#define MAX_PROMPT_LEN 10   // Max length of prompt
#define MAX_NUM_DIR 10      // Max number of directories in a single path
/******************************* D E F I N E S *******************************/

/********************* G L O B A L    V A R I A B L E S **********************/
char *execve_args[MAX_NUM_ARGS + 1];        // args for execve
int execve_args_len;                        // number of element in execve_args[]
char *execve_args_head[MAX_NUM_ARGS + 1];   // args for exec_pipe
char *execve_args_tail[MAX_NUM_ARGS + 1];   // args for exec_pipe
/*****************************************************************************/

/*
 * void exec_pipe(char *, char *[], char *, char *[])
 * 
 * Description:
 *      Given an input of the form 
 *          PP <command1> -> <command2>
 *      This function uses fork(), execve(), and pipe() to connect the output 
 *      from cmd_head to the input of cmd_tail, where cmd_head = command1 and 
 *      cmd_tail = command2
 * 
 * The following code was borrowed from Dr. Zastre
 *      Author: Dr. Zastre
 *      Title/Program Name: appendix_d.c
 */
void exec_pipe(char *envpath_head, char *cmd_head[MAX_NUM_ARGS + 1], char *envpath_tail, char *cmd_tail[MAX_NUM_ARGS + 1]){
    char *envp[] = { 0 };
    int status;
    int pid_head, pid_tail;
    int fd[2];

    pipe(fd);

    // printf("parent: setting up piped commands...\n");
    if ((pid_head = fork()) == 0) {
        // printf("child (head): re-routing plumbing; STDOUT to pipe.\n");
        dup2(fd[1], 1);
        close(fd[0]);
        execve(envpath_head, cmd_head, envp);
        // printf("child (head): SHOULDN'T BE HERE.\n");
    }

    if ((pid_tail = fork()) == 0) {
        // printf("child (tail): re-routing plumbing; pipe to STDIN.\n");
        dup2(fd[0], 0);
        close(fd[1]);
        execve(envpath_tail, cmd_tail, envp);
        // printf("child (tail): SHOULDN'T BE HERE.\n");
    }

    close(fd[0]);
    close(fd[1]);

    // printf("parent: waiting for child (head) to finish...\n");
    waitpid(pid_head, &status, 0);
    // printf("parent: child (head) is finished.\n");

    // printf("parent: waiting for child (tail) to finish...\n");
    waitpid(pid_tail, &status, 0); 
    // printf("parent: child (tail) is finished.\n");
}


/*
 * void exec_cmd(char *, char *[], int);
 * 
 * Description:
 *      If cmd_flag = 0 then the input is in the form of  
 *          <arg1> <arg2> <arg3>...
 *      The function will use fork() and execve() to create a child process
 *      and that runs the commands found within *args[]
 *   
 *      If cmd_flag = 1 then the input is in the form of
 *         OR <command> -> <outputfile> 
 *      The function will user fork(), execve(), and dup2() to redirect the
 *      output of the command to the given outputfile 
 * 
 * The following code was borrowed from Dr. Zastre
 *      Author: Dr. Zastre
 *      Title/Program Name: appendinx_b.c, appendix_c.c
 */
void exec_cmd(char *arg, char *args[MAX_INPUT_LEN + 1], int cmd_flag){
    char *envp[] = { 0 };
    int pid, fd;
    int status, execve_val;

    if ((pid = fork()) == 0) {
        if(cmd_flag == 1){
            fd = open(execve_args[execve_args_len - 1], O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
            if (fd == -1) {
                fprintf(stderr, "\n     Error - [Input] Could not open: %s\n\n", execve_args[execve_args_len - 1]);
                exit(1);
            }
            execve_args[execve_args_len - 1] = '\0'; // removing output file so it matches the format { "/bin/ls", "-1", 0 };
            dup2(fd, 1);
            dup2(fd, 2);
        }
        execve_val = execve(arg, args, envp);
        fprintf(stderr, "\n     Error - [exec_cmd] Could not execute:\n              %s\n\n", arg);
        exit(1);
    }
    if(cmd_flag == 0){
        while (wait(&status) > 0) {
            // printf("        parent: child is finished.\n");
        }
    }
    if(cmd_flag == 1){
        waitpid(pid, &status, 0);
    }
}

/*
 * void prep_pp(char *[], int, char [][], int);
 * 
 * Description:
 *      Takes input in the form of 
 *          PP <cmd1> <arg1> -> <cmd2> <arg2>
 *      
 *      The function parses the input to the proper format and inserts into
 *      and array called cmd_head and cmd_tail, where cmd_head contains info
 *      to run cmd1 and cmd_tail info to run cmd2.
 *     
 *      
 * 
 */
void prep_pp(char *input_tokens[MAX_NUM_ARGS], int num_tokens, char envpath_arr[20][MAX_INPUT_LEN], int envpath_len){
    char envpath_head[MAX_INPUT_LEN + 1];   // Contains information to run "head" cmd
    char envpath_tail[MAX_INPUT_LEN + 1];   // Contains information to run "tail" cmd
    int arrow_index;    // Tracks the index of "->"

    /*
     *  This and the next loop parses the input tokens  to the proper format and
     *  inserts into the arrays called cmd_head and cmd_tail
     */
    int i;
    for(i = 2; i < num_tokens; i++){    // i = 2 b/c token[0] = PP and token[1] = command
        if(strncmp(input_tokens[i], "->", MAX_INPUT_LEN) == 0){
            arrow_index = i;
            break;
        }
        execve_args_head[i - 1] = input_tokens[i];
    }
    execve_args_head[i] = '\0';
    
    int j;
    for(j = arrow_index + 2; j < num_tokens; j++){  // j = arrow_index + 2 to skip "->" and cmd2
        execve_args_tail[j - arrow_index - 1] = input_tokens[j];
    }
    execve_args_tail[j - arrow_index] = '\0';

    /*
     *  This and the following loop iterates through each path in the .sh360rc 
     *  file and concatenates the cmd to the and of the path, 
     *      e.g. /path/ -> /path/cmd
     *  then uses access() to check if the binary file exists and is executable
     */
    int f;
    for(f = 1; f < envpath_len; f++){
        strncpy(envpath_head, envpath_arr[f], MAX_INPUT_LEN);   // copy path
        
        if(envpath_arr[f][strlen(envpath_arr[f]) - 1] != '/'){
            strncat(envpath_head, "/", MAX_INPUT_LEN);          // /path/path -> /path/path/
        }
        
        strncat(envpath_head, input_tokens[1], MAX_INPUT_LEN);  // /path/path/cmd_input
        
        if(access(envpath_head, F_OK & X_OK) == 0){ // checking if binary file exists
            execve_args_head[0] = envpath_head;
            break;
        }
    }
    
    int g;
    for(g = 1; g < envpath_len; g++){
        strncpy(envpath_tail, envpath_arr[g], MAX_INPUT_LEN);   // copy path
        
        if(envpath_arr[g][strlen(envpath_arr[g]) - 1] != '/'){
            strncat(envpath_tail, "/", MAX_INPUT_LEN);          // /path/path -> /path/path/
        }
        
        strncat(envpath_tail, input_tokens[arrow_index + 1], MAX_INPUT_LEN);
        
        if(access(envpath_tail, F_OK & X_OK) == 0){ // checking if binary file exists
            execve_args_tail[0] = envpath_tail;
            break;
        }
    }

    // Send all args/paths to exec_pipe for the commands to be executed.
    exec_pipe(envpath_head, execve_args_head, envpath_tail, execve_args_tail);
}


int main(int argc, char *argv[]){

    // ----------------------- START FILE READER -----------------------
    char envpath_arr[MAX_INPUT_LEN][MAX_INPUT_LEN]; // Holds data from .sh360rc
    FILE *sh360rc;
    sh360rc = fopen(".sh360rc", "r");
    
    if(sh360rc == NULL){
        fprintf(stderr, "\n     Error - [File] Unable to read .sh360rc\n\n");
        exit(1);
    }

    /* 
     *  The following code snipped borrowed from 
     *  https://stackoverflow.com/questions/13566082/how-to-check-if-a-file-has-content-or-not-using-c
     */
    if (sh360rc != NULL) {
        fseek(sh360rc, 0, SEEK_END);
        int size = ftell(sh360rc);        
        if (size == 0) {
            fprintf(stderr, "\n     Error - [File] The file .sh360rc is empty\n\n");
            exit(1);
        }
        fseek(sh360rc, 0, SEEK_SET);
    }
    

    int envpath_len = 0;    // Number of distinct directories in .sh360rc
    int cur_line_len = 0;   // Stores length of cur line read from .sh360rc
                            // Used to remove '\0' character from string

    /**
     *  Loop thorugh .sh360rc file, checks for a newline character and adds a 
     *  null-pointer character in its places, then adds line to envpath_arr
     */
    while(envpath_len < MAX_INPUT_LEN){
        fgets(envpath_arr[envpath_len], MAX_INPUT_LEN, sh360rc);
        
        cur_line_len = strlen(envpath_arr[envpath_len]);

        // Remove <new line> from input
        if (envpath_arr[envpath_len][cur_line_len - 1] == '\n'){
            envpath_arr[envpath_len][cur_line_len - 1] = '\0';
        }

        envpath_len++;
        if(feof(sh360rc)){
            break;
        }

    }
    // ----------------------- END FILE READER -----------------------


    // ----------------------- START INPUT READER -----------------------
    char input[MAX_INPUT_LEN];          // holds user input
    char *input_tokens[MAX_NUM_ARGS];   // holds tokenized user input
    int num_tokens;                     // tracks the number of tokens

    int input_flag; // Keeps track of errors in the user input
                    // 0 = no input error, 1 = input error
     
    /*
     *  Infite loop, propms user for input, and parses commands, if user
     *  send 'exit' as a sole command the program ends.
     */
    while(1){
        input_flag = 0;
        fprintf(stdout, "%s ", envpath_arr[0]);         // Start with prompt, e.g. uvic %
        fflush(stdout);
        fgets(input, MAX_INPUT_LEN, stdin);             // Get user command
        
        if (input[strlen(input) - 1] == '\n') {         // Replace \n with null-pointer char
            input[strlen(input) - 1] = '\0';
        }

        if(strncmp(input, "exit", MAX_INPUT_LEN) == 0){ // Check if user sent 'exit'
            exit(0);
        }

        // ----------------------- START INPUT TOKENS -----------------------
        /*
         *  The following code snippet is based on appendix_a.c by Dr. Zastre
         */
        num_tokens = 0;
        char *token = strtok(input, " ");
        while(token != NULL){
            if(num_tokens == MAX_NUM_ARGS){ // '==' since num_tokens start at 0
                fprintf(stderr, "\n     Error - [Input] Max number of args is 7\n\n");
                input_flag = 1;
                break;
                // exit(0);
            }
            input_tokens[num_tokens] = token;
            num_tokens++;
            token = strtok(NULL, " ");
        }
        // ----------------------- END INPUT TOKENS -----------------------

        // ----------------------- START PATH PARSING -----------------------

        int cmd_flag = 0;   // Keeps track of the command format, 0 = regular, 1 = OR
        int outfile_index;  // Keeps track of the output file index for the OR cmd
        int arrow_flag = 1; // Keeps track of "->", Has "->" = 0, Missing "->" = 1;

    
        // Determing if user has types the OR cmd, and if the cmd is in the correct format
        if(num_tokens != 0 && strncmp(input_tokens[0], "OR", 3) == 0){
            cmd_flag = 1;
            for(outfile_index = 0; outfile_index < num_tokens; outfile_index++){
                if(strncmp(input_tokens[outfile_index], "->", 3) == 0){
                    arrow_flag = 0;
                    if(outfile_index + 1 >= num_tokens){    // No output file = error
                        fprintf(stderr, "\n     Error - [Command] No output file provided\n             Please follow the format 'OR <command> -> <output file>'\n\n");
                        input_flag = 1;
                        break;
                    }
                    outfile_index++;
                    break;
                }
            }
            if(arrow_flag == 1){    // Missing "->" = error
                fprintf(stderr, "\n     Error - [Command] Missing '->'\n             Please follow the format 'OR <command> -> <output file>'\n\n");
                input_flag = 1;
            }

        }

        // Determing if user has typed the PP cmd, and calling other functions
        if(num_tokens != 0 && strncmp(input_tokens[0], "PP", 3) == 0){
            int arrow;
            for(arrow = 0; arrow < num_tokens; arrow++){
                if(strncmp(input_tokens[arrow], "->", 3) == 0){
                    arrow_flag = 0;
                    if(arrow + 1 >= num_tokens){    // No output cmd = error
                        fprintf(stderr, "\n     Error - [Command] No second to pipe\n             Please follow the format 'PP <command1> -> <command2>'\n\n");
                        input_flag = 1;
                        break;
                    }
                    arrow++;
                    break;
                }
            }
            if(arrow_flag == 1){    // Missing "->" = error
                fprintf(stderr, "\n     Error - [Command] Missing '->'\n             Please follow the format 'PP <command1> -> <command2>'\n\n");
                input_flag = 1;
            }
            
            if(input_flag != 1 && num_tokens > 1){
                prep_pp(input_tokens, num_tokens, envpath_arr, envpath_len);
                input_flag = 1;
            }
        }
        // ----------------------- END PATH PARSING -----------------------


        // ----------------------- START COMMAND FILTER -----------------------
        int f;
        char envpath[MAX_INPUT_LEN + 1];    // +1 for the null-pointer char
        int num_tries = 0;                  // tracks the number of paths tried
        
        if(num_tokens != 0 && input_flag == 0){
            execve_args_len = 0;            // Num of elements in execve_args[]
            for(f = 1; f < envpath_len; f++){
                strncpy(envpath, envpath_arr[f], MAX_INPUT_LEN);    // copy path to temp var
                if(envpath_arr[f][strlen(envpath_arr[f]) - 1] != '/'){
                    strncat(envpath, "/", MAX_INPUT_LEN); // Concatenate "/" if missing, e.g. /path => /path
                }

                if(cmd_flag == 1){  // Parsing cmd to OR format.
                    strncat(envpath, input_tokens[1], MAX_INPUT_LEN);   // Concatenate cmd to path, e.g. /path/ => /path/cmd
                    int x;
                    int j = 1;
                    for(x = 2; x < num_tokens; x++){    // x = 2 b/c want {"/path/cmd", "arg", 0}
                        if(strncmp(input_tokens[x], "->", MAX_INPUT_LEN) != 0){
                            // printf("%d: %s \n", x, input_tokens[x]);
                            execve_args[j] = input_tokens[x];
                            j++;
                        }
                    }
                    execve_args[num_tokens - 2] = '\0'; // adding null-point char
                    execve_args_len = num_tokens - 2;   // -2 since we didn't add 'OR' and '->'
                }else{
                    strncat(envpath, input_tokens[0], MAX_INPUT_LEN);   // Concatenate cmd to path, e.g. /path/ => /path/cmd
                    int z;
                    for(z = 1; z < num_tokens; z++){    // want {"/path/cmd", "arg", 0}
                        execve_args[z] = input_tokens[z];   
                    }
                    execve_args[num_tokens] = '\0'; // adding null-point char
                    execve_args_len = num_tokens;
                }
                execve_args[0] = envpath; // making the "/path/cmd" the first element

                if(access(envpath, F_OK & X_OK) == 0){  // checking if "/path/cmd" exists and is executable
                    exec_cmd(envpath, execve_args, cmd_flag);
                    break;
                }

                num_tries++;    // if num_tries == number of paths then cmd doesn't exist in given paths
                if(num_tries + 1 == envpath_len){
                    fprintf(stderr, "\n     Error - [PATH] Could not find binary command in paths given by .sh360rc\n\n");
                }
            }
        }
        
        // The following erases the array contents for next user command
        memset(execve_args, 0, MAX_NUM_ARGS + 1);
        memset(envpath, 0, MAX_INPUT_LEN);
        memset(input, 0, MAX_INPUT_LEN);
        memset(input_tokens, 0, MAX_NUM_ARGS);
        memset(execve_args_head, 0, MAX_NUM_ARGS + 1);
        memset(execve_args_tail, 0, MAX_NUM_ARGS + 1);
    }
}
