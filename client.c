#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <zlib.h>
#include <pthread.h>

#define SERVER_IP "127.0.0.1"  // Change to the server's IP address
#define SERVER_PORT 9090        // Change to the server's listening port
#define BUFFER_SIZE 1024
#define FILE_NAME_SIZE 256

struct ClientInfo {
    int socket;
    struct sockaddr_in address;
};

void send_file(int socket, char *file_path, char *file_name) {
    FILE *file;
    char buffer[BUFFER_SIZE];
    int bytes_read;

    file = fopen(file_path, "rb");
    if (file == NULL) {
        perror("File not found");
        return;
    }

    send(socket, file_name, strlen(file_name), 0); // Send the file name

    fseek(file, 0, SEEK_END);
    int file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    sprintf(buffer, "%d", file_size);
    send(socket, buffer, strlen(buffer), 0);

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        send(socket, buffer, bytes_read, 0);
    }

    fclose(file);
}

void receive_compressed_file(int socket) {
    FILE *file;
    gzFile compressed_file;
    char buffer[BUFFER_SIZE];
    int bytes_received;

    bytes_received = recv(socket, buffer, FILE_NAME_SIZE, 0);
    if (bytes_received <= 0) {
        perror("File name not received from the server");
        return;
    }
    buffer[bytes_received] = '\0';

    char *compressed_file_name = buffer;
    char decompressed_file_name[FILE_NAME_SIZE];
    strcpy(decompressed_file_name, "decompressed_");
    strcat(decompressed_file_name, compressed_file_name);

    file = fopen(decompressed_file_name, "wb");
    if (file == NULL) {
        perror("Cannot open or create file for decompression");
        return;
    }

    compressed_file = gzdopen(fileno(file), "wb");
    if (compressed_file == NULL) {
        perror("Error in creating compressed file");
        fclose(file);
        return;
    }

    while ((bytes_received = recv(socket, buffer, BUFFER_SIZE, 0)) > 0) {
        gzwrite(compressed_file, buffer, bytes_received);
    }

    gzclose(compressed_file);
    fclose(file);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        return 1;
    }

    char *local_file_path = argv[1]; // File path provided as a command-line argument

    char file_name[FILE_NAME_SIZE];
    printf("Enter the name of the file: ");
    fgets(file_name, FILE_NAME_SIZE, stdin);
    // Remove the newline character from the end of the input
    file_name[strcspn(file_name, "\n")] = '\0';

    int num_clients = 5;  // Adjust to the number of clients you want to manage
    struct ClientInfo *clients = malloc(num_clients * sizeof(struct ClientInfo));

    if (clients == NULL) {
        perror("Memory allocation error");
        return 1;
    }

    for (int i = 0; i < num_clients; i++) {
        clients[i].socket = -1; // Initialize all client sockets to -1
    }

    for (int i = 0; i < num_clients; i++) {
        clients[i].socket = socket(AF_INET, SOCK_STREAM, 0);
        if (clients[i].socket < 0) {
            perror("Socket creation error");
            exit(EXIT_FAILURE);
        }

        clients[i].address.sin_family = AF_INET;
        clients[i].address.sin_port = htons(SERVER_PORT);

        if (inet_pton(AF_INET, SERVER_IP, &clients[i].address.sin_addr) <= 0) {
            perror("Invalid address/Address not supported");
            exit(EXIT_FAILURE);
        }

        if (connect(clients[i].socket, (struct sockaddr *)&clients[i].address, sizeof(clients[i].address)) < 0) {
            perror("Connection Failed");
            exit(EXIT_FAILURE);
        }

        send_file(clients[i].socket, local_file_path, file_name);
        printf("File '%s' sent to the server.\n", file_name);

        receive_compressed_file(clients[i].socket);
        printf("Compressed file received from the server and decompressed.\n");

        close(clients[i].socket);
        printf("Client socket %d closed.\n", i);
    }

    free(clients); // Release the dynamically allocated memory

    return 0;
}
