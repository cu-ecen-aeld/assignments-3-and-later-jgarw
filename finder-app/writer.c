#include <stdio.h>
#include <syslog.h>

// create main method with argc to count arguments entered and argv[] to hold arguments
int main(int argc, char *argv[]){

    // Open a log session
    openlog(NULL, LOG_PID, LOG_USER);

    // Ensure that file name and string are entered after program is run
    if(argc < 3 ){
        syslog(LOG_ERR, "Error: Missing arguments.\n");
        return 1;
    }

    // assign arguments entered to variables
    char *writefile = argv[1];
    char *writestr = argv[2];

    // Open the specified file
    FILE *file = fopen(writefile, "w");

    // If file entered doesnt exist or cant be found
    if(file == NULL){
        syslog(LOG_ERR, "Error: File %s not found.\n", writefile);
        return 1;
    }

    // If an error occurs during the writing process
    if(fputs(writestr, file) == EOF){
        syslog(LOG_ERR, "Failed to write to file %s.\n", writefile);
        fclose(file);
        return 1;
    }

    // Log successful write using LOG_DEBUG
    syslog(LOG_DEBUG, "Writing \"%s\" to %s", writestr, writefile);

    // Close the file after writing
    fclose(file);

    // Close syslog
    closelog();

    // Exit successfully
    return 0;

}