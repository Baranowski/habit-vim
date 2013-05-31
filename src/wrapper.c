/* (c) 2013 Wojciech Baranowski
 *
 * Based on the 'script' program from the util-linux package.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pty.h>

char vim_path[] = "/usr/bin/vim";
char dump_path[] = "dump.log";

int die;
int childstatus;
int master;
int slave;

pid_t	child;

struct	winsize win;
struct	termios tt;
struct	termios old_tt;

int resized;

FILE * dumpfile;

void warn(const char* msg) {
    fprintf(stderr, "%s\n", msg);
}

void finish(int dummy) {
    int status;
    pid_t pid;

    while ((pid = wait3(&status, WNOHANG, 0)) > 0) {
        if (pid == child) {
            childstatus = status;
            die = 1;
        }
    }
}

void resize(int dummy) {
    resized = 1;
    /* transmit window change information to the child */
    ioctl(STDIN_FILENO, TIOCGWINSZ, (char *)&win);
    ioctl(slave, TIOCSWINSZ, (char *)&win);
}

void cleanup() {
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tt);
    fclose(dumpfile);
}

void fail() {
    kill(0, SIGTERM);
    cleanup();
}

void run_vim(char **argv) {
    dup2(slave, STDIN_FILENO);
    execv(vim_path, argv);
    warn("failed to execute vim");
    fail();
}

void log_keys() {
    ssize_t read_res;
    char new_char;
    struct timeval tv;

    while (die == 0) {
        read_res = read(STDIN_FILENO, &new_char, 1);
        if (read_res > 0) {
            gettimeofday(&tv, NULL);
            if (write(master, &new_char, 1) < 0) {
                warn ("write failed");
                fail();
            }
            fprintf(dumpfile, "%c %ld.%ld\n", new_char,
                    (long)tv.tv_sec, (long)tv.tv_usec);
        } else if (read_res < 0 && errno == EINTR && resized) {
            resized = 0;
        } else {
            break;
        }
    }
}

int main(int argc, char **argv) {
    sigset_t block_mask, unblock_mask;
    struct sigaction sa;

    die = 0;
    resized = 0;
    dumpfile = fopen(dump_path, "a");

    {
        tcgetattr(STDIN_FILENO, &old_tt);
        tcgetattr(STDIN_FILENO, &tt);
        tt.c_lflag &= ~ICANON;
        tt.c_lflag &= ~ECHO;
        ioctl(STDIN_FILENO, TIOCGWINSZ, (char *)&win);
        if (openpty(&master, &slave, NULL, &tt, &win) < 0) {
                warn("openpty failed");
                fail();
        }
        tcsetattr(STDIN_FILENO, TCSANOW, &tt);
        grantpt(master);
        unlockpt(master);
    }
    /* setup SIGCHLD handler */
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = finish;
    sigaction(SIGCHLD, &sa, NULL);

    /* init mask for SIGCHLD */
    sigprocmask(SIG_SETMASK, NULL, &block_mask);
    sigaddset(&block_mask, SIGCHLD);

    sigprocmask(SIG_SETMASK, &block_mask, &unblock_mask);
    child = fork();
    sigprocmask(SIG_SETMASK, &unblock_mask, NULL);

    if (child < 0) {
        warn("fork failed");
        fail();
    }
    if (child == 0) {

        run_vim(argv);
    } else {
        sa.sa_handler = resize;
        sigaction(SIGWINCH, &sa, NULL);
    }

    log_keys();

    cleanup();
    return EXIT_SUCCESS;
}
