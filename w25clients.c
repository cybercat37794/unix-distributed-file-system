// w25clients.c - Client program for interacting with the distributed file system
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void display_menu() {
    printf("\nDistributed File System Client\n");
    printf("1. uploadf - Upload a file\n");
    printf("2. downlf - Download a file\n");
    printf("3. removef - Remove a file\n");
    printf("4. downltar - Download all files of a type as tar\n");
    printf("5. dispfnames - Display file names\n");
    printf("6. Exit\n");
    printf("Enter command: ");
}

void upload_file(int sock) {
    char filename[256];
    printf("Enter filename to upload: ");
    scanf("%s", filename);
    
    // Open the file
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Could not open file %s\n", filename);
        return;
    }
    
    // Get file size
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    // Read file data
    char *file_data = malloc(file_size);
    fread(file_data, 1, file_size, fp);
    fclose(fp);
    
    // Send command
    send(sock, "uploadf", strlen("uploadf"), 0);
    usleep(100000); // Small delay
    
    // Send filename
    send(sock, filename, strlen(filename), 0);
    usleep(100000); // Small delay
    
    // Send file data
    send(sock, file_data, file_size, 0);
    
    // Receive response
    char response[BUFFER_SIZE] = {0};
    read(sock, response, BUFFER_SIZE);
    printf("Server response: %s\n", response);
    
    free(file_data);
}

void display_file_names(int sock) {
    // Send command
    send(sock, "dispfnames", strlen("dispfnames"), 0);
    
    // Receive response
    char response[BUFFER_SIZE * 10] = {0}; // Larger buffer for file list
    read(sock, response, sizeof(response));
    printf("Files on server:\n%s", response);
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    
    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
    
    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }
    
    printf("Connected to server\n");
    
    int choice;
    char command[20];
    
    while (1) {
        display_menu();
        scanf("%s", command);
        
        if (strcmp(command, "uploadf") == 0) {
            upload_file(sock);
        } else if (strcmp(command, "dispfnames") == 0) {
            display_file_names(sock);
        } else if (strcmp(command, "exit") == 0) {
            break;
        } else {
            printf("Unknown command\n");
        }
    }
    
    close(sock);
    return 0;
}