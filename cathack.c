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
char opt_clear = 0;

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

static void usage(FILE *fp, const char* arg0)
{
    fprintf(fp,
        "Usage: " CATHACK_NAME " [options] file [file...]\n"
        "Options:\n"
        " -c    Clear the screen before beginning\n"
        " -f    Print this many characters per keystroke\n"
        " -l    Keep going until interrupted\n"
        " -h    Print this message and exit\n"
    );
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

/* Clear the screen */
static void clear()
{
    printf("\e[1;1H\e[2J");
}

int main(int argc, char **argv)
{
    char buf[CATHACK_BUFSIZE];
    char *endptr;
    int c;
    FILE *fp;
    int current_file = 0;
    size_t nread;

    while((c = getopt(argc, argv, "cf:l")) != -1) {
        long l;
        switch(c) {
        case 'c':
            opt_clear = 1;
            break;
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
        case 'h':
            usage(stdout, argv[0]);
            exit(EXIT_SUCCESS);
        default:
            usage(stderr, argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    argc -= optind;
    argv += optind;

    setup();
    atexit(teardown);

    if(argc < 1) {
        usage(stderr, argv[0]);
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

    if(opt_clear) {
        clear();
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
            fp = NULL;
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
    if(fp) {
        fclose(fp);
        fp = NULL;
    }
    exit(EXIT_SUCCESS);
}
