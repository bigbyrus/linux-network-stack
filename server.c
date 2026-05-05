#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

// The Server will produce threads to handle ONE request from a client

void *handle_client(void *arg){
    int client_fd = *(int *)arg;
    free(arg);

    char buffer[1024];

    // WAIT FOR REQUEST FROM CLIENT
    int n = recv(client_fd, buffer, sizeof(buffer), 0);

    // PARSE DATA, RESPOND TO CLIENT, RELEASE ALL RESOURCES
    if(n > 0){
        printf("Received:\n%.*s\n", n, buffer);

        if(n >= 4 && strncmp(buffer, "GET ", 4) == 0){
            const char *response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 13\r\n"
                "\r\n"
                "Hello, world!";

            send(client_fd, response, strlen(response), 0);
        } else if(n >= 4 && strncmp(buffer, "POST ", 5)){
            // not sure 

        } else {
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