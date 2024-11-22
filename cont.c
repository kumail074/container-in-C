#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sched.h>
#include <grp.h>
#include <pwd.h>
#include <seccomp.h>
#include <linux/wait.h>
#include <linux/limits.h>
#include <linux/capability.h>
#include <sys/capability.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <sys/wait.h>

typedef struct {
    int argc;
    uid_t uid;
    int fd;
    char *hostname;
    char **argv;
    char *mount_dir;
} child_config;

int choose_hostname(char *buff, size_t len) {
    static const char *suits[] = {"swords", "wands", "pentacles", "cups"};
    static const char *minor[] = {
        "ace", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "page", "knight", "queen", "king"
    };
    static const char *major[] = {
        "fool", "magician", "high-priestess", "empress", "emperor", "hierophant", "lovers", "chariot", "strength", "hermit", "wheel", "justice", "hanged-man", "death", "temperance", "devil", "tower", "star", "moon", "sun", "judgement", "world"
    };
    struct timespec now = {0};
    clock_gettime(CLOCK_MONOTONIC, &now);
    size_t ix = now.tv_nsec % 78;
    if(ix < sizeof(major) / sizeof(*major)) {
        snprintf(buff, len, "%05lx-%s", now.tv_sec, major[ix]);
    } else {
        ix -= sizeof(major) / sizeof(*major);
        snprintf(buff, len, "%05lxc-%s-of-%s",
                now.tv_sec, minor[ix % (sizeof(minor) / sizeof(*minor))],
                suits[ix / (sizeof(minor) / sizeof(*minor))]);
    }
    return 0;
}


int main(int argc, char *argv[]) {
    
    child_config config = {0};
    int err = 0;
    int option = 0;
    int sockets[2] = {0};
    pid_t child_pid = 0;
    int last_optind = 0;

    while((option = getopt(argc, argv, "c:m:u:"))) {
        switch(option) {
        case 'c':
            config.argc = argc - last_optind - 1;
            config.argv = &argv[argc - config.argc];
            goto finish_options;
        case 'm':
            config.mount_dir = optarg;
            break;
        case 'u':
            if(sscanf(optarg, "%d", &config.uid) != 1) {
                fprintf(stderr, "badly-formatted uid: %s\n", optarg);
                goto usage;
            }
            break;
        default:
            goto usage;
        }
        last_optind = optind;
    }

finish_options:
    if(!config.argc) goto usage;
    if(!config.mount_dir) goto usage;

//<<check-linux-version>>
    char hostname[256] = {0};
    if(choose_hostname(hostname, sizeof(hostname)))
            goto error;
    config.hostname = hostname;

//<<namespaces>>
    
    goto cleanup;

usage:
    fprintf(stderr, "Usage: %s -u -l -m . -c /bin/sh ~\n", argv[0]);

error:
    err = 1;

cleanup:
    if(sockets[0]) close(sockets[0]);
    if(sockets[1]) close(sockets[1]);

    return err;
}
