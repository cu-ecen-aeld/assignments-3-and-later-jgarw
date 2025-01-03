#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h> 

#define PORT 9000
#define FILE_PATH "/var/tmp/aesdsocketdata"

int createSocket(){
    struct sockaddr_in sockaddr;
    int socketfd = socket(PF_INET, SOCK_STREAM, 0);
    int optval = -1;

    // if error occurs creating socketfd
    if(socketfd == -1){
        syslog(LOG_ERR, "Failed to created socket file descriptor.");
        return -1;
    }

    // Set socket options to reuse the address
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        syslog(LOG_ERR, "Failed to set socket options.");
        close(socketfd);
        return -1;
    }

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = INADDR_ANY;
    sockaddr.sin_port = htons(PORT);

    if(bind(socketfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) == -1){
        syslog(LOG_ERR, "Failed to bind socket file descriptor and address.");
        close(socketfd);
        return -1;
    }

    if(listen(socketfd, 10) == -1){
        syslog(LOG_ERR, "Failed while listening.");
        close(socketfd);
        return -1;
    }

    return socketfd;

}

int handleClient(int client_fd){

    char buffer[1024];
    ssize_t bytes_received;

    FILE *file = fopen(FILE_PATH, "a");

    if(file == NULL){
        syslog(LOG_ERR, "Error opening file.");
        return -1;
    }

     // Receive data from the client and append it to the file
    while ((bytes_received = recv(client_fd, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytes_received, file);
    }

    fclose(file);


    // Reopen the file to send the contents back to the client
    file = fopen(FILE_PATH, "r");
    if (file == NULL) {
        syslog(LOG_ERR, "Failed to open file for reading");
        close(client_fd);
        return -1;
    }

    // Send the contents of the file back to the client
    while ((bytes_received = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(client_fd, buffer, bytes_received, 0);
    }

    fclose(file);
    close(client_fd);

    return 0;
}

int main(){

    int server_fd = createSocket();

    if(server_fd == -1){
        syslog(LOG_ERR, "Failed to create server socket file descriptor.");
        return EXIT_FAILURE;
    }

    syslog(LOG_INFO, "Server is listening on port %d", PORT);
    printf("Server is listening on port %d...", PORT);

    // Accept a connection (optional for demonstration)
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd == -1) {
        syslog(LOG_ERR, "accept failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    handleClient(client_fd);

    // Log the client's IP address
    syslog(LOG_INFO, "Accepted connection from %s", inet_ntoa(client_addr.sin_addr));
    printf("Accepted connection from %s", inet_ntoa(client_addr.sin_addr));

    close(client_fd);
    close(server_fd);

    return EXIT_SUCCESS;

}