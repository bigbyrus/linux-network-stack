#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// Unix specific libraries (program will not work on Windows)
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

#define SERIAL_PORT "/dev/ttyCOM8"
#define BAUDRATE B115200

/* ensure that all serial data is read */
ssize_t read_exact(int fd, void *buf, size_t count) {
    size_t total = 0;
    while(total < count){
        ssize_t n = read(fd, (char*)buf + total, count - total);
        if(n <= 0)
            return -1;
        total += n;
    }
    return total;
}

int main() {
    // obtain FILE DESCRIPTOR representing the "open" Serial Device
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






    // SEPARATE THIS INTO ITS OWN THREAD

        // WRITE "TRIGGER\n" TO THE FILE 
        uint32_t length;
        const char *cmd = "TRIGGER\n";
        ssize_t written = write(fd, cmd, strlen(cmd));
        tcdrain(fd);

        if(written < 0)
            perror("write");

        // READ THE IMAGE DATA RECEIVED
        if(read_exact(fd, &length, sizeof(length)) != sizeof(length))
            perror("read length");
        printf("Image length: %u bytes\n", length);

        if(length == 0 || length > 10 * 1024 * 1024){
            printf("Invalid length: %u\n", length);
            printf("exiting...");
            return 0;
        }

        
        // USE LENGTH TO CREATE BUFFER IN HEAP
        unsigned char *buffer = malloc(length);
        if(!buffer)
            perror("malloc");

        // STORE IMAGE DATA IN OUR BUFFER
        if(read_exact(fd, buffer, length) != length){
            perror("read image");
            free(buffer);
        }

        // SAVE IMAGE DATA
        FILE *fp = fopen("image.jpg", "wb");
        if(!fp)
            perror("fopen");
        else{
            fwrite(buffer, 1, length, fp);
            fclose(fp);
            printf("Saved image.jpg\n");
        }

    free(buffer);
    close(fd);
    return 0;
}