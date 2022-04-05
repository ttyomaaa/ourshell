#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
/*
  Function Declarations for builtin shell commands:
 */
int shell_cd(char **args);
int shell_ls(char **args);
int shell_help(char **args);
int shell_exit(char **args);
int shell_cat(char **args);
int shell_mkdir(char **args);
int shell_rmdir(char **args);
int shell_rm(char **args);
//int lsh_func(char **args);
/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
    "scd",
    "shelp",
    "sexit",
    "sls",
    "scat",
    "srmdir",
    "srm",
    "smkdir"
};

char *builtin_keys[] = {
    "-a",
    "-b",
};


int (*builtin_func[]) (char **) = {
    &shell_cd,
    &shell_help,
    &shell_exit,
    &shell_ls,
    &shell_cat,
    &shell_rmdir,
    &shell_rm,
    &shell_mkdir
};

int lsh_num_builtins() {
    return sizeof (builtin_str) / sizeof (char *);
}

int lsh_num_builtins_keys() {
    return sizeof (builtin_keys) / sizeof (char *);
}

int shell_mkdir(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "lsh: expected argument to \"mkdir\"\n");
    } else {
        if (mkdir(args[1], S_IRWXU) != 0) {
            perror("lsh");
        }
    }
    return 1;
}

int shell_rmdir(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "lsh: expected argument to \"rmdir\"\n");
    }
    else {
        DIR *dptr = opendir(args[1]);
        struct dirent *ds;
        char filepath[64];
        while ((ds = readdir(dptr)) != NULL) {
            sprintf(filepath, "%s/%s", args[1], ds->d_name);
            remove(filepath);
        }
        closedir(dptr);
        if (rmdir(args[1]) != 0) {
            perror("lsh");
        }
    }

    return 1;
}

int shell_rm(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "lsh: expected argument to \"rm\"\n");
    } else {
        if (remove(args[1]) != 0) {
            perror("lsh");
        }
    }
    return 1;
}

int shell_cat(char **args) {
    DIR *dptr;
    if (args[1] == NULL) {
        fprintf(stderr, "lsh: expected argument to \"cat\"\n");
    } else {
        char pn;
        char currpath[64];
        pn = getwd(currpath);
        dptr = opendir(currpath);
        int c;
        FILE *file;
        file = fopen(args[1], "r");
        if (file) {
            while ((c = getc(file)) != EOF)
                putchar(c);
            fclose(file);
            printf("\n");
        } else fprintf(stderr, "not a file\n");
        closedir(dptr);
    }
    return 1;
}

int shell_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "lsh: expected argument to \"cd\"\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("lsh");
        }
    }
    return 1;
}

int shell_ls(char **args) {
    DIR *dptr;
    struct dirent *ds;
    int i;

    if (args[1] == NULL) {
        char pn;
        char currpath[64];
        pn = getwd(currpath);
        dptr = opendir(currpath);
        while ((ds = readdir(dptr)) != 0)
            printf("%s\n", ds->d_name);
        closedir(dptr);
    } else {
        for (i = 0; i < lsh_num_builtins_keys(); i++) {
            if (strcmp(args[1], builtin_keys[i]) == 0) {
                break;
            }
        }
        switch (i) {
            case 0:
                dptr = opendir(args[2]);
                if (!dptr) {
                    fprintf(stderr, "not a folder\n");
                } else {
                    while ((ds = readdir(dptr)) != 0)
                        printf("%c\n", ds->d_type);
                    closedir(dptr);
                }
                break;
            case 1:
                dptr = opendir(args[2]);
                if (!dptr) {
                    fprintf(stderr, "not a folder\n");
                } else {
                    while ((ds = readdir(dptr)) != 0)
                        printf("%d\n", ds->d_ino);
                    closedir(dptr);
                }
                break;
            default:
            {
                dptr = opendir(args[1]);
                if (!dptr) {
                    fprintf(stderr, "not a folder\n");
                } else {
                    while ((ds = readdir(dptr)) != 0)
                        printf("%s\n", ds->d_name);
                    closedir(dptr);
                }
            }
        }

    }
    return 1;

}

int shell_help(char **args) {
    int i;
    printf("Stephen Brennan's LSH\n");
    printf("Type program names and arguments, and hit enter.\n");
    printf("The following are built in:\n");

    for (i = 0; i < lsh_num_builtins(); i++) {
        printf("  %s\n", builtin_str[i]);
    }

    printf("Use the man command for information on other programs.\n");
    return 1;
}

int shell_exit(char **args) {
    return 0;
}

int shell_launch(char **args) {
    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0) {
        if (execvp(args[0], args) == -1) {
            perror("lsh");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("lsh");
    } else {
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

int shell_execute(char **args) {
    int i;

    if (args[0] == NULL) {
        return 1;
    }

    for (i = 0; i < lsh_num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    return shell_launch(args);
}

#define LSH_RL_BUFSIZE 1024

char *read_line(void) {
    int bufsize = LSH_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof (char) * bufsize);
    int c;

    if (!buffer) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Read a character
        c = getchar();

        // If we hit EOF, replace it with a null character and return.
        if (c == EOF || c == '\n') {
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position] = c;
        }
        position++;

        // If we have exceeded the buffer, reallocate.
        if (position >= bufsize) {
            bufsize += LSH_RL_BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer) {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

char **split_line(char *line) {
    int bufsize = LSH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof (char*));
    char *token;

    if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, LSH_TOK_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += LSH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof (char*));
            if (!tokens) {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, LSH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

void shell_loop(void) {
    char *line;
    char **args;
    int status;

    do {
        printf("myshell>> ");
        line = read_line();
        args = split_line(line);
        status = shell_execute(args);

        free(line);
        free(args);
    } while (status);
}

int main(int argc, char **argv) {

    shell_loop();


    return EXIT_SUCCESS;
}

