#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <syslog.h>
#include <errno.h>
#include <fcntl.h>

#define PORT 9000
#define BUFFER_SIZE 1024
#define FILE_PATH "/var/tmp/aesdsocketdata"

int socket_fd;
int is_running = 1;
FILE *storage_file = NULL;

void setup_signal_handlers();
void setup_server_socket();
void handle_client_connection(int client_fd, struct sockaddr_in *client_addr);
void receive_data_and_store(int client_fd);
void send_data_to_client(int client_fd);
void close_resources();

void cleanup() {
    if (storage_file) fclose(storage_file);
    if (socket_fd != -1) close(socket_fd);
    remove(FILE_PATH);
    closelog();
}

void handle_signal(int signal_number) {
    if (signal_number == SIGINT || signal_number == SIGTERM) {
        syslog(LOG_INFO, "Received termination signal, shutting down");
        is_running = 0;
        cleanup();
        exit(0);
    }
}

void setup_signal_handlers() {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
}

void setup_server_socket() {
    struct sockaddr_in server_address;

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        syslog(LOG_ERR, "Error creating socket: %s", strerror(errno));
        exit(-1);
    }

    int opt = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        syslog(LOG_ERR, "Error setting socket option: %s", strerror(errno));
        close(socket_fd);
        exit(-1);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    if (bind(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        syslog(LOG_ERR, "Error binding socket: %s", strerror(errno));
        close(socket_fd);
        exit(-1);
    }

    if (listen(socket_fd, 5) < 0) {
        syslog(LOG_ERR, "Error listening on socket: %s", strerror(errno));
        close(socket_fd);
        exit(-1);
    }
}

void receive_data_and_store(int client_fd) {
    char data_buffer[1024] = {0};
    int received_bytes;
    
    while ((received_bytes = recv(client_fd, data_buffer, 1024 - 1, 0)) > 0) {
        data_buffer[received_bytes] = '\0';
        fwrite(data_buffer, sizeof(char), received_bytes, storage_file);
        if (strchr(data_buffer, '\n') != NULL) {
            fflush(storage_file);
            break;
        }
    }

    if (received_bytes < 0) {
        syslog(LOG_ERR, "Error receiving data: %s", strerror(errno));
    }
}

void send_data_to_client(int client_fd) {
    char data_buffer[1024] = {0};
    size_t read_bytes;

    fseek(storage_file, 0, SEEK_SET);
    while ((read_bytes = fread(data_buffer, sizeof(char), 1024 - 1, storage_file)) > 0) {
        data_buffer[read_bytes] = '\0';
        ssize_t sent_bytes = send(client_fd, data_buffer, read_bytes, 0);
        if (sent_bytes < 0) {
            syslog(LOG_ERR, "Error sending data: %s", strerror(errno));
            break;
        }
    }
}

void handle_client_connection(int client_fd, struct sockaddr_in *client_addr) {
    char *client_ip = inet_ntoa(client_addr->sin_addr);
    syslog(LOG_INFO, "Connection established with client: %s", client_ip);

    receive_data_and_store(client_fd);
    send_data_to_client(client_fd);

    close(client_fd);
    syslog(LOG_INFO, "Connection closed with client: %s", client_ip);
}

int main(int argc, char *argv[]) {
    struct sockaddr_in client_address;
    int address_length = sizeof(client_address);

    openlog("myserver", LOG_PID | LOG_CONS, LOG_USER);

    setup_signal_handlers();
    setup_server_socket();

    storage_file = fopen(FILE_PATH, "w+");
    if (storage_file == NULL) {
        syslog(LOG_ERR, "Error opening file: %s", strerror(errno));
        close(socket_fd);
        return -1;
    }

    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        if (fork() > 0) exit(0);

        setsid();

        signal(SIGCHLD, SIG_IGN);
        signal(SIGHUP, SIG_IGN);

        umask(0);
        chdir("/");

        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }

    while (is_running) {
        int client_fd;
        if ((client_fd = accept(socket_fd, (struct sockaddr *)&client_address, (socklen_t*)&address_length)) < 0) {
            if (errno == EINTR) continue;
            syslog(LOG_ERR, "Error accepting connection: %s", strerror(errno));
            continue;
        }

        handle_client_connection(client_fd, &client_address);
    }

    cleanup();
    return 0;
}