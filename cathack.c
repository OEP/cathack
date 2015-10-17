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
char opt_forever = 0;

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
    char *endptr;
    int c;
    FILE *fp;
    int current_file = 0;
    size_t nread;

    while((c = getopt(argc, argv, "f:l")) != -1) {
        long l;
        switch(c) {
        case 'f':
            endptr = NULL;
            l = strtol(optarg, &endptr, 10);
            if(endptr != NULL && *endptr != 0) {
                snprintf(buf, sizeof(buf), "Invalid character: `%c'", *endptr);
                fail(NULL, buf);
            }
            if(l <= 0) {
                fail(NULL, "Type factor must be a positive integer");
            }
            opt_factor = (size_t) l;
            break;
        case 'l':
            opt_forever = 1;
            break;
        default:
            usage(stderr);
            exit(EXIT_FAILURE);
        }
    }
    argc -= optind;
    argv += optind;

    setup();
    atexit(teardown);

    if(argc < 1) {
        usage(stderr);
        exit(EXIT_FAILURE);
    }

    if(opt_factor > CATHACK_BUFSIZE - 1) {
        snprintf(buf, sizeof(buf),
                 "Type factor cannot be >%d", CATHACK_BUFSIZE - 1);
        fail(NULL, buf);
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
            if(current_file == argc && !opt_forever) {
                break;
            }
            else if(current_file == argc) {
                current_file = 0;
            }
            fp = fopen(argv[current_file], "r");
            if(!fp) {
                fatal(argv[current_file]);
            }
            continue;
        }
        fwrite(buf, sizeof(*buf), nread, stdout);
    }
    fclose(fp);
    exit(EXIT_SUCCESS);
}
