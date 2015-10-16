#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define CATHACK_BUFSIZE 256
#define CATHACK_NAME "cathack"

struct termios original_termios;

size_t opt_factor = 4;

static void setup()
{
    struct termios new_termios;
    tcgetattr(STDIN_FILENO, &original_termios);
    new_termios = original_termios;

    /* Do not echo stdin and flush every character */
    new_termios.c_lflag &= ~ECHO;
    new_termios.c_lflag &= ~ICANON;

    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

static void teardown()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
}

static void usage(FILE *fp)
{
    fprintf(fp, CATHACK_NAME " [options] file1 [file2...]\n");
}

/* Prints message to stderr with prefix */
static void error(const char *prefix, const char *msg)
{
    fprintf(stderr, CATHACK_NAME ": ");
    if(prefix) {
        fprintf(stderr, "%s: ", prefix);
    }
    fprintf(stderr, "%s\n", msg);
}

/* Prints message and exits with failure */
static void fail(const char *prefix, const char *msg)
{
    error(prefix, msg);
    exit(EXIT_FAILURE);
}

/* Prints strerror and exits with failure */
static void fatal(const char *prefix)
{
    fail(prefix, strerror(errno));
}

int main(int argc, char **argv)
{
    char buf[CATHACK_BUFSIZE];
    int c;
    FILE *fp;
    int current_file = 1;
    size_t nread;

    setup();
    atexit(teardown);

    if(argc <= 1) {
        usage(stderr);
        exit(EXIT_FAILURE);
    }

    if(opt_factor > CATHACK_BUFSIZE - 1) {
        fprintf(stderr, "Type factor cannot be >%d\n", CATHACK_BUFSIZE - 1);
        exit(EXIT_FAILURE);
    }

    fp = fopen(argv[current_file], "r");
    if(!fp) {
        fatal(argv[current_file]);
    }

    while(1) {
        c = getchar();
        if(c == EOF || c == 4) {
            break;
        }

        nread = fread(buf, sizeof(*buf), opt_factor, fp);
        if(nread == 0) {
            ungetc(c, stdin);
            fclose(fp);
            current_file++;
            if(current_file == argc) {
                break;
            }
            fp = fopen(argv[current_file], "r");
            if(!fp) {
                fatal(argv[current_file]);
            }
            continue;
        }
        fwrite(buf, sizeof(*buf), nread, stdout);
    }
    exit(EXIT_SUCCESS);
}
