#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "file_watcher.h"
#include "module.h"
#include "notify.h"

static long int take_num(const char* filename) {
    int fd;
    ssize_t len;
    char buf[256];

    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 0;
    }

    len = read(fd, buf, 255);
    close(fd);
    if (len == -1) {
        perror("read");
        return 0;
    }

    buf[len] = '\0';
    return atol(buf);
}

void notify_brightness(long actual, long max) {
    char buf[256];
    sprintf(buf, "change backlight to %ld, max %ld\n", actual, max);
    show_notice(buf);
}

const char* ACTUAL_BRIGHTNESS = "/sys/class/backlight/acpi_video0/actual_brightness";
const char* MAX_BRIGHTNESS = "/sys/class/backlight/acpi_video0/max_brightness";

int on_brightness_modified(void* private) {
    long int actual = take_num(ACTUAL_BRIGHTNESS);
    long int max = take_num(MAX_BRIGHTNESS);

    notify_brightness(actual, max);
    return 0;
}

static void init_backlight() { add_file_watcher(ACTUAL_BRIGHTNESS, on_brightness_modified, NULL); }

static void destroy_backlight() {}

DEFINE_CONFIG_MODULE(backlight, init_backlight, destroy_backlight);
