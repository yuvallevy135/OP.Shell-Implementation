/*
 * Author: Yuval Levy
 * ID:     205781966
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <stdlib.h>

#define ZERO_CHAR '\0'
#define QUOTE '"'
#define DONT_WAIT "&"
typedef struct CommandStruct {
    int numberOfArgs;
    char *commandLine;
    char **commandArgs;
    bool waitForSonFlag;
    char *isDone;
    pid_t pid;
} CommandStruct;

void myShell();

CommandStruct getMyStruct(char *lineCommand);

int cmdExec(CommandStruct *allMyCommands, int numberOfAllCmds, char *prevDir);

int notBuildInExec(CommandStruct *allMyCommands, int numberOfAllCmds, char *prevDir);

int buildInExec(CommandStruct *allMyCommands, int numberOfAllCmds);

void checkDoneCmd(CommandStruct *allMyCommands, int numberOfAllCmds);

/**
 * Function Name: myShell
 * Function Input: None
 * Function Output: void
 * Function Operation: the function run in a loop and gets the user's command until it gets 'exit'.
 */
void myShell() {

    CommandStruct myCommandStruct;
    // Create an arr with max 100 structs.
    CommandStruct allMyCommands[100] = {0};
    char *prevDir = (char *) calloc(500, sizeof(char));
    int shouldStop = 1;
    int numberOfAllCmds = 0;
    // Run in a loop until it gets 'exit'
    while (shouldStop) {
        char *line;
        line = (char *) calloc(100, sizeof(char));
        printf("> ");
        fgets(line,100,stdin);
        line[strlen(line)-1] = '\0';
        int commandLen = strlen(line);
        line[commandLen] = ZERO_CHAR;
        if (strcmp(line, "") == 0) {
            continue;
        }
        // Create a struct out of the command.
        myCommandStruct = getMyStruct(line);
        // Add it to the arr.
        allMyCommands[numberOfAllCmds] = myCommandStruct;
        numberOfAllCmds++;
        // See if its a build in command or not.
        shouldStop = cmdExec(allMyCommands, numberOfAllCmds, prevDir);
    }
}

/**
 * Function Name: getMyStruct
 * Function Input: char *lineCommand
 * Function Output: CommandStruct
 * Function Operation: the function gets a command and return a struct with all the info we need.
 */
CommandStruct getMyStruct(char *lineCommand) {
    CommandStruct myCommandStruct;
    myCommandStruct.commandArgs = (char **) calloc(100, sizeof(char *));
    myCommandStruct.commandLine = (char *) calloc(100, sizeof(char));
    // Indicates if the command is still RUNNING or DONE.
    myCommandStruct.isDone = "RUNNING";
    myCommandStruct.waitForSonFlag = true;
    int index = 0;
    char echoString[100] = {0};
    char echoNoQuote[100] = {0};
    bool quotFlag = false;
    char *token = strtok(lineCommand, " ");
    char tempLine[100] = {0};
    bool singleQuotFlag = false;
    // loop through the string to extract all other tokens
    while (token != NULL) {
        if( (strlen(token) == 1) && (token[0] == '"')){
            token = strtok(NULL, " ");
            continue;
        }
        strcat(tempLine, token);
        strcat(tempLine, " ");
        // using for echo - see if it has quotes or not and if do then take them off.
        if (token[0] == QUOTE) {
            strcat(echoString, token);
            strcat(echoString, " ");
            memcpy(echoNoQuote, token + 1, strlen(token) - 1);
            strcpy(token, echoNoQuote);
            quotFlag = true;
        }
        if (token[strlen(token) - 1] == QUOTE) {
            if (quotFlag) {
                token[strlen(token) - 1] = '\0';
            } else {
                strcat(echoString, token);
                strcat(echoString, " ");
                memcpy(echoNoQuote, token, strlen(token) - 1);
                strcpy(token, echoNoQuote);
            }
        }
        myCommandStruct.commandArgs[index] = token;
        index++;
        token = strtok(NULL, " ");
    }
    tempLine[strlen(tempLine) - 1] = '\0';
    strcpy(myCommandStruct.commandLine, tempLine);
    // Check if this command will run in the background or foreground.
    if (strcmp(myCommandStruct.commandArgs[(index - 1)], DONT_WAIT) == 0) {
        myCommandStruct.waitForSonFlag = false;
        myCommandStruct.commandArgs[index - 1] = NULL;
        int len = strlen(myCommandStruct.commandLine);
        // Delete the '&' out of the args.
        myCommandStruct.commandLine[len - 1] = '\0';
        myCommandStruct.commandLine[len - 2] = '\0';
    }
    myCommandStruct.commandArgs[index] = NULL;
    myCommandStruct.numberOfArgs = index;
    return myCommandStruct;
}

/**
 * Function Name: cmdExec
 * Function Input: CommandStruct *allMyCommands, int numberOfAllCmds, char *prevDir
 * Function Output: int
 * Function Operation: the function gets the arr of commands and decide if the current cmd is a build in cmd or not.
 */
int cmdExec(CommandStruct *allMyCommands, int numberOfAllCmds, char *prevDir) {
    int isOkExec;
    if ((strcmp(allMyCommands[numberOfAllCmds - 1].commandArgs[0], "jobs") == 0) ||
        (strcmp(allMyCommands[numberOfAllCmds - 1].commandArgs[0], "history") == 0) ||
        (strcmp(allMyCommands[numberOfAllCmds - 1].commandArgs[0], "cd") == 0) ||
        (strcmp(allMyCommands[numberOfAllCmds - 1].commandArgs[0], "exit") == 0)) {
        isOkExec = notBuildInExec(allMyCommands, numberOfAllCmds, prevDir);
    } else {
        isOkExec = buildInExec(allMyCommands, numberOfAllCmds);
    }
    return isOkExec;
}

/**
 * Function Name: notBuildInExec
 * Function Input: CommandStruct *allMyCommands, int numberOfAllCmds, char *prevDir
 * Function Output: int
 * Function Operation: This function is a not build in command, which means either "cd", "exit", "jobs' or "history"
 */
int notBuildInExec(CommandStruct *allMyCommands, int numberOfAllCmds, char *prevDir) {
    pid_t pid = (long) getpid();
    // Add the pid to the current cmd.
    allMyCommands[numberOfAllCmds - 1].pid = pid;
    // Covers the jobs command.
    if (strcmp(allMyCommands[numberOfAllCmds - 1].commandArgs[0], "jobs") == 0) {
        // Before we print all the commands that are running in the backgrond we want to check if either one of
        // them is DONE already.
        checkDoneCmd(allMyCommands, numberOfAllCmds);
        allMyCommands[numberOfAllCmds - 1].isDone = "DONE";
        int i;
        // Loop over all the structs and print only the ones that are still RUNNING.
        for (i = 0; i < numberOfAllCmds; i++) {
            if (strcmp(allMyCommands[i].isDone,"RUNNING") == 0) {
                printf("%d ", allMyCommands[i].pid);
                printf("%s\n", allMyCommands[i].commandLine);
            }
        }
        // Covers the history command.
    } else if (strcmp(allMyCommands[numberOfAllCmds - 1].commandArgs[0], "history") == 0) {
        // Before we print all the commands that are running in the backgrond we want to check if either one of
        // them is DONE already.
        checkDoneCmd(allMyCommands, numberOfAllCmds);
        int i;
        // Loop over all the structs and print all the commands.
        for (i = 0; i < numberOfAllCmds; i++) {
            printf("%d ", allMyCommands[i].pid);
            printf("%s ", allMyCommands[i].commandLine);
            printf("%s\n", allMyCommands[i].isDone);
        }
        // Covers the exit command.
    } else if (strcmp(allMyCommands[numberOfAllCmds - 1].commandArgs[0], "exit") == 0) {
        printf("%d ", allMyCommands[0].pid);
        return 0;
        // Covers the cd command.
    } else if (strcmp(allMyCommands[numberOfAllCmds - 1].commandArgs[0], "cd") == 0) {
        allMyCommands[numberOfAllCmds - 1].pid = getpid();
        printf("%d\n", allMyCommands[numberOfAllCmds - 1].pid);
        char currDir[100];
        // Get current directory.
        getcwd(currDir, 100);
        // Checks if there are to many args.
        if (allMyCommands[numberOfAllCmds - 1].numberOfArgs > 2) {
            fprintf(stderr, "Error: Too many arguments\n");
            return 1;
        }
            // CD with no args.
        else if (allMyCommands[numberOfAllCmds - 1].numberOfArgs == 1) {
            return 1;
        }
            // CD and '~' go to home directory.
        else if (strcmp(allMyCommands[numberOfAllCmds - 1].commandArgs[1], "~") == 0) {
            if (chdir(getenv("HOME")) != 0) {
                fprintf(stderr, "Error in system call\n");
                return 1;
            } else {
                strcpy(prevDir, currDir);
//                printf("%s\n", currDir);
                return 1;
            }
        }
            // CD and '-' go to prev directory.
        else if (strcmp(allMyCommands[numberOfAllCmds - 1].commandArgs[1], "-") == 0) {
            if (prevDir[0] == '\0') {
                return 1;
            } else {
                if (chdir(prevDir) != 0) {
                    fprintf(stderr, "Error in system call\n");
                    return 1;
                } else {
                    strcpy(prevDir, currDir);
//                    printf("%s\n", currDir);
                    return 1;
                }
            }
            // CD and ".." - go to the home directory
        } else if (strcmp(allMyCommands[numberOfAllCmds - 1].commandArgs[1], "..") == 0) {
            if (chdir("..") != 0) {
                fprintf(stderr, "Error in system call\n");
                return 1;
            } else {
                strcpy(prevDir, currDir);
//                printf("%s\n", currDir);
                return 1;
            }
            // Regular CD and a path.

        } else if (chdir(allMyCommands[numberOfAllCmds - 1].commandArgs[1]) == 0) {
            strcpy(prevDir, currDir);
//            printf("%s\n", currDir);
            return 1;
            // This is for the '~/..' case.
        }else if(allMyCommands[numberOfAllCmds - 1].commandArgs[1][0] == '~'){
            char temp[100] = {0};
            char final[100] = {0};
            memcpy(temp,allMyCommands[numberOfAllCmds - 1].commandArgs[1] + 1,strlen(allMyCommands[numberOfAllCmds - 1].commandArgs[1])-1);
            strcat(final,getenv("HOME"));
            strcat(final,temp);
            if (chdir(final) == 0){
                strcpy(prevDir, currDir);
//                printf("%s\n", currDir);
                return 1;
            }else{
                fprintf(stderr, "Error in system call\n");
                return 1;
            }
        } else {
                fprintf(stderr, "Error: No such file or directory\n");
            return 1;
        }
    }
}

/**
 * Function Name: buildInExec
 * Function Input: CommandStruct *allMyCommands, int numberOfAllCmds
 * Function Output: int
 * Function Operation: This function is a  build in command. It will run the command in a different process using
 * execvp.
 */
int buildInExec(CommandStruct *allMyCommands, int numberOfAllCmds) {
    pid_t pid;
    int status;
    // Create a new process.
    if ((pid = fork()) < 0)
        printf("Error in system call\n");
    else {
        if (pid == 0) {
            // The child process.
            if (execvp(allMyCommands[numberOfAllCmds - 1].commandArgs[0],
                       allMyCommands[numberOfAllCmds - 1].commandArgs) == -1) {
                fprintf(stderr, "Error in system call\n");
                return 0;
            }
            return 0;
        } else {
            // The parent process.
            allMyCommands[numberOfAllCmds - 1].pid = pid;
            printf("%d\n", allMyCommands[numberOfAllCmds - 1].pid);
            // Check if we need to wait for the child process.
            if (allMyCommands[numberOfAllCmds - 1].waitForSonFlag) {
                waitpid(pid, &status, 0);
                allMyCommands[numberOfAllCmds - 1].isDone = "DONE";
            }
        }
    }
    return 1;
}

/**
 * Function Name: checkDoneCmd
 * Function Input: CommandStruct *allMyCommands, int numberOfAllCmds
 * Function Output: void
 * Function Operation: This function gets the arr and loop over all the commands and check if any of them is
 * DONE and we can change its "isDone" member.
 */
void checkDoneCmd(CommandStruct *allMyCommands, int numberOfAllCmds) {
    int i, status;
    for (i = 0; i < numberOfAllCmds - 1; i++) {
        if ((strcmp(allMyCommands[i].isDone,"RUNNING") == 0) && waitpid(allMyCommands[i].pid, &status, WNOHANG)) {
            allMyCommands[i].isDone = "DONE";
        }
    }
}

int main() {
    myShell();
    return 0;
}