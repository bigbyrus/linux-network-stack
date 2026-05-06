#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/stat.h>

// The Server will produce a new thread for EVERY HTTP request from clients

void *handle_client(void *arg){
    char buffer[1024];

    // add client_fd to the thread's stack
    int client_fd = *(int *)arg;
    
    // release client_fd from the heap
    free(arg);

    // WAIT FOR REQUEST FROM CLIENT
    int n = recv(client_fd, buffer, sizeof(buffer), 0);

    // parse data, respond to client, release all resources
    if(n > 0){
        printf("Received:\n%.*s\n", n, buffer);

        char method[8];
        char path[256];
        char version[16];
        char file_buffer[4096];
        char header[256];

        // extract data from HTTP request
        sscanf(buffer, "%7s %255s %15s", method, path, version);

        // initial response from server
        if(strcmp(method, "GET") == 0 && strcmp(path, "/") == 0){
            int fd = open("index.html", O_RDONLY);
            int bytes = read(fd, file_buffer, sizeof(file_buffer));

            sprintf(header,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: %d\r\n"
                "\r\n",
                bytes);

            send(client_fd, header, strlen(header), 0);
            send(client_fd, file_buffer, bytes, 0);

        // after clicking the "Users" button
        } else if(strcmp(method, "GET") == 0 && strcmp(path, "/users") == 0){

            // ADD users.html to display a dummy image
            int fd = open("users.html", O_RDONLY);
            int bytes = read(fd, file_buffer, sizeof(file_buffer));

            sprintf(header,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: %d\r\n"
                "\r\n",
                bytes);

            send(client_fd, header, strlen(header), 0);
            send(client_fd, file_buffer, bytes, 0);

        // provide image to client
        } else if(strcmp(method, "GET") == 0 && strcmp(path, "/image.jpg") == 0){
            int fd = open("image.jpg", O_RDONLY);

            // get file size
            struct stat st;
            fstat(fd, &st);
            int file_size = st.st_size;

            // tell browser how many bytes to expect
            sprintf(header,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: image/jpeg\r\n"
                "Content-Length: %d\r\n"
                "\r\n",
                file_size);

            send(client_fd, header, strlen(header), 0);

            int n;

            // continuously stream data until entire FILE is sent
            while((n = read(fd, buffer, sizeof(buffer))) > 0) {
                send(client_fd, buffer, n, 0);
            }

            close(fd);
        }
        else {
            const char *response =
                "HTTP/1.1 400 Bad Request\r\n"
                "Content-Length: 11\r\n"
                "\r\n"
                "Bad Request";

            send(client_fd, response, strlen(response), 0);
        }
    }

    close(client_fd);
    return NULL;
}

int main() {
    // ESTABLISH PORT AND IP FOR THIS APPLICATION
    struct sockaddr_in addr;
    int server_fd;

    // file descriptor for our socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // define IP address and PORT number
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    // update the port using its file descriptor
    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    
    listen(server_fd, 10);
    printf("Listening on port 8080...\n");


    // BEGIN CREATING THREADS FOR EACH TCP CONNECTION ESTABLISHED
    while(1){

        // dynamically allocate new Sockets
        int *client_fd = malloc(sizeof(int));

        // thread BLOCKS here, won't use CPU until a TCP connection occurs
        *client_fd = accept(server_fd, NULL, NULL);

        // new TCP connection with a client, HANDLE request in thread
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, client_fd);

        // allow thread to exit on its own (detached)
        pthread_detach(tid);
    }

    close(server_fd);
    return 0;
}