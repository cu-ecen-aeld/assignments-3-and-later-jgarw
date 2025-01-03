#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <syslog.h>
#include <fcntl.h>
#include <stdbool.h>

#define SERVER_PORT 9000
#define QUEUE_LENGTH 10
#define STORAGE_FILE "/var/tmp/serverdata"

int server_fd = -1;
bool server_active = true;

void handle_signal(int sig) {
    printf("Signal %d received\n", sig);
    server_active = false;
}

void clean_up() {
    if (server_fd != -1) {
        close(server_fd);
        server_fd = -1;
    }
    unlink(STORAGE_FILE);
    syslog(LOG_INFO, "Server shutting down");
    closelog();
}

int create_storage_file() {
    int file_fd = open(STORAGE_FILE, O_CREAT | O_RDWR | O_APPEND, 0644);
    if (file_fd == -1) {
        syslog(LOG_ERR, "Error opening storage file: %s", strerror(errno));
    }
    return file_fd;
}

void process_request(int client_fd, struct sockaddr_in *client_info) {
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    int file_fd = create_storage_file();
    if (file_fd == -1) {
        close(client_fd);
        return;
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_info->sin_addr, client_ip, INET_ADDRSTRLEN);
    syslog(LOG_INFO, "Connection from %s", client_ip);

    bool continue_receiving = true;
    ssize_t bytes_read = 0;
    while (continue_receiving) {
        bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        buffer[bytes_read] = '\0';
        syslog(LOG_INFO, "Received bytes: %zd", bytes_read);
        syslog(LOG_DEBUG, "Data: %s", buffer);

        if (bytes_read > 0) {
            if (write(file_fd, buffer, bytes_read) == -1) {
                syslog(LOG_ERR, "Error writing to storage file: %s", strerror(errno));
                break;
            }
        }

        if (strchr(buffer, '\n')) {
            lseek(file_fd, 0, SEEK_SET);

            memset(buffer, 0, sizeof(buffer));
            while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
                if (send(client_fd, buffer, bytes_read, 0) == -1) {
                    syslog(LOG_ERR, "Error sending data to client: %s", strerror(errno));
                    break;
                }
            }
            continue_receiving = false;
        }
    }

    close(file_fd);
    close(client_fd);
    syslog(LOG_INFO, "Connection from %s closed", client_ip);
}

void start_daemon() {
    pid_t pid = fork();
    if (pid < 0) {
        syslog(LOG_ERR, "Fork failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    if (setsid() < 0) {
        syslog(LOG_ERR, "Session creation failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_RDWR);
    open("/dev/null", O_RDWR);
}

int initialize_socket() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        syslog(LOG_ERR, "Socket creation failed: %s", strerror(errno));
    }
    return fd;
}

int bind_socket(int fd, struct sockaddr_in *server_address) {
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        syslog(LOG_ERR, "Setting SO_REUSEADDR failed: %s", strerror(errno));
        return -1;
    }

    if (bind(fd, (struct sockaddr *)server_address, sizeof(*server_address)) == -1) {
        syslog(LOG_ERR, "Binding failed: %s", strerror(errno));
        return -1;
    }
    return 0;
}

void configure_signal_handlers() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGTSTP, &sa, NULL);
}

int main(int argc, char *argv[]) {
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_len = sizeof(client_address);
    bool is_daemon = false;

    int opt_char;
    while ((opt_char = getopt(argc, argv, "d")) != -1) {
        switch (opt_char) {
            case 'd':
                is_daemon = true;
                break;
            default:
                fprintf(stderr, "Usage: %s [-d]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    openlog("simple_server", LOG_PID, LOG_USER);

    configure_signal_handlers();

    server_fd = initialize_socket();
    if (server_fd == -1) return -1;

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);

    printf("Server binding to %s:%d\n", inet_ntoa(server_address.sin_addr), ntohs(server_address.sin_port));

    if (bind_socket(server_fd, &server_address) == -1) {
        clean_up();
        return -1;
    }

    if (is_daemon) {
        start_daemon();
    }

    if (listen(server_fd, QUEUE_LENGTH) == -1) {
        syslog(LOG_ERR, "Listening failed: %s", strerror(errno));
        clean_up();
        return -1;
    }

    while (server_active) {
        int client_fd = accept(server_fd, (struct sockaddr *)&client_address, &client_address_len);
        if (client_fd == -1) {
            if (errno == EINTR) {
                break;
            }
            syslog(LOG_ERR, "Connection acceptance failed: %s", strerror(errno));
            continue;
        }

        syslog(LOG_INFO, "Connection from %s, port %d", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
        process_request(client_fd, &client_address);
    }

    clean_up();
    return 0;
}
