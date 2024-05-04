#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <regex.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <bits/fcntl-linux.h>

#include "debug.h"
// #include "rwlock.h"
#include "queue.h"
#include "protocol.h"
#include "debug.h"
#include "asgn2_helper_funcs.h"

#define BUFFER_SIZE   2048
#define HTTP_REGEX    "^([a-zA-Z]{1,8}) /([a-zA-Z0-9.-]{1,63}) (HTTP/[0-9].[0-9])\r\n.*$"
#define CONTENT_REGEX "^([a-zA-Z0-9.-]{1,128}): ([ -~]{1,128})\r\n.*$"

typedef struct {
    char *command;
    char *uri;
    char *version;
    char *message_body;
    int request_id;
    int content_length;
    int remainder;
    int sock;
    int file;
    char buf[BUFFER_SIZE];
} Fields;

//a4: adding pointer to queue struct
queue_t *queue;

//a4: TODO: ADD MUTEX FOR ARRAY

//asg2 httpserver.c's parse function:

//manually print out each field to test
//using resource's memory_parsing.c as guidance
void parse(int fd, Fields *f, char *buf) {
    regex_t reg;
    regmatch_t matches[4];

    int rc = regcomp(&reg, HTTP_REGEX, REG_EXTENDED);

    if (rc != 0) {
        dprintf(f->sock, "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 22\r\n\r\nInternal "
                         "Server Error\n");
        return;
    }
    rc = regexec(&reg, buf, 4, matches, 0);

    //if a match is found
    if (rc == 0) {
        f->command = buf;
        f->uri = buf + matches[2].rm_so;
        f->version = buf + matches[3].rm_so;
        // null-terminate
        for (int i = 1; i < 4; i++) {
            buf[matches[i].rm_eo] = '\0';
        }
        //need this line so as to make space in the buffer for \r\n after version and before content length
        buf += matches[3].rm_eo + 2;

        //anytime incrementing buf w latest chunk -> need to decrement remainder
        f->remainder -= (matches[3].rm_eo + 2);

    } else {
        dprintf(f->sock, "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n");
        regfree(&reg);
        close(fd);
        return;
    }

    //content length initialized to -1 to show it hasn't been found yet
    f->content_length = -1;

    //2nd regex expression: defining and recompiling content length and message body
    rc = regcomp(&reg, CONTENT_REGEX, REG_EXTENDED);
    rc = regexec(&reg, buf, 3, matches, 0);

    while (rc == 0) {
        //print out this from .so
        buf[matches[1].rm_eo] = '\0';

        //fprintf(stderr, "Substring from matches[1].rm_so to end of buffer: %s\n", &buf[matches[1].rm_so]);

        buf[matches[2].rm_eo] = '\0';

        //fprintf(stderr, "Substring from matches[2].rm_so to end of buffer: %s\n", &buf[matches[2].rm_so]);

        if (strncmp(buf, "Content-Length", 15) == 0) {
            int header_length = atoi(buf + matches[2].rm_so);

            //fprintf(stderr, "Header Length: %d\n", header_length);

            if (errno == EINVAL) {
                dprintf(
                    f->sock, "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n");
            }
            f->content_length = header_length;
        }

        //asgn4: request id
        if (strncmp(buf, "Request-Id", 10) == 0) {
            int header_length = atoi(buf + matches[2].rm_so);

            //fprintf(stderr, "Header Length: %d\n", header_length);

            if (errno == EINVAL) {
                dprintf(
                    f->sock, "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n");
            }
            f->request_id = header_length;
        }
        //keep parsing through buffer
        //end of version + /r/n
        buf += (matches[2].rm_eo + 2);

        f->remainder -= (matches[2].rm_eo + 2);

        //finally execute regex for content-length through message body
        rc = regexec(&reg, buf, 3, matches, 0);
    }
    // moved this to outside the loop cuz it was checking for the \r\n even if there were more headers
    //might always fail check what rc is equal to after parsing happens
    if (buf[0] == '\r' && buf[1] == '\n') {
        f->message_body = buf + 2;
        f->remainder -= 2;
    } else {
        //fprintf(stderr, "u think u the shit?\n");
        dprintf(f->sock, "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n");
        regfree(&reg);
        return;
    }

    //fprintf(stderr, "content length: %d\n", f->content_length);
    //returning 0 to indicate a success and exit(1) used throughout my code is to indicate failure
    regfree(&reg);
}

//asg2 httpserver.c's get function:

//writing to socket in get and main
//writing to file descriptor in put
int get(Fields *f) {

    //iget_dir.sh test case
    //ensures the filename isn't leading to a directory

    int file_dir;

    if ((file_dir = open(f->uri, O_DIRECTORY)) != -1) {
        dprintf(f->sock, "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n");
        fprintf(stderr, "GET,/%s,403,%d\n", f->uri, f->request_id);
        return (1);
    }

    if (f->uri == NULL) {
        dprintf(f->sock, "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n");
        fprintf(stderr, "GET,/%s,403,%d\n", f->uri, f->request_id);
        return (1);
    }

    //opening file only to read
    int file = open(f->uri, O_RDONLY, 0666);

    //if the file couldn't be opened -- might have to get rid of this
    if (file == -1) {

        //Permission denied
        if (errno == EACCES) {
            dprintf(f->sock, "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n");
            fprintf(stderr, "GET,/%s,403,%d\n", f->uri, f->request_id);

            //No such file or directory
        } else if (errno == ENOENT) {
            dprintf(f->sock, "HTTP/1.1 404 Not Found\r\nContent-Length: 10\r\n\r\nNot Found\n");
            fprintf(stderr, "GET,/%s,404,%d\n", f->uri, f->request_id);

            //in case the file couldn't open for any other reason 500 signal error prints
        } else {
            dprintf(f->sock, "HTTP/1.1 500 Internal Server Error \r\nContent-Length: "
                             "22\r\n\r\nInternal Server Error\n");
            fprintf(stderr, "GET,/%s,500,%d\n", f->uri, f->request_id);
        }
    } else {
        //handle size and send ok response
        //goes to the end of file
        int file_size = lseek(file, 0, SEEK_END);

        //reset so cursor can go back and read
        lseek(file, 0, SEEK_SET);

        dprintf(f->sock, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n", file_size);
        fprintf(stderr, "GET,/%s,200,%d\n", f->uri, f->request_id);

        int pass_size = pass_n_bytes(file, f->sock, file_size);

        if (pass_size == -1) {
            dprintf(f->sock, "HTTP/1.1 500 Internal Server Error\r\nContent-Length: "
                             "22\r\n\r\nInternal Server Error\n");
        }
    }

    close(file);
    return 0;
}

//asg2 httpserver.c's put function:

int put(Fields *f) {

    //read extra stuff that's not put in the buffer -> read until reads in chunks until /r/r/n
    //read extra parts -> read_n_bytes to read rest of it into file

    if (f->content_length == -1) {
        dprintf(f->sock, "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n");
        return (1);
    }

    //iget_dir.sh test case
    //ensures the filename isn't leading to a directory

    if (open(f->uri, O_DIRECTORY) != -1) {
        dprintf(f->sock, "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n");
        fprintf(stderr, "PUT,/%s,403,%d\n", f->uri, f->request_id);
        return (1);
    }

    if (f->uri == NULL) {
        dprintf(f->sock, "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n");
        fprintf(stderr, "PUT,/%s,403,%d\n", f->uri, f->request_id);
        return (1);
    }

    int file = open(f->uri, O_WRONLY | O_CREAT | O_TRUNC, 0666);

    // If file already exists
    if (errno == EEXIST) {

        file = open(f->uri, O_CREAT | O_WRONLY | O_TRUNC, 0666);
        dprintf(f->sock, "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nOK\n");
        fprintf(stderr, "PUT,/%s,200,%d\n", f->uri, f->request_id);

        //if file wasn't created for some reason -> error message deploys
        if (file == -1) {
            dprintf(f->sock, "HTTP/1.1 500 Internal Server Error r\nContent-Length: "
                             "22\r\n\r\nInternal Server Error\n");
            fprintf(stderr, "PUT,/%s,500,%d\n", f->uri, f->request_id);
            return (1);
        }

    } else if (file != -1) {
        dprintf(f->sock, "HTTP/1.1 201 Created\r\nContent-Length: 8\r\n\r\nCreated\n");
        fprintf(stderr, "PUT,/%s,201,%d\n", f->uri, f->request_id);

    } else if (errno == EACCES) {
        // Permission denied or other error
        dprintf(f->sock, "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n");
        fprintf(stderr, "PUT,/%s,403,%d\n", f->uri, f->request_id);
        close(file);
        return (1);
    }

    //writing message body to file
    ssize_t write_bytes = write_n_bytes(file, f->message_body, f->remainder);

    //CHANGE: wasnt here before
    if (write_bytes == -1) {
        dprintf(f->sock, "HTTP/1.1 500 Internal Server Error\r\nContent-Length: "
                         "22\r\n\r\nInternal Server Error\n");
        fprintf(stderr, "PUT,/%s,500,%d\n", f->uri, f->request_id);
        close(file);
        return (1);
    }

    int leftover = (f->content_length - f->remainder);

    int pass_written_bytes = pass_n_bytes(f->sock, file, leftover);

    if (pass_written_bytes == -1) {
        dprintf(f->sock, "HTTP/1.1 500 Internal Server Error\r\nContent-Length: "
                         "22\r\n\r\nInternal Server Error\n");
        fprintf(stderr, "PUT,/%s,500,%d\n", f->uri, f->request_id);
        close(file);
        return (1);
    }

    //content_length minus amount read from buffer
    //need to keep track of how much I'm reading

    close(file);
    return 0;
}

//while loop in asgn2
//constantly checking for elements in queue
//if finds it pop it
//do request stuff

void *werk_thread() {
    //intiializing buffer used to store the bytes read and written between the server and client
    char buf[BUFFER_SIZE + 1] = "";
    while (1) {

        //asgn4: queue pop and open connection
        uintptr_t accept_sock = -1;
        queue_pop(queue, (void **) &accept_sock);

        if (accept_sock > 0) {
            Fields f;

            f.sock = accept_sock;

            //reading bytes that came from the socket attatched to a port
            int read_bytes = read_until(accept_sock, buf, BUFFER_SIZE, "\r\n\r\n");
            f.remainder = read_bytes;

            //return value for read_until is -1 which indicates error
            if (read_bytes == -1) {
                dprintf(accept_sock,
                    "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n");
                return 0;
            }

            //igetdir maybe?
            if (read_bytes < 1) {
                dprintf(f.sock, "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n");
                return 0;
            }

            //f.sock seems wrong
            parse(accept_sock, &f, buf);

            if (strncmp(f.version, "HTTP/1.1", 8) != 0) {
                //fprintf(stderr, "wom p womp");
                dprintf(accept_sock, "HTTP/1.1 505 Version Not Supported\r\nContent-Length: "
                                     "22\r\n\r\nVersion Not Supported\n");
                return 0;
            }

            int is_get = strcmp(f.command, "GET") == 0;
            int is_put = strcmp(f.command, "PUT") == 0;

            if (!is_get && !is_put) {
                dprintf(f.sock,
                    "HTTP/1.1 501 Not Implemented\r\nContent-Length: 16\r\n\r\nNot Implemented\n");
            }

            if (is_get) {
                get(&f);
            } else if (is_put) {
                put(&f);
            }

            close(accept_sock);

            memset(buf, '\0', sizeof(buf));
        }
    }
}

// only pushing requests into queue
int main(int argc, char *argv[]) {

    if (argc < 2) {
        exit(1);
    }

    //a4: configure command line -t to recogize threads command (which is defaulted to 4)
    int thread_count = 4;

    int opt;
    while ((opt = getopt(argc, argv, "t:")) != -1) {

        switch (opt) {
        case 't': thread_count = atoi(optarg); break;
        default: break;
        }
    }

    //initializing socket with helper function
    Listener_Socket sock;

    //creating my port by converting given argument to an int
    int port = atoi(argv[optind]);

    //initializing a listen_struct to a given port
    int init_sock = listener_init(&sock, port);

    if (init_sock == -1) {
        fprintf(stderr, "Invalid Port Number\n");
        exit(1);
    }
    //a4: intializing queue for threads to push/pop into
    queue = queue_new(thread_count);

    //a4: initialize worker threads
    //a4: using better_lock_unlock.c from rwlock_users to understand how to initialize threads

    pthread_t threads[thread_count];

    for (int i = 0; i < thread_count; i++) {
        pthread_create(&(threads[i]), NULL, werk_thread, NULL);
    }

    //a4: dispatcher thread -> accepting requests from socket
    //pushing said requests to queue for worker threads

    while (1) {
        uintptr_t listen_request = listener_accept(&sock);
        queue_push(queue, (void *) listen_request);
    }

    return 0;
}
