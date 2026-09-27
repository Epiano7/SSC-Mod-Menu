#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <fcntl.h>

static void diagnostic_output(void) {
    char path[PATH_MAX];
    const char *state = getenv("XDG_STATE_HOME"), *home = getenv("HOME");
    int n;
    if (state && *state == '/') n = snprintf(path, sizeof(path), "%s/ssc-mod-menu/logs", state);
    else if (home) n = snprintf(path, sizeof(path), "%s/.local/state/ssc-mod-menu/logs", home);
    else return;
    if (n < 0 || n >= (int)sizeof(path)-24) return;
    for (char *p=path+1; *p; ++p) if (*p=='/') {*p=0; mkdir(path,0700); *p='/';}
    mkdir(path,0700);
    strcat(path,"/setup-startup.log");
    int fd=open(path,O_WRONLY|O_CREAT|O_APPEND|O_NOFOLLOW,0600);
    if(fd>=0) {
        struct stat st;
        if(fstat(fd,&st)==0 && st.st_size>1048576) ftruncate(fd,0);
        dup2(fd,STDERR_FILENO); close(fd);
    }
}

int main(int argc, char **argv) {
    diagnostic_output();
    char root[PATH_MAX], python[PATH_MAX], script[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", root, sizeof(root)-1);
    if (n <= 0 || n >= (ssize_t)sizeof(root)-1) return 1;
    root[n] = 0;
    char *slash = strrchr(root, '/');
    if (!slash) return 1;
    *slash = 0;
    if (snprintf(python, sizeof(python), "%s/python/bin/python3", root) >= (int)sizeof(python) ||
        snprintf(script, sizeof(script), "%s/ssc_installer.py", root) >= (int)sizeof(script)) return 1;
    char **args = calloc((size_t)argc + 2, sizeof(char*));
    if (!args) return 1;
    args[0] = python;
    args[1] = script;
    for (int i=1; i<argc; ++i) args[i+1] = argv[i];
    execv(python, args);
    perror("SSC Mod Menu: cannot start bundled Python; extract the entire archive");
    free(args);
    return 1;
}
