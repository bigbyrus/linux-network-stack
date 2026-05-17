#include <pthread.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// Unix specific libraries (no Windows)
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

// global
#define SERIAL_PORT "/dev/ttyUSB0"
#define BAUDRATE B115200
pthread_mutex_t camera_lock = PTHREAD_MUTEX_INITIALIZER;
const char *cmd = "TRIGGER\n";
int serial_fd;


// The Server will produce threads to handle ONE request from a client at a time

// Every individual HTTP request we receive from a client gets its own thread
void *handle_client(int *arg){
    int client_fd = *arg;
    free(arg);

    int j;
    char buffer[1024];

    // WAIT FOR REQUEST FROM CLIENT
    j = recv(client_fd, buffer, sizeof(buffer), 0);

    // PARSE DATA, RESPOND TO CLIENT, RELEASE ALL RESOURCES
    if(j > 0){
        printf("Received:\n%.*s\n", j, buffer);

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
        
        // send HTML file to browser, BROWSER WILL REQUEST "image.jpg" AFTER RECEIVING HTML FILE
        } else if(strcmp(method, "GET") == 0 && strcmp(path, "/pic") == 0){
            int fd = open("pic.html", O_RDONLY);
            int bytes = read(fd, file_buffer, sizeof(file_buffer));

            // first, send header to client
            sprintf(header,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: %d\r\n"
                "\r\n",
                bytes);
            send(client_fd, header, strlen(header), 0);

            // then, send actual html file to client
            send(client_fd, file_buffer, bytes, 0);

        // send raw image data from CAMERA to BROWSER
        } else if(strcmp(path, "/image.jpg") == 0){
            pthread_mutex_lock(&camera_lock);
            
            // WRITE "TRIGGER\n" TO THE FILE
            uint32_t length;
            ssize_t written = write(serial_fd, cmd, strlen(cmd));
            tcdrain(serial_fd);

            // just in case
            if(written < 0){
                perror("write");   
                pthread_mutex_unlock(&camera_lock);
                return 0;
            }

            // obtain the length image data 
            read_exact(serial_fd, &length, sizeof(length));
            printf("Image length: %u bytes\n", length);

            // just in case 'length' is too large
            if(length == 0 || length > 10 * 1024 * 1024){
                printf("Invalid length: %u\n", length);
                printf("exiting...");
                pthread_mutex_unlock(&camera_lock);
                return 0;
            }

            // allocate 4KB on heap to STREAM image data 
            unsigned char *buf = malloc(4096);

            // send header to client, use LENGTH so browser can display image
            sprintf(header,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: image/jpeg\r\n"
                "Content-Length: %u\r\n"
                "\r\n",
                length);
            send(client_fd, header, strlen(header), 0);

            // STREAM image data to client
            ssize_t n;
            uint32_t remaining = length;

            // obtain 4KB of JPEG data at a time, send to browser
            while(remaining > 0){

                // read 4KB from serial port (if there is enough data)
                int chunk = (remaining > 4096) ? 4096 : remaining;
                n = read_exact(serial_fd, buf, chunk);

                if(n <= 0) {
                    printf("error reading jpeg data...");
                    pthread_mutex_unlock(&camera_lock);
                    break;
                }

                // send 4KB to browser
                send(client_fd, buf, n, 0);

                // update position
                remaining -= n;
            }
            pthread_mutex_unlock(&camera_lock);
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

/* ensure that all serial data is read */
ssize_t read_exact(int fd, char *buf, size_t count) {
    size_t total = 0;

    // if read operation gets interrupted, use "total" as a placeholder
    while(total < count){

        // continue at position denoted by "total"
        ssize_t n = read(fd, buf + total, count - total);
        if(n <= 0)
            return -1;
        
        // keep track of position
        total += n;
    }

    // return how many bytes were read
    return total;
}


int main() {
    struct sockaddr_in addr;
    
    // define IP address and PORT number
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    // socket for our application
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // update the port using its file descriptor
    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    
    listen(server_fd, 10);
    printf("listening... on port 8080\n");


    // need to close this...
    serial_fd = open(SERIAL_PORT, O_RDWR | O_NOCTTY);
    if(serial_fd < 0){
        perror("open");
        return 1;
    }

    // serial port config struct
    struct termios tty;

    // obtain the current port configurations
    if(tcgetattr(serial_fd, &tty) != 0) {
        perror("tcgetattr");
        return 1;
    }

    // DEFINE SERIAL PORT CONFIGURATIONS
    cfsetispeed(&tty, BAUDRATE);
    cfsetospeed(&tty, BAUDRATE);

    // Configure 8 data bits, no parity, 1 stop bit
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
    if(tcsetattr(serial_fd, TCSANOW, &tty) != 0){
        perror("tcsetattr");
        return 1;
    }
    printf("Serial connected\n");

    // BEGIN CREATING THREADS FOR EACH TCP CONNECTION ESTABLISHED
    while(1){

        // dynamically allocate client file descriptors
        int *client_fd = malloc(sizeof(int));
        *client_fd = accept(server_fd, NULL, NULL);

        // new TCP connection with a client, HANDLE request in thread
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, client_fd);

        // allow thread to exit on its own
        pthread_detach(tid);
    }

    close(serial_fd);
    close(server_fd);
    return 0;
}