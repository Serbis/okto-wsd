#include "include/gpio.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>

void __GPIO_export(int pin) {
    char *pins = (char*) malloc(4);
    sprintf(pins, "%d", pin);
    int export = open("/sys/class/gpio/export", O_RDWR);
    write(export, pins, strlen(pins) + 1);
    close(export);
    free(pins);
}

void __GPIO_direction(int pin, const char *dir) {
    char *direction = (char*) malloc(150);
    sprintf(direction, "/sys/class/gpio/gpio%d/direction", pin);
    int dfd = -1;
    while (dfd == -1)
        dfd = open(direction, O_RDWR);
    free(direction);
    write(dfd, dir, strlen(dir) + 1);
    close(dfd);
}

int __GPIO_get_value_fd(int pin) {
    char *value = (char*) malloc(150);
    sprintf(value, "/sys/class/gpio/gpio%d/value", pin);
    int vfd = -1;
    while(vfd == -1)
        vfd = open(value, O_RDWR);
    free(value);
    
    return vfd;
}

int GPIO_get_out(int pin) {
    __GPIO_export(pin);
    __GPIO_direction(pin, GPIO_MODE_OUT);

    return __GPIO_get_value_fd(pin);
}

int GPIO_get_in(int pin) {
    __GPIO_export(pin);
    __GPIO_direction(pin, GPIO_MODE_IN);

    return __GPIO_get_value_fd(pin);
}

int GPIO_read(int pin) {
    char v = 10;
    lseek(pin, 0, SEEK_SET);
    int redw = read(pin, &v, 1);
    printf("redw=%d\n", redw);    
    printf("xd=%d\n", v);

    if (v == '0')
        return 0;

    return 1;
}

/*int GPIO_get_interrupt(int pin, const char *edge) {
    __GPIO_export(pin);
    __GPIO_direction(pin, GPIO_MODE_IN);
    
    char *edgem = (char*) malloc(150);
    sprintf(edgem, "/sys/class/gpio/gpio%d/edge", pin);
    int dfd = -1;
    while (dfd == -1)
        dfd = open(edgem, O_RDWR);
    free(edgem);
    write(dfd, edge, strlen(edge) + 1);
    close(fdf);

    return __GPIO_get_value_fd(pin);
}

int GPIO_write()*/

int GPIO_write(int pin, const char *value) {
    write(pin, value, strlen(value) + 1);
}
