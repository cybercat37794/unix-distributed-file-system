// Distributed File System - Client Implementation (w25clients.c)

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
#include <libgen.h>
#include <errno.h>

#define PORT 4307
#define BUFFER_SIZE 1024
#define MAX_PATH_LEN 1024

void error(const char *msg);
int connect_to_server();
void handle_uploadf(int sockfd, char *filename, char *dest_path);
void handle_downlf(int sockfd, char *filename);
void handle_removef(int sockfd, char *filename);
void handle_downltar(int sockfd, char *filetype);
void handle_dispfnames(int sockfd, char *pathname);
int send_file(int sockfd, char *filename);
int receive_file(int sockfd, char *filename);

int main() {
    int sockfd;
    char buffer[BUFFER_SIZE];
    
    printf("Distributed File System Client\n");
    printf("Available commands:\n");
    printf("  uploadf <filename> <destination_path>\n");
    printf("  downlf <filename>\n");
    printf("  removef <filename>\n");
    printf("  downltar <filetype>\n");
    printf("  dispfnames <pathname>\n");
    printf("  exit\n\n");
    
    while (1) {
        printf("w25clients$ ");
        bzero(buffer, BUFFER_SIZE);
        fgets(buffer, BUFFER_SIZE - 1, stdin);
        
        // Remove newline
        buffer[strcspn(buffer, "\n")] = 0;
        
        if (strlen(buffer) == 0) {
            continue;
        }
        
        // Check for exit command
        if (strcmp(buffer, "exit") == 0) {
            break;
        }
        
        // Connect to server
        sockfd = connect_to_server();
        if (sockfd < 0) {
            printf("Failed to connect to server\n");
            continue;
        }
        
        // Parse command
        char *cmd = strtok(buffer, " ");
        
        //task 1 
		if (strcmp(cmd, "uploadf") == 0) {
            char *filename = strtok(NULL, " ");
            char *dest_path = strtok(NULL, " ");
            if (filename == NULL || dest_path == NULL) {
                printf("Invalid command format. Usage: uploadf <filename> <destination_path>\n");
                close(sockfd);
                continue;
            }
            handle_uploadf(sockfd, filename, dest_path);
        } 
        //task 2
		else if (strcmp(cmd, "downlf") == 0) {
            char *filename = strtok(NULL, " ");
            if (filename == NULL) {
                printf("Invalid command format. Usage: downlf <filename>\n");
                close(sockfd);
                continue;
            }
            handle_downlf(sockfd, filename);
        }
		// task 3
        else if (strcmp(cmd, "removef") == 0) {
            char *filename = strtok(NULL, " ");
            if (filename == NULL) {
                printf("Invalid command format. Usage: removef <filename>\n");
                close(sockfd);
                continue;
            }
            handle_removef(sockfd, filename);
        } 
		// task 4
        else if (strcmp(cmd, "downltar") == 0) {
            char *filetype = strtok(NULL, " ");
            if (filetype == NULL) {
                printf("Invalid command format. Usage: downltar <filetype>\n");
                close(sockfd);
                continue;
            }
            handle_downltar(sockfd, filetype);
        }
		// task 5
        else if (strcmp(cmd, "dispfnames") == 0) {
            char *pathname = strtok(NULL, " ");
            if (pathname == NULL) {
                printf("Invalid command format. Usage: dispfnames <pathname>\n");
                close(sockfd);
                continue;
            }
            handle_dispfnames(sockfd, pathname);
        } 
        else {
            printf("Unknown command: %s\n", cmd);
        }
        
        close(sockfd);
    }
    
    return 0;
}

int connect_to_server() {
    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("ERROR opening socket");
        return -1;
    }
    
    server = gethostbyname("localhost");
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        return -1;
    }
    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(PORT);
    
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR connecting");
        return -1;
    }
    
    return sockfd;
}

void handle_uploadf(int sockfd, char *filename, char *dest_path) {
    // Verify file exists
    struct stat st;
    if (stat(filename, &st) != 0) {
        printf("ERROR: File '%s' not found\n", filename);
        return;
    }
    
    // Check if destination path starts with ~S1/
    if (strncmp(dest_path, "~S1/", 4) != 0) {
        printf("ERROR: Destination path must start with ~S1/\n");
        return;
    }
    
    // Check file extension
    char *ext = strrchr(filename, '.');
    if (ext == NULL) {
        printf("ERROR: File has no extension\n");
        return;
    }
    
    if (strcmp(ext, ".c") != 0 && strcmp(ext, ".pdf") != 0 && 
        strcmp(ext, ".txt") != 0 && strcmp(ext, ".zip") != 0) {
        printf("ERROR: Unsupported file type. Only .c, .pdf, .txt, .zip allowed\n");
        return;
    }
    
    // Send command to server
    char command[BUFFER_SIZE];
    snprintf(command, BUFFER_SIZE, "uploadf %s %s", filename, dest_path);
    if (write(sockfd, command, strlen(command)) < 0) {
        error("ERROR writing to socket");
        return;
    }
    
    // Wait for server response
    char response[BUFFER_SIZE];
    bzero(response, BUFFER_SIZE);
    if (read(sockfd, response, BUFFER_SIZE - 1) < 0) {
        error("ERROR reading from socket");
        return;
    }
    
    if (strcmp(response, "READY") == 0) {
        // Server is ready to receive file
        if (send_file(sockfd, filename) == 0) {
            // Wait for final response
            bzero(response, BUFFER_SIZE);
            if (read(sockfd, response, BUFFER_SIZE - 1) < 0) {
                error("ERROR reading from socket");
                return;
            }
            printf("%s\n", response);
        }
    } else {
        printf("%s\n", response);
    }
}

void handle_downlf(int sockfd, char *filename) {
    // Check if filename starts with ~S1/
    if (strncmp(filename, "~S1/", 4) != 0) {
        printf("ERROR: Filename must start with ~S1/\n");
        return;
    }
    
    // Check file extension
    char *ext = strrchr(filename, '.');
    if (ext == NULL) {
        printf("ERROR: File has no extension\n");
        return;
    }
    
    if (strcmp(ext, ".c") != 0 && strcmp(ext, ".pdf") != 0 && 
        strcmp(ext, ".txt") != 0 && strcmp(ext, ".zip") != 0) {
        printf("ERROR: Unsupported file type. Only .c, .pdf, .txt, .zip allowed\n");
        return;
    }
    
    // Send command to server
    char command[BUFFER_SIZE];
    snprintf(command, BUFFER_SIZE, "downlf %s", filename);
    if (write(sockfd, command, strlen(command)) < 0) {
        error("ERROR writing to socket");
        return;
    }
    
    // Get the base name for saving locally
    char *base_name = basename(filename);
    
    // Receive file from server
    if (receive_file(sockfd, base_name) == 0) {
        printf("File '%s' downloaded successfully\n", base_name);
    }
}

void handle_removef(int sockfd, char *filename) {
    // Check if filename starts with ~S1/
    if (strncmp(filename, "~S1/", 4) != 0) {
        printf("ERROR: Filename must start with ~S1/\n");
        return;
    }
    
    // Check file extension
    char *ext = strrchr(filename, '.');
    if (ext == NULL) {
        printf("ERROR: File has no extension\n");
        return;
    }
    
    if (strcmp(ext, ".c") != 0 && strcmp(ext, ".pdf") != 0 && 
        strcmp(ext, ".txt") != 0 && strcmp(ext, ".zip") != 0) {
        printf("ERROR: Unsupported file type. Only .c, .pdf, .txt, .zip allowed\n");
        return;
    }
    
    // Send command to server
    char command[BUFFER_SIZE];
    snprintf(command, BUFFER_SIZE, "removef %s", filename);
    if (write(sockfd, command, strlen(command)) < 0) {
        error("ERROR writing to socket");
        return;
    }
    
    // Get server response
    char response[BUFFER_SIZE];
    bzero(response, BUFFER_SIZE);
    if (read(sockfd, response, BUFFER_SIZE - 1) < 0) {
        error("ERROR reading from socket");
        return;
    }
    
    printf("%s\n", response);
}

void handle_downltar(int sockfd, char *filetype) {
    // Check file type
    if (strcmp(filetype, ".c") != 0 && strcmp(filetype, ".pdf") != 0 && 
        strcmp(filetype, ".txt") != 0) {
        printf("ERROR: Unsupported file type for tar. Only .c, .pdf, .txt allowed\n");
        return;
    }
    
    // Determine output filename
    char output_file[50];
    if (strcmp(filetype, ".c") == 0) {
        strcpy(output_file, "cfiles.tar");
    } else if (strcmp(filetype, ".pdf") == 0) {
        strcpy(output_file, "pdfiles.tar");
    } else {
        strcpy(output_file, "txtfiles.tar");
    }
    
    // Send command to server
    char command[BUFFER_SIZE];
    snprintf(command, BUFFER_SIZE, "downltar %s", filetype);
    if (write(sockfd, command, strlen(command)) < 0) {
        error("ERROR writing to socket");
        return;
    }
    
    // Receive tar file from server
    if (receive_file(sockfd, output_file) == 0) {
        printf("Tar file '%s' downloaded successfully\n", output_file);
    }
}

void handle_dispfnames(int sockfd, char *pathname) {
    // Check if pathname starts with ~S1/
    if (strncmp(pathname, "~S1/", 4) != 0) {
        printf("ERROR: Pathname must start with ~S1/\n");
        return;
    }
    
    // Send command to server
    char command[BUFFER_SIZE];
    snprintf(command, BUFFER_SIZE, "dispfnames %s", pathname);
    if (write(sockfd, command, strlen(command)) < 0) {
        error("ERROR writing to socket");
        return;
    }
    
    // Get server response
    char response[BUFFER_SIZE];
    bzero(response, BUFFER_SIZE);
    if (read(sockfd, response, BUFFER_SIZE - 1) < 0) {
        error("ERROR reading from socket");
        return;
    }
    
    printf("Files in %s:\n%s", pathname, response);
}

int send_file(int sockfd, char *filename) {
    int fd;
    char buffer[BUFFER_SIZE];
    ssize_t n;
    
    // Open file
    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        printf("ERROR: Failed to open file '%s'\n", filename);
        return -1;
    }
    
    // Get file size
    struct stat st;
    if (fstat(fd, &st) < 0) {
        printf("ERROR: Failed to get file size\n");
        close(fd);
        return -1;
    }
    
    // Send file size
    if (write(sockfd, &st.st_size, sizeof(off_t)) < 0) {
        error("ERROR writing file size to socket");
        close(fd);
        return -1;
    }
    
    // Send file data
    off_t remaining = st.st_size;
    while (remaining > 0) {
        n = read(fd, buffer, BUFFER_SIZE);
        if (n <= 0) {
            printf("ERROR: Failed to read from file\n");
            close(fd);
            return -1;
        }
        
        if (write(sockfd, buffer, n) < 0) {
            error("ERROR writing to socket");
            close(fd);
            return -1;
        }
        
        remaining -= n;
    }
    
    close(fd);
    return 0;
}

int receive_file(int sockfd, char *filename) {
    int fd;
    char buffer[BUFFER_SIZE];
    ssize_t n;
    
    // Read file size
    off_t file_size;
    if (read(sockfd, &file_size, sizeof(off_t)) < 0) {
        error("ERROR reading file size from socket");
        return -1;
    }
    
    if (file_size <= 0) {
        char error_msg[BUFFER_SIZE];
        if (read(sockfd, error_msg, BUFFER_SIZE - 1) < 0) {
            error("ERROR reading from socket");
        } else {
            printf("%s\n", error_msg);
        }
        return -1;
    }
    
    // Create file
    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        printf("ERROR: Failed to create file '%s'\n", filename);
        return -1;
    }
    
    // Receive file data
    off_t remaining = file_size;
    while (remaining > 0) {
        n = read(sockfd, buffer, (remaining < BUFFER_SIZE) ? remaining : BUFFER_SIZE);
        if (n <= 0) {
            printf("ERROR: File transfer failed\n");
            close(fd);
            unlink(filename);
            return -1;
        }
        
        if (write(fd, buffer, n) < 0) {
            printf("ERROR: Failed to write to file\n");
            close(fd);
            unlink(filename);
            return -1;
        }
        
        remaining -= n;
    }
    
    close(fd);
    return 0;
}

void error(const char *msg) {
    perror(msg);
}