// Distributed File System - S4 Server Implementation (ZIP Files)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/wait.h>
#include <sys/sendfile.h>
#include <time.h>
#include <errno.h>

#define PORT 4310
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024
#define MAX_PATH_LEN 1024

// Function prototypes
void handle_client(int client_sock);
int upload_file(int client_sock, char *filename, char *dest_path);
int download_file(int client_sock, char *filename);
int remove_file(int client_sock, char *filename);
int display_filenames(int client_sock, char *pathname);
int create_directory_tree(char *path);
void error(const char *msg);

int main() {
    int sockfd, newsockfd;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    pid_t pid;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("ERROR opening socket");
    }

    // Initialize socket structure
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    // Bind the host address
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR on binding");
    }

    // Start listening for the clients
    listen(sockfd, MAX_CLIENTS);
    clilen = sizeof(cli_addr);

    printf("S4 server (ZIP files) started on port %d\n", PORT);

    while (1) {
        // Accept connection from client (actually from S1)
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {
            error("ERROR on accept");
        }

        // Create child process to handle client
        pid = fork();
        if (pid < 0) {
            error("ERROR on fork");
        }

        if (pid == 0) {
            // Child process
            close(sockfd);
            handle_client(newsockfd);
            close(newsockfd);
            exit(0);
        } else {
            // Parent process
            close(newsockfd);
            // Clean up zombie processes
            while (waitpid(-1, NULL, WNOHANG) > 0);
        }
    }

    close(sockfd);
    return 0;
}

void handle_client(int client_sock) {
    char buffer[BUFFER_SIZE];
    int n;
    
    // Read command from client (S1)
    bzero(buffer, BUFFER_SIZE);
    n = read(client_sock, buffer, BUFFER_SIZE - 1);
    if (n < 0) {
        error("ERROR reading from socket");
    }
    
    printf("Received command: %s\n", buffer);
    
    // Parse command
    char *cmd = strtok(buffer, " ");
    if (cmd == NULL) {
        write(client_sock, "ERROR: Invalid command", 22);
        return;
    }
    
    if (strcmp(cmd, "uploadf") == 0) {
        char *filename = strtok(NULL, " ");
        char *dest_path = strtok(NULL, " ");
        if (filename == NULL || dest_path == NULL) {
            write(client_sock, "ERROR: Invalid uploadf command format", 36);
            return;
        }
        upload_file(client_sock, filename, dest_path);
    } 
    else if (strcmp(cmd, "downlf") == 0) {
        char *filename = strtok(NULL, " ");
        if (filename == NULL) {
            write(client_sock, "ERROR: Invalid downlf command format", 34);
            return;
        }
        download_file(client_sock, filename);
    } 
    else if (strcmp(cmd, "removef") == 0) {
        char *filename = strtok(NULL, " ");
        if (filename == NULL) {
            write(client_sock, "ERROR: Invalid removef command format", 35);
            return;
        }
        remove_file(client_sock, filename);
    } 
    else if (strcmp(cmd, "dispfnames") == 0) {
        char *pathname = strtok(NULL, " ");
        if (pathname == NULL) {
            write(client_sock, "ERROR: Invalid dispfnames command format", 38);
            return;
        }
        display_filenames(client_sock, pathname);
    } 
    else {
        write(client_sock, "ERROR: Unknown command", 21);
    }
}

int upload_file(int client_sock, char *filename, char *dest_path) {
    // First, check if the file is a ZIP file
    char *ext = strrchr(filename, '.');
    if (ext == NULL || strcmp(ext, ".zip") != 0) {
        write(client_sock, "ERROR: S4 only handles ZIP files", 31);
        return -1;
    }
    
    // Create destination path in S4
    char s4_path[MAX_PATH_LEN];
    snprintf(s4_path, MAX_PATH_LEN, "%s/S4%s", getenv("HOME"), dest_path + 3); // +3 to skip "~S1"
    
    // Create directory tree if needed
    if (create_directory_tree(s4_path) < 0) {
        write(client_sock, "ERROR: Failed to create directory", 32);
        return -1;
    }
    
    // Construct full file path
    char *base_name = basename(filename);
    char full_path[MAX_PATH_LEN];
    snprintf(full_path, MAX_PATH_LEN, "%s/%s", s4_path, base_name);
    
    // Rename/move the file from temporary location (sent by S1) to final destination
    if (rename(filename, full_path) < 0) {
        write(client_sock, "ERROR: Failed to move file to destination", 38);
        return -1;
    }
    
    write(client_sock, "SUCCESS: ZIP file stored in S4", 30);
    return 0;
}

int download_file(int client_sock, char *filename) {
    // Check if file exists in S4
    char s4_path[MAX_PATH_LEN];
    snprintf(s4_path, MAX_PATH_LEN, "%s/S4%s", getenv("HOME"), filename + 3); // +3 to skip "~S1"
    
    struct stat st;
    if (stat(s4_path, &st) != 0) {
        write(client_sock, "ERROR: ZIP file not found in S4", 30);
        return -1;
    }
    
    // Check if it's really a ZIP file
    char *ext = strrchr(s4_path, '.');
    if (ext == NULL || strcmp(ext, ".zip") != 0) {
        write(client_sock, "ERROR: Not a ZIP file", 21);
        return -1;
    }
    
    // Open file
    int fd = open(s4_path, O_RDONLY);
    if (fd < 0) {
        write(client_sock, "ERROR: Failed to open ZIP file", 29);
        return -1;
    }
    
    // Send file size
    write(client_sock, &st.st_size, sizeof(off_t));
    
    // Send file data
    off_t remaining = st.st_size;
    while (remaining > 0) {
        ssize_t sent = sendfile(client_sock, fd, NULL, remaining);
        if (sent <= 0) {
            close(fd);
            write(client_sock, "ERROR: File transfer failed", 27);
            return -1;
        }
        remaining -= sent;
    }
    close(fd);
    return 0;
}

int remove_file(int client_sock, char *filename) {
    // Check if file exists in S4
    char s4_path[MAX_PATH_LEN];
    snprintf(s4_path, MAX_PATH_LEN, "%s/S4%s", getenv("HOME"), filename + 3); // +3 to skip "~S1"
    
    if (unlink(s4_path) == 0) {
        write(client_sock, "SUCCESS: ZIP file deleted from S4", 32);
        return 0;
    }
    
    write(client_sock, "ERROR: ZIP file not found in S4", 30);
    return -1;
}

int display_filenames(int client_sock, char *pathname) {
    // Get the corresponding path in S4
    char s4_path[MAX_PATH_LEN];
    snprintf(s4_path, MAX_PATH_LEN, "%s/S4%s", getenv("HOME"), pathname + 3); // +3 to skip "~S1"
    
    // Check if path exists and is a directory
    struct stat st;
    if (stat(s4_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        write(client_sock, "", 0); // Send empty response if directory doesn't exist
        return 0;
    }
    
    // Get ZIP files from S4
    DIR *dir;
    struct dirent *ent;
    char file_list[BUFFER_SIZE] = {0};
    
    if ((dir = opendir(s4_path)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_type == DT_REG) {
                char *ext = strrchr(ent->d_name, '.');
                if (ext && strcmp(ext, ".zip") == 0) {
                    strncat(file_list, ent->d_name, BUFFER_SIZE - strlen(file_list) - 1);
                    strncat(file_list, "\n", BUFFER_SIZE - strlen(file_list) - 1);
                }
            }
        }
        closedir(dir);
    }
    
    // Send the list to S1
    write(client_sock, file_list, strlen(file_list));
    return 0;
}

int create_directory_tree(char *path) {
    char *p;
    char tmp[MAX_PATH_LEN];
    
    snprintf(tmp, MAX_PATH_LEN, "%s", path);
    
    // Skip leading slash if present
    if (tmp[0] == '/') {
        p = tmp + 1;
    } else {
        p = tmp;
    }
    
    // Create each directory in the path
    while ((p = strchr(p, '/'))) {
        *p = '\0';
        if (mkdir(tmp, 0755) && errno != EEXIST) {
            return -1;
        }
        *p = '/';
        p++;
    }
    
    // Create the final directory
    if (mkdir(tmp, 0755) && errno != EEXIST) {
        return -1;
    }
    
    return 0;
}

void error(const char *msg) {
    perror(msg);
    exit(1);
}