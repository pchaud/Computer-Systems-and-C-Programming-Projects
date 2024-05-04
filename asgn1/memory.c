#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/limits.h>

int read_bytes(int infile, char *buf, int to_read) {
    int bytes_read = 0;
    int temp = 1;
    while (bytes_read < to_read && temp > 0) {
        temp = read(infile, buf, to_read - bytes_read);
        bytes_read += temp;
    }
    return bytes_read;
}

//ensures all bytes are wrote and returns the # of bytes written
//writes bytes from a buffer to outfile
int write_bytes(int outfile, char *buf, int to_write) {
    int bytes_wrote = 0;
    int temp = 1;
    //looping calls to write() until we have wither all bytes written out or no bytes written
    while (bytes_wrote < to_write && temp > 0) {
        temp = write(outfile, buf, to_write - bytes_wrote);
        bytes_wrote += temp;
    }
    //number of bytes written out is returned
    return bytes_wrote;
}

int main(void) {
    //file location, writing/reading the file, and commans

    //defining buffer for storing input
    char command_buffer[4];

    //setting up file's location buffer as PATH_MAX's size
    char location_buffer[PATH_MAX];

    //reading command from stdin
    read_bytes(STDIN_FILENO, command_buffer, 4);

    //if command get is received by user
    if (strcmp(command_buffer, "get\n") == 0) {

        //keeps track of all reads conducted
        int reads = 0;
        int total = 0;

        while (1) {
            reads = read_bytes(STDIN_FILENO, location_buffer + total, 1);
            if (reads == 0) {
                break;
            }

            total += reads;

            if (location_buffer[total - 1] == '\n') {
                break;
            }

            if (total > PATH_MAX) {
                fprintf(stderr, "Invalid Command\n");
                exit(1);
            }
        }

        location_buffer[total - 1] = '\0';

        //printf("%s\n", location_buffer);
        //open a file and reads it
        int file;
        file = open(location_buffer, O_RDONLY);

        if (file == -1) {
            fprintf(stderr, "Invalid Command\n");
            exit(1);
        }

        do {
            //open w a read only command
            reads = read_bytes(file, location_buffer, sizeof(location_buffer));

            if (reads < 0) {
                fprintf(stderr, "Invalid Command\n");
                exit(1);

            } else if (reads > 0) {
                int writes = 0;

                do {
                    int bytes_in_write
                        = write_bytes(STDOUT_FILENO, location_buffer + writes, reads - writes);

                    if (bytes_in_write <= 0) {
                        fprintf(stderr, "Invalid Command\n");
                        exit(1);
                    }

                    //incrementing reads with the data that just entered the write statement above
                    writes += bytes_in_write;

                    // continues executing until all bytes are written
                } while (writes < reads);
            }

            //continutes exeucting until all bytes are read
        } while (reads > 0);

        close(file);
    }

    //if command set is received by user
    /*if(strcmp(command_buffer, "set") == 0){

        char *filename = NULL;

        //open a file and reads it 
        int file;
        file = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);

        do{
            //open w a read only command
            reads = read_bytes(file, command_buffer, sizeof(command_buffer));
            //get file location from stdin, content_length, read content length bytes from content 
            if(reads < 0){
                fprintf(stderr, "Operation Failed to stderr\n");
                exit(1);

            } else if (reads > 0){
                    int writes = 0;

                    do{
                        int bytes_in_write = write_bytes(STDOUT_FILENO, command_buffer + writes, reads - writes);

                        if(bytes_in_write <= 0){
                            fprintf(stderr, "Operation Failed to stderr\n");
                            exit(1);
                        }

                        //incrementing reads with 
                        writes += bytes_in_write;
                    
                    // continues executing until all bytes are written
                    } while(writes < reads);

           
            } 
        //continutes exeucting until all bytes are read
        } while(reads > 0);
    } */

    else {
        fprintf(stderr, "Invalid Command\n");
        return -1;
    }

    return 0;
}
