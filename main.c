#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <termios.h>
#include <sys/stat.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<net/if.h>
#include<netinet/in.h>
#include<sys/ioctl.h>

int shell_cd(char **args);
int shell_ls(char **args);
int shell_help(char **args);
int shell_exit(char **args);
int shell_cat(char **args);
int shell_mkdir(char **args);
int shell_rmdir(char **args);
int shell_rm(char **args);
int shell_menu(void);

char *builtin_str[] = {
    "scd",
    "shelp",
    "sexit",
    "sls",
    "scat",
    "srmdir",
    "srm",
    "smkdir",
    "smenu"
};

char *builtin_keys[] = {
    "-a",
    "-b",
};

char history[64][64]; 

int (*builtin_func[]) (char **) = {
    &shell_cd,
    &shell_help,
    &shell_exit,
    &shell_ls,
    &shell_cat,
    &shell_rmdir,
    &shell_rm,
    &shell_mkdir,
    &shell_menu,
};

int lsh_num_builtins() {
    return sizeof (builtin_str) / sizeof (char *);
}

int lsh_num_builtins_keys() {
    return sizeof (builtin_keys) / sizeof (char *);
}
char *menu[] = {
 "i - ip",
 "q - quit",
 NULL,

};


int getchoice(char *greet, char *choices[]) {

    int chosen = 0;
    int selected;
    char **option;
    do {
        fprintf(stdout, "Choice: %s\n", greet);
        option = choices;
        while (*option) {
            printf("%s\n", *option);
            option++;
        }
        do {
            selected = fgetc(stdin);
        } while (selected == '\n' || selected == '\r');
        option = choices;
        while (*option) {
            if (selected == *option[0]) {
                chosen = 1;
                break;
            }
            option++;
        }
        if (!chosen) {
            printf("Incorrect choice, select again\n");
        }
    } while (!chosen);

    return selected;

}
int shell_ip(void){
  int i,n,s;
  struct ifreq ifr[100];
  struct ifconf ifconf;
  struct sockaddr_in *sin;
 
  s=socket(AF_INET, SOCK_DGRAM, 0);
  ifconf.ifc_len=sizeof(ifr);
  ifconf.ifc_req=ifr;
  ioctl(s, SIOCGIFCONF, &ifconf);
  close(s);
 
  n=ifconf.ifc_len/sizeof(struct ifreq);
  for(i=0;i<n;i++) {
    sin=(struct sockaddr_in *) (&ifr[i].ifr_addr);
    printf("%s -> %s\n",ifr[i].ifr_name,inet_ntoa(sin->sin_addr));
  }
}

int shell_menu(void) {
    int choice = 0;
    struct termios initial_settings, new_settings;

    tcgetattr(fileno(stdin), &initial_settings);
    new_settings = initial_settings;
    new_settings.c_lflag &= ~ICANON;
    new_settings.c_lflag &= ~ECHO;
    new_settings.c_cc[VMIN] = 1;
    new_settings.c_cc[VTIME] = 0;
    new_settings.c_lflag &= ~ISIG;
    if (tcsetattr(fileno(stdin), TCSANOW, &new_settings) != 0) {
        fprintf(stderr, "could not set attributes\n");
    }
    do {
        choice = getchoice("Please select an action", menu);
        printf("You have chosen: %c\n", choice);
        if(choice == 'i') shell_ip();
    } while (choice != 'q');

    tcsetattr(fileno(stdin), TCSANOW, &initial_settings);
    return 1;
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
                if (args[2] == NULL) {
                    char pn;
                    char currpath[64];
                    pn = getwd(currpath);
                    dptr = opendir(currpath);
                    while ((ds = readdir(dptr)) != 0)
                        printf("%c\n", (char) ds->d_type);
                    closedir(dptr);                  
                } else {
                    dptr = opendir(args[2]);
                    if (!dptr) {
                        fprintf(stderr, "not a folder\n");
                    } else {
                        while ((ds = readdir(dptr)) != 0)
                            printf("%c\n", (char) ds->d_type);
                        closedir(dptr);
                    }
                }
                break;
            case 1:
                if (args[2] == NULL) {
                    char pn;
                    char currpath[64];
                    pn = getwd(currpath);
                    dptr = opendir(currpath);
                    while ((ds = readdir(dptr)) != 0)
                        printf("%d\n", (int) ds->d_ino);
                    closedir(dptr);
                } else {
                    dptr = opendir(args[2]);
                    if (!dptr) {
                        fprintf(stderr, "not a folder\n");
                    } else {
                        while ((ds = readdir(dptr)) != 0)
                            printf("%d\n", (int) ds->d_ino);
                        closedir(dptr);
                    }
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
    printf("The following are built in:\n");

    for (i = 0; i < lsh_num_builtins(); i++) {
        printf("  %s\n", builtin_str[i]);
    }
    char buf[64];
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
        c = getchar();
        if (c == EOF || c == '\n') {
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position] = c;
        }
        position++;

        if (position >= bufsize) {
            bufsize += LSH_RL_BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer) {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
        char buffer[10];
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
    int i=0;
    int state = 1;

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

