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

int die;
int childstatus;
int master;
int slave;

pid_t	child;

struct	winsize win;
struct	termios tt;
struct	termios old_tt;

int resized;

FILE * logfile;

void help(char *progname) {
fprintf(stderr, "Usage:\n"
    "%s <log file> <exec path> [args ...]\n", progname);
fprintf(stderr,
    "\n"
    "This program executes <exec path>, passing any arguments to\n"
    "it, and logs every keystroke with corresponding timestamp to\n"
    "<log file>.\n"
    );
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
    if (logfile != NULL) {
        fclose(logfile);
    }
}

void fail() {
    cleanup();
    kill(0, SIGTERM);
}

void run_child(char **argv) {
    close(master);
    dup2(slave, STDIN_FILENO);
    close(slave);

    execv(argv[0], argv);

    fprintf(stderr, "failed to execute %s", argv[0]);
    fail();
}

/* Pass any possible control sequence without logging.
 * TODO: doesn't work, seems that control sequence appears
 * in stdin shortly after we try to read from it.
 */
void pass_control_seq() {
    char buf[100];
    int stdin_flags;
    ssize_t read_cnt;

    stdin_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, stdin_flags | O_NONBLOCK);
    do {
        read_cnt = read(STDIN_FILENO, &buf, sizeof(buf));
        if (read_cnt > 0) {
            if (write(master, &buf, read_cnt) < 0) {
                fprintf(stderr, "writing control sequence");
                fail();
            }
        }
    } while (read_cnt > 0 && errno != EAGAIN && errno != EWOULDBLOCK);
    fcntl(STDIN_FILENO, F_SETFL, stdin_flags);
}

void log_keys(char *log_path) {
    ssize_t read_res;
    char new_char;
    struct timeval tv;

    close(slave);
    //pass_control_seq();

    logfile = fopen(log_path, "a");
    if (logfile == NULL) {
        fprintf(stderr, "Cannot open %s for writing\n",
                log_path);
        fail();
    }

    while (die == 0) {
        read_res = read(STDIN_FILENO, &new_char, 1);
        if (read_res > 0) {
            gettimeofday(&tv, NULL);
            if (write(master, &new_char, 1) < 0) {
                fprintf(stderr, "write failed");
                fail();
            }
            fprintf(logfile, "%c %ld.%06ld\n", new_char,
                    (long)tv.tv_sec, (long)tv.tv_usec);
        } else if (read_res < 0 && errno == EINTR && resized) {
            resized = 0;
        } else {
            break;
        }
    }
    close(master);
}

void init() {
    die = 0;
    resized = 0;
    logfile = NULL;

    tcgetattr(STDIN_FILENO, &old_tt);
    tcgetattr(STDIN_FILENO, &tt);
    tt.c_lflag &= ~ICANON;
    tt.c_lflag &= ~ECHO;
    ioctl(STDIN_FILENO, TIOCGWINSZ, (char *)&win);
    if (openpty(&master, &slave, NULL, &tt, &win) < 0) {
        fprintf(stderr, "openpty failed");
        fail();
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &tt);
    grantpt(master);
    unlockpt(master);
}

int main(int argc, char **argv) {
    sigset_t block_mask, unblock_mask;
    struct sigaction sa;

    if (argc < 3) {
        help(argv[0]);
        return EXIT_FAILURE;
    }

    init();

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
        fprintf(stderr, "fork failed");
        fail();
    }
    if (child == 0) {
        run_child(argv + 2);
    }
    sa.sa_handler = resize;
    sigaction(SIGWINCH, &sa, NULL);

    log_keys(argv[1]);

    cleanup();
    return EXIT_SUCCESS;
}
