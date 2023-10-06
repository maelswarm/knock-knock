//
//  main.m
//  brutel
//
//  Created by fairy-slipper on 12/26/15.
//  Copyright © 2015 fairy-slipper. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <sched.h>
#include <sys/wait.h>

#define MAXS 1024

void *time_out(void *arg);
void parse_ip();
void parse_version_ip(char *filename, char *addr);
void ctrlc_handler();
void ctrlz_handler();
void ctrlbslash_handler();
void safe_printf(const char *format, ...);

pthread_t timeout_t;

int fo = -1;
int so = -1;
pid_t pid;
int status;

char *nmapoutname = "nmapout.txt";
char ipfilename[100];
char knocklist[100];
char port[11];
char irand[11];
char range[100];

char *token = (char *)NULL;

int main(int argc, const char *argv[])
{

    uid_t euid = geteuid();
    if (0 != euid)
    {
        printf("\nPlease run as root\n");
        exit(0);
    }

    signal(SIGINT, ctrlc_handler);
    signal(SIGTSTP, ctrlz_handler);
    int isdiscover = 0;
    int isknock = 0;
    int isversion = 0;
    int isauto = 0;
    int mark = 0;
    strcpy(ipfilename, "ip.txt");
    strcpy(knocklist, "klist.txt");
    strcpy(port, "23");
    strcpy(irand, "1000");
    memset(range, '\0', 100 * sizeof(char));

    // if (argc == 1) {
    //     isdiscover = 1;
    //     isknock = 1;
    // }
    for (int i = 1; i < argc && argv[i][0] != '>'; i++)
    {
        if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--discover"))
        {
            if (mark)
            {
                break;
            }
            if (argv[i + 1] != NULL)
            {
                strcpy(ipfilename, argv[i + 1]);
                i++;
            }
            else
            {
                printf("\nPlease specify an address list.\n");
                exit(0);
            }
            isdiscover = 1;
            mark = 1;
        }
        else if (!strcmp(argv[i], "-r") || !strcmp(argv[i], "--range"))
        {
            strcpy(range, argv[i + 1]);
            isdiscover = 1;
        }
        else if (!strcmp(argv[i], "-k") || !strcmp(argv[i], "--knock"))
        {
            if (mark)
            {
                break;
            }
            if (argv[i + 1] != NULL)
            {
                strcpy(ipfilename, argv[i + 1]);
                i++;
            }
            else
            {
                printf("\nPlease specify an address list.\n");
                exit(0);
            }
            isknock = 1;
            mark = 1;
        }
        else if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "--output"))
        {
            if (mark)
            {
                break;
            }
            if (argv[i + 1] != NULL)
            {
                strcpy(ipfilename, argv[i + 1]);
                i++;
            }
            else
            {
                printf("\nPlease specify a name.\n");
                exit(0);
            }
            isdiscover = 1;
            isknock = 1;
            mark = 1;
        }
        else if (!strcmp(argv[i], "-p") || !strcmp(argv[i], "--port"))
        {
            if (argv[i + 1] != NULL)
            {
                strcpy(port, argv[i + 1]);
                i++;
            }
            else
            {
                printf("\nPlease specify a port.\n");
                exit(0);
            }
            if (!isdiscover && !isknock && !isversion)
            {
                isdiscover = 1;
                isknock = 1;
            }
        }
        else if (!strcmp(argv[i], "-t") || !strcmp(argv[i], "--type"))
        {
            if (argv[i + 1] != NULL)
            {
                strcpy(ipfilename, argv[i + 1]);
                i++;
            }
            else
            {
                printf("\nPlease specify a file.\n");
                exit(0);
            }
            isversion = 1;
            mark = 1;
        }
        else if (!strcmp(argv[i], "-n") || !strcmp(argv[i], "--numaddr"))
        {
            if (isknock && !isdiscover)
            {
                if (argv[i + 1] != NULL)
                {
                    if (argv[i + 1][0] != '-')
                    {
                        i++;
                    }
                }
                continue;
            }
            if (argv[i + 1] != NULL)
            {
                strcpy(irand, argv[i + 1]);
                i++;
            }
            else
            {
                printf("\nPlease specify a value.\n");
                exit(0);
            }
            if (!isdiscover && !isknock && !isversion)
            {
                isdiscover = 1;
                isknock = 1;
            }
        }
        else if (!strcmp(argv[i], "-a") || !strcmp(argv[i], "--auto"))
        {
            if (argv[i + 1] != NULL)
            {
                if (argv[i + 1][0] != '-')
                {
                    strcpy(knocklist, argv[i + 1]);
                    i++;
                }
            }
            isauto = 1;
        }
        else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
        {
            printf("Usage:\n");
            printf("    -o <filename>               runs -d and -k\n");
            printf("    -n <# of addr>              number of addr to search\n");
            printf("    -p <port>                   default 23\n");
            printf("    -d <filename>               creates file of addr whose ports are open\n");
            printf("    -t <filename>               version detection of addr\n");
            printf("    -k <filename>               creates connection for addr on specified port\n");
            printf("    -a <filename>               auto-knock using specified wordlist\n\n");
            printf("    -r <ip range>               search ip range\n\n");
            printf("Default values\n");
            printf("    filename: ip.txt\n");
            printf("    port: 23\n");
            printf("    # of addr: 1000\n\n");
            exit(0);
        }
    }

    if (isdiscover)
    {
        safe_printf("Searching %s addresses port %s\nPlease wait...\n", (strlen(range) ? range : irand), port);
        char *execArgsS[100];
        pid = fork();
        if (pid == 0)
        {
            fo = open(nmapoutname, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
            dup2(fo, 1);
            close(fo);
            char *execArgsS[100];
            printf("%s\n", range);
            if (strlen(range) > 0)
            {
                execArgsS[0] = "nmap";
                execArgsS[1] = "-n";
                execArgsS[2] = "-Pn";
                execArgsS[3] = "-sS";
                execArgsS[4] = "-p";
                execArgsS[5] = port;
                execArgsS[6] = "--open";
                execArgsS[7] = range;
                execArgsS[8] = NULL;
            }
            else
            {
                execArgsS[0] = "nmap";
                execArgsS[1] = "-n";
                execArgsS[2] = "-Pn";
                execArgsS[3] = "-sS";
                execArgsS[4] = "-p";
                execArgsS[5] = port;
                execArgsS[6] = "--open";
                execArgsS[7] = "--iR";
                execArgsS[8] = irand;
                execArgsS[9] = NULL;
            }
            int discoverSpawn = execvp(execArgsS[0], execArgsS);
            exit(0);
        }
        else
        {
            parse_ip();
        }
    }

    if (isversion)
    {

        FILE *ipfd = fopen(ipfilename, "rw+");

        char ipwithver[100];
        strcpy(ipwithver, "ver_");
        strcat(ipwithver, ipfilename);

        FILE *ipwithverfd = fopen(ipwithver, "w");
        fclose(ipwithverfd);

        char buf[1000];
        while (fgets(buf, 1000, ipfd) != NULL)
        {
            const char t[2] = " \n";
            token = strtok(buf, t);

            pthread_create(&timeout_t, NULL, time_out, NULL);

            pid = fork();
            if (pid == 0)
            {
                fo = open(nmapoutname, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
                so = dup(1);
                dup2(fo, 1);
                close(fo);
                char *execArgs[] = {"nmap", "-n", "-Pn", "-sV", "-p", port, token, NULL};
                int versionSpawn = execvp(execArgs[0], execArgs);
                dup2(so, 1);
                close(so);
                exit(0);
            }
            else
            {
                parse_version_ip(ipwithver, token);
            }
        }
    }

    if (isknock)
    {
        printf("Starting knock\n");
        FILE *ipfd = fopen(ipfilename, "rw+");
        char buf[1000];
        char cmd[1000];
        int ii = 0;
        while (fgets(buf, 1000, ipfd) != NULL)
        {
            //ii > 0 ? sleep(60) : sleep(0);
            ++ii;
            char *execArgs[100];
            for (int i = 0; i < 100; ++i)
            {
                execArgs[i] = (char *)malloc(100 * sizeof(char));
                memset(execArgs[i], '\0', 100);
            }
            if (0==strcmp(port, "21"))
            {
                strcpy(cmd, "ftp ");
            }
            else if (0==strcmp(port, "22"))
            {
                strcpy(cmd, "ssh ");
            }
            else if (0==strcmp(port, "23"))
            {
                // strcpy(execArgs[0], "telnet");
                // execArgs[2] = "23";
                // execArgs[3] = NULL;
                strcpy(cmd, "telnet ");
                strcat(cmd, buf);
            }
            else
            {
                strcpy(cmd, "ncat ");
                strcat(cmd, port);
                strcat(cmd, " -v");
            }
            //strcpy(execArgs[1], buf);
            int fdp[2];
            pipe(fdp);
            pid = fork();

            if (isauto)
            {
                if (pid == 0)
                {
                    int result = open("auto.txt", O_WRONLY | O_APPEND);
                    //close(fdp[0]);
                    //close(fdp[0]);
                    dup2(fdp[0], STDIN_FILENO);
                    close(fdp[0]);
                    //dup2(result, STDOUT_FILENO);
                    //close(result);
                    //safe_printf("SPAWNING\n");
                    system(cmd);
                    //int knockSpawn = execvp(execArgs[0], execArgs);
                    //safe_printf("%i %s %s %s\n", knockSpawn, execArgs[0], execArgs[1], execArgs[2]);
                }
                else
                {
                    int oldout = dup(1);
                    //close(oldout);
                    //close(fdp[1]);
                    dup2(fdp[1], STDOUT_FILENO);
                    close(fdp[1]);
                    sleep(5);
                    FILE *knocklistfd = fopen(knocklist, "r+");
                    if (knocklistfd == NULL)
                    {
                        printf("Error!!");
                    }

                    char kbuf[1000];
                    while (fgets(kbuf, 1000, knocklistfd) != NULL)
                    {
                        //safe_printf("%s\n", kbuf);
                        const char t[1] = " ";
                        token = strtok(kbuf, t);
                        strcat(token, "\r");
                        for (int i = 0; token != NULL; i++)
                        {
                            if (write(STDOUT_FILENO, token, strlen(token)) == -1)
                            {
                                close(oldout);
                                dup2(oldout, 1);
                                printf("FAILED");
                                kill(pid, SIGKILL);
                                waitpid(-1, &status, 0);
                                break;
                            }
                            sleep(5);
                            token = strtok(NULL, t);
                            if (token != NULL && strcmp(token, "empty") == 0)
                            {
                                token = "";
                            }
                        }
                    }
                    close(oldout);
                    dup2(oldout, 1);
                    kill(pid, SIGKILL);
                    waitpid(-1, &status, 0);
                }
            }
            else
            {
                if (pid == 0)
                {
                    int knockSpawn = execvp(execArgs[0], execArgs);
                }
                else
                {
                    waitpid(-1, &status, 0);
                }
            }
        }
    }
    return 0;
}

void *time_out(void *arg)
{
    usleep(15000000);
    safe_printf("%s timeout!\n", token);
    kill(pid, SIGINT);
    dup2(so, 1);
    close(so);
    return NULL;
}

void safe_printf(const char *format, ...)
{
    char buf[MAXS];
    va_list args;

    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    write(1, buf, strlen(buf)); /* write is async-signal-safe */
}

void ctrlc_handler()
{
    close(fo);
    kill(pid, SIGINT);
    dup2(so, 1);
    close(so);
    exit(0);
}

void ctrlz_handler()
{
    safe_printf("\n");
    kill(pid, SIGINT);
}

void parse_version_ip(char *filename, char *addr)
{
    waitpid(-1, &status, 0);
    pthread_cancel(timeout_t);
    FILE *nnapoutfd = fopen(nmapoutname, "r+");
    FILE *ipwithverfd = fopen(filename, "a");

    char buf[1000];
    int i = 0;
    while (fgets(buf, 1000, nnapoutfd) != NULL)
    {
        if (i == 5)
        {
            fprintf(ipwithverfd, "%s\t%s", addr, buf);
            printf("%s\t%s", addr, buf);
        }
        i++;
    }
    fclose(ipwithverfd);
    fclose(nnapoutfd);
}

void parse_ip()
{
    waitpid(-1, &status, 0);

    FILE *nmapoutfd = fopen(nmapoutname, "r+");
    FILE *ipfd = fopen(ipfilename, "w+");

    if (nmapoutfd == NULL)
    {
        printf("Error!!");
    }
    if (ipfd == NULL)
    {
        printf("Error!!");
    }
    char buf[1000];
    while (fgets(buf, 1000, nmapoutfd) != NULL)
    {
        int ipline = 0;
        token = strtok(buf, " ");
        for (int i = 0; token != NULL; i++)
        {
            if (i == 0 && !strcmp(token, "Nmap"))
            {
                ipline = 1;
            }
            else if (ipline && i == 4 && isdigit(token[0]))
            {
                fprintf(ipfd, "%s", token);
            }
            token = strtok(NULL, " ");
        }
    }

    fclose(nmapoutfd);
    fclose(ipfd);
}
