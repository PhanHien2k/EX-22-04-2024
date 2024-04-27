#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 256

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    int ret = recv(client_socket, buffer, BUFFER_SIZE, 0);

    if (ret < 0) {
        perror("Error reading from socket");
        close(client_socket);
        return;
    }

    // Đảm bảo buffer kết thúc bằng null-terminated
    buffer[ret] = '\0';

    // Kiểm tra lệnh của client
    if (strncmp(buffer, "GET_TIME", 8) == 0) {
        // Phân tích định dạng thời gian yêu cầu
        char *format = buffer + 9; // Bỏ qua "GET_TIME " để lấy định dạng

        // Lấy thời gian hiện tại
        time_t now;
        struct tm *tm_info;
        time(&now);
        tm_info = localtime(&now);

        char response[BUFFER_SIZE];
        size_t bytes_written;

        if (strcmp(format, "dd/mm/yyyy") == 0) {
            bytes_written = strftime(response, BUFFER_SIZE, "%d/%m/%Y", tm_info);
        } else if (strcmp(format, "dd/mm/yy") == 0) {
            bytes_written = strftime(response, BUFFER_SIZE, "%d/%m/%y", tm_info);
        } else if (strcmp(format, "mm/dd/yyyy") == 0) {
            bytes_written = strftime(response, BUFFER_SIZE, "%m/%d/%Y", tm_info);
        } else if (strcmp(format, "mm/dd/yy") == 0) {
            bytes_written = strftime(response, BUFFER_SIZE, "%m/%d/%y", tm_info);
        } else {
            bytes_written = snprintf(response, BUFFER_SIZE, "Invalid format\n");
        }

        // Gửi kết quả cho client
        send(client_socket, response, bytes_written, 0);
    } else {
        // Lệnh không hợp lệ
        char *error_message = "Invalid command\n";
        send(client_socket, error_message, strlen(error_message), 0);
    }

    // Đóng kết nối
    close(client_socket);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Tạo socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Thiết lập địa chỉ server
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket tới địa chỉ và cổng
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Lắng nghe kết nối đến từ client
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d...\n", PORT);

    while (1) {
        // Chấp nhận kết nối mới từ client
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }

        // Xử lý client trong một tiến trình con
        pid_t pid = fork();

        if (pid == 0) {
            // Tiến trình con
            close(server_socket); // Đóng server socket trong tiến trình con
            handle_client(client_socket);
            exit(EXIT_SUCCESS);
        } else if (pid > 0) {
            // Tiến trình cha
            close(client_socket); // Đóng client socket trong tiến trình cha
        } else {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }
    }

    // Đóng server socket
    close(server_socket);

    return 0;
}