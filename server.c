#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <zlib.h>
#include <pthread.h>
#include <time.h> // Added for timestamp

#define PORT 9090
#define MAX_CLIENTS 10  // Maximum number of concurrent clients
#define BUFFER_SIZE 1024
#define FILE_NAME_SIZE 256

void receive_file(int socket); // Function prototype

struct ClientInfo {
    int socket;
    struct sockaddr_in address;
    char file_name[FILE_NAME_SIZE]; // Add a field to store the file name
};

void *client_handler(void *client_socket) {
    struct ClientInfo *client = (struct ClientInfo *)client_socket;

    // Handle the client's requests here
    receive_file(client->socket);

    // Close the client socket
    close(client->socket);
    printf("Client disconnected: %s\n", client->file_name);

    // Free the dynamically allocated client socket structure
    free(client);

    pthread_exit(NULL); // Exit the thread
}

void receive_file(int socket) {
    FILE *file;
    char buffer[BUFFER_SIZE];
    int bytes_received;

    // Receive the file name from the client
    bytes_received = recv(socket, buffer, FILE_NAME_SIZE, 0);
    if (bytes_received <= 0) {
        return;
    }
    buffer[bytes_received] = '\0';
    char *file_name = buffer;

    // Receive the file size from the client
    bytes_received = recv(socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0) {
        return;
    }
    buffer[bytes_received] = '\0';
    int file_size = atoi(buffer);

    // Store the file name in the client information
    strncpy(file_name, file_name, FILE_NAME_SIZE);

    // Open the file for writing
    file = fopen(file_name, "wb");
    if (file == NULL) {
        perror("Cannot open or create file");
        send(socket, "ERROR: Cannot open or create file", strlen("ERROR: Cannot open or create file"), 0);
        return;
    }

    // Receive and write the file data
    int remaining_bytes = file_size;
    while (remaining_bytes > 0) {
        bytes_received = recv(socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            break;
        }
        fwrite(buffer, 1, bytes_received, file);
        remaining_bytes -= bytes_received;
    }

    // Close the file
    fclose(file);

    // Get the current timestamp
    time_t rawtime;
    struct tm *timestamp;
    time(&rawtime);
    timestamp = localtime(&rawtime);

    // Format the timestamp as dd/mm/yyyy hh:mm:ss
    char timestamp_str[20]; // For dd/mm/yyyy hh:mm:ss
    strftime(timestamp_str, sizeof(timestamp_str), "%d/%m/%Y %H:%M:%S", timestamp);

    // Display the file details including the file name
    printf("Received file: %s\n", file_name);
    printf("File Size: %d bytes\n", file_size);
    printf("Received on: %s\n", timestamp_str);
}

int main() {
    int server_socket;
    struct sockaddr_in server_address;
    int opt = 1;
    socklen_t address_len = sizeof(server_address);

    struct ClientInfo clients[MAX_CLIENTS];

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket = 0; // Initialize all client sockets to 0
        memset(clients[i].file_name, 0, FILE_NAME_SIZE); // Initialize file name to empty string
    }

    // Create socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }
    printf("Socket created successfully\n");

    // Set socket options to allow reuse of the address
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt error");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    // Bind the socket to the specified port
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Bind error");
        exit(EXIT_FAILURE);
    }
    printf("Socket binding successful\n");

    // Listen for incoming connections
    if (listen(server_socket, 5) < 0) {
        perror("Listen error");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Accept a new connection
        int new_socket;
        if ((new_socket = accept(server_socket, (struct sockaddr *)&server_address, &address_len)) < 0) {
            perror("Accept error");
            exit(EXIT_FAILURE);
        }
        printf("Client connected from %s:%d\n", inet_ntoa(server_address.sin_addr), ntohs(server_address.sin_port));

        // Find an available slot for the client in the clients array
        int i;
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket == 0) {
                clients[i].socket = new_socket;
                clients[i].address = server_address;
                break;
            }
        }

        if (i == MAX_CLIENTS) {
            printf("Max clients reached. Connection rejected.\n");
            close(new_socket);
        } else {
            // Create a new thread for the client
            pthread_t tid;
            struct ClientInfo *client_info = (struct ClientInfo *)malloc(sizeof(struct ClientInfo));
            client_info->socket = new_socket;
            client_info->address = server_address;
            memset(client_info->file_name, 0, FILE_NAME_SIZE);
            if (pthread_create(&tid, NULL, client_handler, (void *)client_info) != 0) {
                perror("Thread creation error");
                free(client_info);
            }
        }
    }

    // Close the server socket when done
    close(server_socket);
    printf("Server socket closed\n");

    return 0;
}
