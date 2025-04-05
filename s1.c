// S1.c - Main server that handles client requests and distributes files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>

#define PORT 8080
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024

// Function to check file extension
const char *get_file_extension(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot + 1;
}

// Function to forward file to appropriate server
void forward_file(const char *filename, const char *file_data, size_t file_size, int server_port) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        return;
    }
    
    // Send filename and file data
    send(sock, filename, strlen(filename), 0);
    usleep(100000); // Small delay to ensure separate messages
    send(sock, file_data, file_size, 0);
    
    close(sock);
}

// Client handler thread function
void *handle_client(void *arg) {
    int client_socket = *((int *)arg);
    char buffer[BUFFER_SIZE] = {0};
    char filename[256] = {0};
    
    // Receive command from client
    read(client_socket, buffer, BUFFER_SIZE);
    printf("Received command: %s\n", buffer);
    
    if (strncmp(buffer, "uploadf", 7) == 0) {
        // Receive filename
        read(client_socket, filename, sizeof(filename));
        printf("Receiving file: %s\n", filename);
        
        // Receive file data
        char file_data[BUFFER_SIZE * 10] = {0}; // Larger buffer for file data
        int bytes_received = read(client_socket, file_data, sizeof(file_data));
        
        const char *ext = get_file_extension(filename);
        
        // Determine where to store the file
        if (strcmp(ext, "c") == 0) {
            // Store locally
            FILE *fp = fopen(filename, "wb");
            fwrite(file_data, 1, bytes_received, fp);
            fclose(fp);
            send(client_socket, "File stored on S1", 17, 0);
        } else if (strcmp(ext, "pdf") == 0) {
            forward_file(filename, file_data, bytes_received, 8081); // S2's port
            send(client_socket, "File forwarded to S2", 21, 0);
        } else if (strcmp(ext, "txt") == 0) {
            forward_file(filename, file_data, bytes_received, 8082); // S3's port
            send(client_socket, "File forwarded to S3", 21, 0);
        } else if (strcmp(ext, "zip") == 0) {
            forward_file(filename, file_data, bytes_received, 8083); // S4's port
            send(client_socket, "File forwarded to S4", 21, 0);
        } else {
            send(client_socket, "Unsupported file type", 21, 0);
        }
    } else if (strncmp(buffer, "downlf", 6) == 0) {
        // Download file implementation
        // (Would need to check local storage or request from appropriate server)
        send(client_socket, "Download functionality not implemented", 37, 0);
    } else if (strncmp(buffer, "removef", 7) == 0) {
        // Remove file implementation
        send(client_socket, "Remove functionality not implemented", 35, 0);
    } else if (strncmp(buffer, "dispfnames", 9) == 0) {
        // Display file names
        DIR *d;
        struct dirent *dir;
        d = opendir(".");
        if (d) {
            char file_list[BUFFER_SIZE] = {0};
            while ((dir = readdir(d)) != NULL) {
                if (dir->d_type == DT_REG) { // Regular file
                    strcat(file_list, dir->d_name);
                    strcat(file_list, "\n");
                }
            }
            closedir(d);
            send(client_socket, file_list, strlen(file_list), 0);
        } else {
            send(client_socket, "Could not open directory", 24, 0);
        }
    } else {
        send(client_socket, "Unknown command", 15, 0);
    }
    
    close(client_socket);
    free(arg);
    pthread_exit(NULL);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    
    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Listen for connections
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    printf("S1 server listening on port %d\n", PORT);
    
    while (1) {
        // Accept new connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        
        printf("New connection accepted\n");
        
        // Create new thread for client
        pthread_t thread_id;
        int *client_socket = malloc(sizeof(int));
        *client_socket = new_socket;
        
        if (pthread_create(&thread_id, NULL, handle_client, (void *)client_socket) < 0) {
            perror("could not create thread");
            exit(EXIT_FAILURE);
        }
        
        // Detach thread so resources are freed when done
        pthread_detach(thread_id);
    }
    
    return 0;
}