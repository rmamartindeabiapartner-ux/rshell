// exec_hook.c
#define _GNU_SOURCE
#include <dlfcn.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void send_exec_info(const char *filename, char *const argv[]) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return;
    
    // Configuration du serveur
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(80);
    inet_pton(AF_INET, "185.181.4.52", &server.sin_addr); // Remplacer par l'IP de ton serveur
    
    // Timeout de connexion
    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    
    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) == 0) {
        // Construire les arguments
        char args[2048] = "";
        int i = 0;
        while (argv[i] != NULL && strlen(args) < 1800) {
            strcat(args, argv[i]);
            strcat(args, " ");
            i++;
        }
        
        // Construire la requête HTTP POST
        char http_request[4096];
        char body[3096];
        
        snprintf(body, sizeof(body), 
            "{\"filename\":\"%s\",\"args\":\"%s\",\"hostname\":\"%s\",\"pid\":%d}",
            filename, args, getenv("HOSTNAME") ?: "unknown", getpid());
        
        snprintf(http_request, sizeof(http_request),
            "POST /api/exec HTTP/1.1\r\n"
            "Host: monitoring-server\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %ld\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            strlen(body), body);
        
        send(sock, http_request, strlen(http_request), 0);
    }
    
    close(sock);
}

int execve(const char *filename, char *const argv[], char *const envp[]) {
    // Envoyer les infos au serveur
    send_exec_info(filename, argv);
    
    // Appeler le vrai execve
    int (*real_execve)(const char*, char *const[], char *const[]);
    real_execve = dlsym(RTLD_NEXT, "execve");
    return real_execve(filename, argv, envp);
}

// Hook d'autres variantes d'exec
int execv(const char *filename, char *const argv[]) {
    send_exec_info(filename, argv);
    int (*real_execv)(const char*, char *const[]);
    real_execv = dlsym(RTLD_NEXT, "execv");
    return real_execv(filename, argv);
}

int execvp(const char *filename, char *const argv[]) {
    send_exec_info(filename, argv);
    int (*real_execvp)(const char*, char *const[]);
    real_execvp = dlsym(RTLD_NEXT, "execvp");
    return real_execvp(filename, argv);
}
