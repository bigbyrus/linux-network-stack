#include <pthread.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// Unix specific libraries (program will not work on Windows)
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

#define SERIAL_PORT "/dev/ttyUSB0"
#define BAUDRATE B115200

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

        char method[8];
        char path[256];
        char version[16];
        char file_buffer[4096];
        char header[256];

        // extract data from HTTP request
        sscanf(buffer, "%7s %255s %15s", method, path, version);

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
        
        // show a picture from the camera module
        } else if(strcmp(method, "GET") == 0 && strcmp(path, "/pic") == 0){
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
    // need to close serial port at some point...
    int fd = open(SERIAL_PORT, O_RDWR | O_NOCTTY);
    if(fd < 0){
        perror("open");
        return 1;
    }

    // serial port config struct
    struct termios tty;

    // obtain the current port configurations
    if(tcgetattr(fd, &tty) != 0) {
        perror("tcgetattr");
        return 1;
    }

    // DEFINE SERIAL PORT CONFIGURATIONS
    cfsetispeed(&tty, BAUDRATE);
    cfsetospeed(&tty, BAUDRATE);

    // Configure 8N1 (8 data bits, no parity, 1 stop bit)
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag |= CREAD | CLOCAL;

    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_iflag = 0;
    tty.c_cc[VMIN]  = 1;
    tty.c_cc[VTIME] = 0;


    // UPDATE SERIAL PORT CONFIGURATIONS
    if(tcsetattr(fd, TCSANOW, &tty) != 0){
        perror("tcsetattr");
        return 1;
    }
    printf("Serial connected\n");



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