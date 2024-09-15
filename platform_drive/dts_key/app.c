#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define DEVICE_FILE "/dev/gpio_key_device"

int main() {
    int fd;
    char buf[256];
    ssize_t bytes_read;

    fd = open(DEVICE_FILE, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    while (1) {
        bytes_read = read(fd, buf, sizeof(buf) - 1);
        if (bytes_read < 0) {
            perror("read");
            close(fd);
            return 1;
        }
        buf[bytes_read] = '\0';
        printf("%s", buf);
        sleep(1);
    }

    close(fd);
    return 0;
}

