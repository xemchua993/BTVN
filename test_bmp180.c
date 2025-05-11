#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define DEVICE_FILE "/dev/bmp180"

int main() {
    int fd;
    char buffer[128];
    ssize_t bytes_read;

    // Mở thiết bị BMP180
    fd = open(DEVICE_FILE, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open device");
        return 1;
    }

    // Đọc dữ liệu từ thiết bị
    bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes_read == -1) {
        perror("Failed to read from device");
        close(fd);
        return 1;
    }

    // Kết thúc chuỗi đọc được và in ra
    buffer[bytes_read] = '\0';
    printf("Data from BMP180:\n%s\n", buffer);

    // Đóng thiết bị
    close(fd);

    return 0;
}
