#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "logger.h"

#define MAX_STR_LEN 8192

/**
 * Generates an HTTP 1.1 compliant timestamp for use in HTTP responses.
 *
 * Inputs:
 *  - timestamp: character pointer to a string buffer to be filled with the
 *    timestamp.
 */
void generate_timestamp(char *timestamp)
{
    time_t now = time(0);
    struct tm time = *gmtime(&now);
    strftime(timestamp, sizeof(timestamp), "%a, %d %b %Y %H:%M:%S %Z", &time);
}

char *next_char(char **str_ptr, const char *delim)
{
    size_t token_start = strspn(*str_ptr, delim);
    size_t token_end = strcspn(*str_ptr + token_start, delim);
    
    if(*str_ptr == NULL)
    {
        return NULL;
    }

    if (token_end <= 0)
    {
        *str_ptr = NULL;
        return NULL;
    }

    char *current_ptr = *str_ptr + token_start;

    *str_ptr += token_start + token_end;

    if (**str_ptr == "\0")
    {
        *str_ptr = NULL;
    }
    
    else
    {
        **str_ptr = "\0";
        (*str_ptr)++;
    }

    return current_ptr;
}

/**
 * Reads from a file descriptor until:
 *  - the newline ('\n') character is encountered
 *  - *length* is exceeded
 *  This is helpful for reading HTTP headers line by line.
 *
 * Inputs:
 *  - fd: file descriptor to read from
 *  - buf: buffer to store data read from *fd*
 *  - length: maximum capacity of the buffer
 *
 * Returns:
 *  - Number of bytes read;
 *  - -1 on read failure
 *  - 0 on EOF
 */
ssize_t read_line(int fd, char *buf, size_t length)
{
    ssize_t sum = 0;

    while(sum < length)
    {
        size_t read_size = read(fd, buf + sum, 1);

        if(*(buf + sum) == "\n")
        {
            break;
        }

        if (read_size == -1)
        {
            perror("read");
            
            return -1;
        }
        
        else if (read_size == 0)
        {
            return 0;
        }

        sum += read_size;
    }

    return sum;
}

int file_not_found(int client_fd)
{
    char buf[MAX_STR_LEN] = {0};
    char timestamp[128];
    char *error = "404";

    generate_timestamp(timestamp, 128);

    sprintf(buf, "HTTP/1.1 404 Not Found\r\n"
        "Date: %s\r\n"
        "Content-Length: %zu\r\n"
        "\r\n"
        "%s",
        timestamp, strlen(error), error);

    int ret = write(client_fd, buf, strlen(buf));

    if(ret == -1)
    {
        return -1;
    }
    
    return 0;
}

int handle_request(int fd)
{
    LOGP("Handling Request\n");
   
    char path[MAX_STR_LEN] = {0};
    
    while(true)
    {
        char header_str[MAX_STR_LEN] = {0};
 
        ssize_t read_size = read_line(client_fd, header_str, MAX_STR_LEN);
        
        if (read_size == 0 || read_size == -1)
        {
            return read_size;
        }
 
        LOG("-> %s", header_str);

        char *next_tok = header_str;
        
        char *curr_tok = next_char(&next_tok, " \t\r\n");
 
        if(strcmp(curr_tok, "GET") == 0 && curr_tok != NULL)
        {
            curr_tok = next_char(&next_tok, " \t\r\n");
            path[0] = '.';

            strcpy(&path[1], curr_tok);

            LOG("URI: %s\n", curr_tok);
        }
        
        else if (curr_tok == NULL)
        {
            break;
        }
    }
 
    LOG("File path: %s\n", path);
 
    struct stat stat_buf;

    int ret = stat(path, &stat_buf);

    if(ret == -1)
    {
        perror("stat");
        file_not_found(client_fd);
        return 0;
    }
 
    if(S_ISDIR(stat_buf.st_mode) == true)
    {
        strcat(path, "/index.html");
        ret = stat(path, &stat_buf);
        
        if (ret == -1)
        {
            perror("stat");

            file_not_found(client_fd);

            return 0;
        }
    }
 
    char message[MAX_STR_LEN] = {0};
    char date[128] = {0};

    generate_timestamp(date, 128);
    
    sprintf(message,
        "HTTP/1.1 200 OK\r\n"
        "Date: %s\r\n"
        "Content-Length: %zu\r\n"
        "\r\n",
        date, stat_buf.st_size);
   
    LOG("Sending response:\n%s", message);

    write(client_fd, message, strlen(message));
   
    int file_fd = open(path, O_RDONLY);
    
    sendfile(client_fd, file_fd, 0, stat_buf.st_size);

    return 0;
}

int main(int argc, char *argv[]){

    if (argc != 3) {
        printf("Usage: %s port dir\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    char *dir = argv[2];

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    if(socket_fd == -1)
    {
        perror("socket");

        return 1;
    }
 
    struct sockaddr_in addr = {0};

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if(bind(socket_fd, (struct sockaddr *) &addr, sizeof(addr)) == -1)
    {
        perror("bind");
        return 1;
    }
 
    if(listen(socket_fd, 10) == -1)
    {
        perror("listen");
        return 1;
    }
 
    LOG("Listening on port %d\n", port);
 
    LOG("changing directory to %s\n", dir);

    int ret = chdir(dir);

    if(ret == -1)
    {
        perror("chdir");
        return 1;
    }
 
    while(true)
    {
        struct sockaddr_in client_addr = {0};
        socklen_t slen = sizeof(client_addr);
 
        int client_fd = accept(
            socket_fd,
            (struct sockaddr *) &client_addr,
            &slen);
 
        if(client_fd == -1)
        {
            perror("accept");
            return 1;
        }
 
        pid_t pid = fork();
 
        if(pid == 0)
        {
            close(socket_fd);

            char remote_host[INET_ADDRSTRLEN];

            inet_ntop(
                client_addr.sin_family,
                (void *) &((&client_addr)->sin_addr),
                remote_host,
                sizeof(remote_host));

            LOG("Accepted connection from %s:%d\n", remote_host, client_addr.sin_port);
 
            while(true)
            {
                int ret = handle_request(client_fd);

                if(ret == -1 || ret == 0)
                {
                    break;
                }
            }
        }
        
        else if (pid < 0)
        {
            perror("fork");
            exit(1);
        }
        
        else
        {
            close(client_fd);
        }
    }
    
    return 0; 
}
