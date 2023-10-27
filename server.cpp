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

#include <string>
#include <regex>
#include <fstream>
#include <iostream>

/**
 * Project 1 starter code
 * All parts needed to be changed/added are marked with TODO
 */

#define BUFFER_SIZE 1024
#define DEFAULT_SERVER_PORT 8081
#define DEFAULT_REMOTE_HOST "131.179.176.34"
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
void serve_local_file(int client_socket, const std::string path);
void proxy_remote_file(struct server_app *app, int client_socket, const std::string path);

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
        }
        
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

    buffer[bytes_read] = '\0';
    // copy buffer to a new string
    char *request = (char*)malloc(strlen(buffer) + 1);
    strcpy(request, buffer);

    // printf("START OF REQUEST\n");
    // printf(request);
    // printf("END OF REQUEST\n");

    // TODO: Parse the header and extract essential fields, e.g. file name
    // Hint: if the requested path is "/" (root), default to index.html

    std::string str_request(request);

    std::string::size_type path_start = str_request.find("GET /");
    std::string::size_type path_end = str_request.find(" HTTP");
    std::string header_path = str_request.substr(path_start + 5, path_end - 5);

    // header_path = 
    // header_filename = 
    // filepath file extension

    std::string file_name = "";
    if ( header_path.compare("/") == 0)
        file_name = "index.html";
    else
        file_name = header_path.substr(0, header_path.size());

    // printf("Header file: %s\n", header_path.c_str());

    // TODO: Implement proxy and call the function under condition
    // specified in the spec
    std::string::size_type filetype_start = file_name.find(".");
    std::string file_type = file_name.substr(filetype_start + 1, file_name.size());

    if (file_type.compare( "ts" ) == 0) {
        proxy_remote_file(app, client_socket, file_name);
    } else {
    serve_local_file(client_socket, file_name);
    }
}

void serve_local_file(int client_socket, const std::string path) {
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

    //adjust for files named with space
    std::string const tempPath = std::regex_replace( path, std::regex( "\\%20" ), " " );
    std::string const encodedPath = std::regex_replace( tempPath, std::regex( "\\%25" ), "%" );

    std::ifstream path_file(encodedPath, std::ios::binary | std::ios::ate);

    if (!path_file.is_open()) {
        std::cerr << "Error: Could not open file: " << encodedPath << std::endl;
        // THESE LINES ARE  A QUICK FIX THAT GET US THE FILE NOT FOUND EXTRA CREDIT
        // THEY MAKE US FAIL VIDEO CHUNK 1 REVERSE PROXY HTTP HEADER, BUT I THINK WE NEED TO FIX THAT ANYWAY
        char response[] = "HTTP/1.0 404 Not Found\r\n\r\n";
        send(client_socket, response, strlen(response), 0);
        return;
        // END OF QUICK FIX LINES
    }

    path_file.seekg(0, std::ios::end);
    std::size_t file_size = path_file.tellg();
    path_file.seekg(0, std::ios::beg);

    std::string::size_type filetype_start = path.find(".");
    std::string file_type = path.substr(filetype_start + 1, path.size());
    // printf("%s", file_type.c_str());

    // char char_file_contents[file_contents.size() + 1];

    // for (int x = 0; x < sizeof(char_file_contents); x++) { 
    //     char_file_contents[x] = file_contents[x]; 
    // } 

    if ( file_type.compare( "txt" ) == 0 )
        file_type = "text/plain";
    else if ( file_type.compare( "jpeg" ) == 0 | file_type.compare( "jpg" ) == 0 )
        file_type = "image/jpeg";
    else if ( file_type.compare( "html" ) == 0 )
        file_type = "text/html";
    else
        file_type = "application/octet-stream";

    // char response_header[] = "HTTP/1.0 200 OK\r\n"
    //                   "Content-Type: text/plain; charset=UTF-8\r\n"
    //                   "Content-Length: \r\n"
    //                   "\r\n";

    // char response[strlen(response_header) + sizeof file_contents.length() + strlen(char_file_contents) + 1];
    // char response[10000];

    std::string status_code = "200 OK";
    std::string status_line = "HTTP/1.1 " + status_code + "\r\n";
    std::string content_length_str = "Content-Length: " + std::to_string(file_size) + "\r\n";
    std::string content_type_str = "Content-Type: " + file_type + "\r\n";
    
    std::string response_headers = status_line + content_length_str + content_type_str + "\r\n";;

    printf(response_headers.c_str());  

    send(client_socket, response_headers.c_str(), response_headers.length(), 0);

    char buffer[BUFFER_SIZE];
    while (!path_file.eof()) {
        path_file.read(buffer, sizeof(buffer));
        send(client_socket, buffer, path_file.gcount(), 0);
    }

    path_file.close();
}

void proxy_remote_file(struct server_app *app, int client_socket, const std::string request) {
    // TODO: Implement proxy request and replace the following code
    
    // What's needed:
    // * Connect to remote server (app->remote_server/app->remote_port)

    int server_socket, remote_socket;
    struct sockaddr_in server_addr, client_addr, remote_addr;

    // Step 1: Open socket()
    server_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    remote_addr.sin_family = AF_INET;
    remote_addr.sin_addr.s_addr = ((in_addr_t)(size_t) app->remote_host);
    remote_addr.sin_port = htons(app->remote_port);
    memset(remote_addr.sin_zero, '\0', sizeof remote_addr.sin_zero);

    // Step 2: connect()
    if(connect(server_socket, (struct sockaddr*) &remote_addr, sizeof(struct sockaddr)) == -1) {
        // perror (“connect”);
        printf("i frew up :(");
        exit (1);
    }
    printf("yeah we connected");
    
    // * Forward the original request to the remote server

    char buffer[BUFFER_SIZE];
    strcpy(buffer, request);
    write(server_socket, buffer, sizeof buffer);
    // char buffer[strlen(request)];
    // char currbyte*;
    // while (&currbyte != '\0') {
    //     path_file.read(buffer, sizeof(buffer));
    //     send(client_socket, buffer, path_file.gcount(), 0);
    // }
    read(server_socket, buffer, sizeof buffer);
    close(server_socket);

    // * Pass the response from remote server back
    // Bonus:
    // * When connection to the remote server fail, properly generate
    // HTTP 502 "Bad Gateway" response

    char response[] = "HTTP/1.0 501 Not Implemented\r\n\r\n";
    send(client_socket, response, strlen(response), 0);
}