#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>

 //User defined Constants
#define BUILTIN  1
#define EXECUTED 0

 //Global Variables
bool userExit = false;
bool isBackground = false;

int inputType;

//-------------------------------------------------------------------------------------------------//


void handle_sigchild(){ // Used to log Children and get wait status for them.

    int waitStatus;
    int pid;
    char* data = "Child process was terminated\n";

    pid = waitpid(-1, &waitStatus, WNOHANG);

    FILE *filePointer;
    filePointer = fopen("./log.txt", "a");
    if(filePointer == NULL){
        printf("unable to create file.\n");
        exit(EXIT_FAILURE);
    }

    fprintf(filePointer, "%s", data);
    fflush(filePointer);

}

//-------------------------------------------------------------------------------------------------//
void sliceString(const char * source, char * destination, size_t start, size_t end)
{
    // Used to get a substring of a string by specifying index of start and end
    if(start>end)
        return;
    size_t j = 0;
    for ( size_t i = start; i <= end; ++i ) {
        destination[j++] = source[i];
    }
    destination[j] = 0;
}



//-------------------------------------------------------------------------------------------------//

char* getString(){

    // Used to get our input as one string and check for environment variables and convert them.

    char* string = (char*) malloc(sizeof(char));
    *string = '\0';

    int key;
    int sizer = 2;

    char sup[2] = {'\0'};

    while ( (key = getc(stdin)) != '\n')
    {
        string = (char*) realloc(string,sizer * sizeof(char));
        sup[0] = (char) key;
        strcat(string,sup);
        sizer++;

    }
    // Getting Input Done

    // Checking For Environment Variables and converting them.
    char *x = strstr(string, "$");
    if(x == NULL){
        return string;
    }

    char *y = strstr(x, " ");

    bool skipFlag;
    if(y == NULL){
        skipFlag = true;
    }
    else{
        skipFlag = false;
    }

    char *newString = (char*) malloc(1000*sizeof(char));

    char before[200];
    int before_index = x - string - 1;
    sliceString(string, before, 0, before_index);

    if(skipFlag){

        x = strtok(x, "$");

        char *substring = (char*) malloc(((strlen(x)+ 1) * sizeof(char)) );
        strcpy(substring, x);
        char* variable = getenv(substring);
        if(variable == NULL){
            variable = "";
        }

        strcat(newString, before);
        strcat(newString, variable);

        free(string);
        return newString;

    }
    else{
        char after[200];
        int after_index = y - string;
        sliceString(string, after, after_index, strlen(string) - 1);

        size_t substringLength = y - x;
        char *substring = (char*) malloc(((substringLength + 1) * sizeof(char)) );
        memcpy(substring, x, substringLength);
        substring = strtok(substring, "$");

        char* variable = getenv(substring);
        if(variable == NULL){
            variable = "";
        }

        strcat(newString, before);
        strcat(newString, variable);
        strcat(newString, after);

        free(string);
        return newString;
    }



}

//-------------------------------------------------------------------------------------------------//

// Parse Input into array of strings.
char **parseInput(char *data, char* seperators, int *count){
    int length = strlen(data);
    *count = 0;
    int i = 0;
    while (i < length){
        while (i < length){
            if (strchr(seperators, data[i]) == NULL)
                break;
            i++;
        }
        int temp = i;
        while (i < length){
            if (strchr(seperators, data[i]) != NULL)
                break;
            i++;
        }
        if(i > temp)
            *count = *count + 1;
    }

    char **strings = malloc(sizeof(char *) * *count);

    i = 0;
    char buffer[15000];
    int stringIndex = 0;
    while (i < length){
        while (i < length){
            if (strchr(seperators, data[i]) == NULL)
                break;
            i++;
        }
        int j = 0;
        while (i < length){
            if (strchr(seperators, data[i]) != NULL)
                break;
            buffer[j] = data[i];
            i++;
            j++;
        }
        if(j > 0){
            buffer[j] = '\0';
            int to_allocate = sizeof(char) * (strlen(buffer) + 1);
            strings[stringIndex] = malloc(to_allocate);
            strcpy(strings[stringIndex], buffer);
            stringIndex++;
        }
    }
    return strings;
}

//-------------------------------------------------------------------------------------------------//

//Printing Current Working Directory.
void printCWD(){

    char cwd[200];

    if(getcwd(cwd, 200) == NULL){
        if (errno == ERANGE)
            printf("[ERROR] pathname length exceeds the buffer size\n");
        else
            perror("getcwd");
        exit(EXIT_FAILURE);
    }

    fflush(stdout);
    printf("Current Working Directory: %s\n", cwd);

}

//-------------------------------------------------------------------------------------------------//

// Setup Environment To Home Directory.
void setupEnvironment(){
    char cwd[200];
    errno = 0;

    chdir(getenv("HOME"));

    if(getcwd(cwd, 200) == NULL){
        if (errno == ERANGE)
            printf("[ERROR] pathname length exceeds the buffer size\n");
        else
            perror("getcwd");
        exit(EXIT_FAILURE);
    }

    printf("Current Working Directory: %s\n", cwd);

}

//-------------------------------------------------------------------------------------------------//

// Evaluate and Validate input and handle.
void evaluateExpression(char **parsedInput, int *stringCount){


    if (*stringCount == 0){
        return;
    }

    if ( !(strcmp(parsedInput[0], "exit")) && (*stringCount == 1) ){
        userExit = true;
        printf("Bye\n");
        fflush(stdout);
        return;
    }


    // Check if command is built_in or executed
    if ( !(strcmp(parsedInput[0], "cd")) || !(strcmp(parsedInput[0], "echo")) || !(strcmp(parsedInput[0], "export")) )
        inputType = BUILTIN;
    else
        inputType = EXECUTED;

    // Check if command is background or foregorund.
    if ( !(strcmp(parsedInput[*stringCount-1], "&") ) ){
        isBackground = true;
        parsedInput[*stringCount-1] = '\0';
        *stringCount--;
    }


}

//-------------------------------------------------------------------------------------------------//

// Execute (cd, echo, and export)
void execBuiltIn(char **parsedInput, int stringCount){

    char* commandType = parsedInput[0];

    if ( !(strcmp(commandType, "cd")) ){

        if (stringCount == 1){
            chdir(getenv("HOME"));\
            return;
        }


        if ( !(strcmp(parsedInput[1], "~")) ){
            chdir(getenv("HOME"));
            return;
        }


        if (chdir(parsedInput[1]) != 0){
            perror("chdir() error()");
        }

    }

    if ( !(strcmp(commandType, "echo")) ){

        for(int i = 1; i < stringCount; i++){
            parsedInput[i] = strtok(parsedInput[i], "\"");
            if(parsedInput[i] != NULL){
                printf("%s ", parsedInput[i]);
            }

        }
        printf("\n");

    }

    if ( !(strcmp(commandType, "export")) ){

        if(stringCount < 4){
            printf("Wrong use of Export\n");
            return;
        }

        if( (strcmp(parsedInput[2], "=")) ){
            printf("Wrong use of Export\n");
            return;
        }

        char *variable = parsedInput[1];
        char *value = parsedInput[3];
        char space = ' ';

        for (int i = 3; i < stringCount-1; i++){
            strncat(value, &space, 1);
            strcat(value,  parsedInput[i+1]);

        }

        value = strtok(value, "\"");
        setenv(variable, value, 1);
    }

    return;
}

//-------------------------------------------------------------------------------------------------//

// Execute Other linux Kernel Commands.
int execCommand(char **parsedInput, int *stringCount){

    int pid = fork();

    //Check forking.
    if(pid == -1)
        exit(1);

    if(pid == 0){  //Child Process
        int err = execvp(parsedInput[0], parsedInput);
        if(err == -1){
            printf("Error command is not found\n");
        }

        exit(errno);
    }

    else{  // Parent Process

        int waitStatus;

        if(isBackground){ //Doesn't Wait for child if it's a background process
            printf("Hello I am a background process\n");
            waitpid(pid, &waitStatus, WNOHANG);
            if(WIFEXITED(waitStatus)){
                int statusCode = WEXITSTATUS(waitStatus);
                if(statusCode != 0) {
                    printf("Process Failed with status code %d\n", statusCode);
                }
            }
            isBackground = false;
        }

        else{ //Waits for child because it's a foreground process
            waitpid(pid, &waitStatus, 0);
            if(WIFEXITED(waitStatus)){
                int statusCode = WEXITSTATUS(waitStatus);
                if(statusCode != 0) {
                    printf("Process Failed with status code %d\n", statusCode);
                }
            }
        }

    }
}

//-------------------------------------------------------------------------------------------------//

// Flow Of Program.
void shell(){

    do{

        char *input = getString();
        int stringCount = 0;
        char **parsedInput = parseInput(input, " ", &stringCount);
        evaluateExpression(parsedInput, &stringCount);

        if(userExit){
            exit(0);
        }

        if(stringCount == 0)
            continue;
        else{
            if(inputType){
                execBuiltIn(parsedInput, stringCount);
            }
            else{
                execCommand(parsedInput, &stringCount);
            }
        }

        printCWD();
        free(input);
        free(parsedInput);

    }
    while(1);

    return;

}
//-------------------------------------------------------------------------------------------------//


int main(int argc, char* argv[]){

    signal(SIGCHLD, &handle_sigchild); // Handle Children Exiting.
    setupEnvironment();
    shell();

    return 0;
}


