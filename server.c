#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

/**
 * Project 1 starter code
 * All parts needed to be changed/added are marked with TODO
 */

#define BUFFER_SIZE 1024
#define DEFAULT_SERVER_PORT 8081
#define DEFAULT_REMOTE_HOST "127.0.0.1"
#define DEFAULT_REMOTE_PORT 5001

struct server_app {
    // Parameters of the server
    // Local port of HTTP server
    uint16_t server_port;

    // Remote host and port of remote proxy
    char *remote_host;
    uint16_t remote_port;
};

// The following function is implemented for you and doesn't need
// to be change
void parse_args(int argc, char *argv[], struct server_app *app);

// The following functions need to be updated
void handle_request(struct server_app *app, int client_socket);
void serve_local_file(int client_socket, const char *path);
void proxy_remote_file(struct server_app *app, int client_socket, const char *path);

// The main function is provided and no change is needed
int main(int argc, char *argv[])
{
    struct server_app app;
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    int ret;

    parse_args(argc, argv, &app);
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(app.server_port);

    // The following allows the program to immediately bind to the port in case
    // previous run exits recently
    int optval = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) == -1) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", app.server_port);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == -1) {
            perror("accept failed");
            continue;
        } //loop looking for socket
        
        printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        handle_request(&app, client_socket);
        close(client_socket);
    }

    close(server_socket);
    return 0;
}
void parse_args(int argc, char *argv[], struct server_app *app)
{
    int opt;

    app->server_port = DEFAULT_SERVER_PORT;
    app->remote_host = NULL;
    app->remote_port = DEFAULT_REMOTE_PORT;

    while ((opt = getopt(argc, argv, "b:r:p:")) != -1) {
        switch (opt) {
        case 'b':
            app->server_port = atoi(optarg);
            break;
        case 'r':
            app->remote_host = strdup(optarg);
            break;
        case 'p':
            app->remote_port = atoi(optarg);
            break;
        default: /* Unrecognized parameter or "-?" */
            fprintf(stderr, "Usage: server [-b local_port] [-r remote_host] [-p remote_port]\n");
            exit(-1);
            break;
        }
    }

    if (app->remote_host == NULL) {
        app->remote_host = strdup(DEFAULT_REMOTE_HOST);
    }
}

void handle_request(struct server_app *app, int client_socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Read the request from HTTP client
    // Note: This code is not ideal in the real world because it
    // assumes that the request header is small enough and can be read
    // once as a whole.
    // However, the current version suffices for our testing.
    bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        return;  // Connection closed or error
    }
    // Ensures the string is null-terminated to prevent overflow.
    buffer[bytes_read] = '\0';
    // copy buffer to a new string
    char *request = malloc(strlen(buffer) + 1);
    strcpy(request, buffer);

    // TODO: Parse the header and extract essential fields, e.g. file name
    // Hint: if the requested path is "/" (root), default to index.html
    // char file_name[] = "index.html";
    printf("request: %s\n", request);
    char* file_name = "index.html";
    char method[4], path[1024];
    sscanf(buffer, "%s %s", method, path);
    //if not index.html, replace file name w/ path
    if (strcmp(path, "/") != 0) {
        file_name = path + 1; 
    }
    printf("filename: %s\n", file_name);
    printf("path: %s\n", path);
    // TODO: Implement proxy and call the function under condition
    // specified in the spec

    // Check if path ends in .ts
    if (strcmp(path + strlen(path) - 3, ".ts") == 0) {
        printf("Client requested .ts file '.ts'\n");
        proxy_remote_file(app, client_socket, request);
    } else {
        serve_local_file(client_socket, file_name);
    }

    // Free dynamically allocated memory
    free(request);
}

void serve_local_file(int client_socket, const char *path) {
    // TODO: Properly implement serving of local files
    // The following code returns a dummy response for all requests
    // but it should give you a rough idea about what a proper response looks like
    // What you need to do 
    // (when the requested file exists):
    // * Open the requested file
    // * Build proper response headers (see details in the spec), and send them
    // * Also send file content
    // (When the requested file does not exist):
    // * Generate a correct response
    // char response[] = "HTTP/1.0 200 OK\r\n"
    //                   "Content-Type: text/plain; charset=UTF-8\r\n"
    //                   "Content-Length: 15\r\n"
    //                   "\r\n"
    //                   "Sample response";
    char filepath[1024];
    // Initialize filepath based on the received path
    if (strcmp(path, "/") == 0) {
        snprintf(filepath, sizeof(filepath), "./index.html");
    } else {
        snprintf(filepath, sizeof(filepath), "./%s", path);
    }
    printf("Attempting to serve: %s\n", filepath);
    
    // Determine content type based on file extension
    const char *ext = strrchr(path, '.'); // Find the last '.' in the path, to find content type
    char *content_type = "text/plain; charset=UTF-8"; //default
    
    if (ext) { // Check if there is an extension
        if (strcmp(ext, ".html") == 0) {
            content_type = "text/html; charset=UTF-8";
        } else if (strcmp(ext, ".txt") == 0) {
            content_type = "text/plain; charset=UTF-8";
        }else if (strcmp(ext, ".css") == 0) {
            content_type = "text/css";
        } else if (strcmp(ext, ".js") == 0) {
            content_type = "application/javascript";
        } else if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) {
            content_type = "image/jpeg";
        }
        else {
        content_type = "application/octet-stream"; // Default binary data handling
        }
    }

    struct stat file_stat;
    if (stat(filepath, &file_stat) < 0) {
        char *not_found_response = "HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        send(client_socket, not_found_response, strlen(not_found_response), 0);
        return;
    }
    //OPEN FILE (read only mode)
    int file_fd = open(filepath, O_RDONLY);
    if (file_fd < 0) {
        perror("Failed to open file");
        char *error_response = "HTTP/1.0 500 Internal Server Error can't read file\r\nContent-Length: 0\r\n\r\n";
        send(client_socket, error_response, strlen(error_response), 0);
        return;
    } else {
        printf("File opened successfully: %s\n", filepath);
    }

    // Read the file's content
    char file_content[4096];
    ssize_t read_bytes = read(file_fd, file_content, sizeof(file_content) - 1);
    if (read_bytes < 0) {
        perror("Failed to read file content");
        // Handle read error
        close(file_fd);
        char *error_response = "HTTP/1.0 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
        send(client_socket, error_response, strlen(error_response), 0);
        return;
    } else {
    printf("Read %zd bytes from file: %s\n", read_bytes, filepath);
    }
    //return
    // file_content[read_bytes] = '\0'; // Ensure null-termination

    // Prepare the HTTP response headers
    char response_header[1024];
    snprintf(response_header, sizeof(response_header), 
         "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nContent-Length: %zd\r\n\r\n", content_type, read_bytes);
    printf("Sending response for %s\nContent-Type: %s\nContent-Length: %zd\n", filepath, content_type, read_bytes);
    // Send the response headers
    send(client_socket, response_header, strlen(response_header), 0);
    // Send the file content
    send(client_socket, file_content, read_bytes, 0);

    close(file_fd);
}

void proxy_remote_file(struct server_app *app, int client_socket, const char *request) {
    // TODO: Implement proxy request and replace the following code
    // What's needed:
    // * Connect to remote server (app->remote_server/app->remote_port)
    // * Forward the original request to the remote server
    // * Pass the response from remote server back
    // Bonus:
    // * When connection to the remote server fail, properly generate
    // HTTP 502 "Bad Gateway" response

    //char response[] = "HTTP/1.0 501 Not Implemented\r\n\r\n";
    //send(client_socket, response, strlen(response), 0);

    // Create a socket for the backend server
    printf("Creating socket for backend server\n");

    int backend_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (backend_socket == -1) {
        perror("socket failed");

        // Send HTTP 502 Bad Gateway response
        char *bad_gateway_response = "HTTP/1.1 502 Bad Gateway\r\nContent-Length: 0\r\n\r\n";
        send(client_socket, bad_gateway_response, strlen(bad_gateway_response), 0);
    }

    // Set up the address of the backend server
    printf("Setting up address of the backend server\n");

    struct sockaddr_in backend_addr;
    memset(&backend_addr, 0, sizeof(backend_addr));
    backend_addr.sin_family = AF_INET;
    backend_addr.sin_addr.s_addr = inet_addr(app->remote_host);
    backend_addr.sin_port = htons(app->remote_port);

    printf("Backend server IP address: %s\n", app->remote_host);
    printf("Backend server port: %d\n", app->remote_port);

    // Connect to the backend server
    printf("Connecting to backend server\n");
    if (connect(backend_socket, (struct sockaddr *)&backend_addr, sizeof(backend_addr)) < 0) {
        perror("connection failed");
        // Send HTTP 502 Bad Gateway response
        char *bad_gateway_response = "HTTP/1.1 502 Bad Gateway\r\nContent-Length: 0\r\n\r\n";
        send(client_socket, bad_gateway_response, strlen(bad_gateway_response), 0);
        close(backend_socket);
        return;
    }

    printf("Connection to the backend server successful.\n");
    printf("Accepted connection from %s:%d\n", inet_ntoa(backend_addr.sin_addr), ntohs(backend_addr.sin_port));

    // Send the client's request to the backend server
    printf("Sending client's request to the backend server\n");
    send(backend_socket, request, strlen(request), 0);

    // Send response from the backend server back to the client
    printf("Sending response from the backend server back to the client\n");
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = recv(backend_socket, buffer, sizeof(buffer), 0)) > 0) {
        send(client_socket, buffer, bytes_read, 0);
    }

    // Close sockets
    printf("Closing sockets \n");
    close(backend_socket);
}