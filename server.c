#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <json-c/json.h>
#include <time.h>

#define PORT 8080
#define BUFF_SIZE 4096

// Function to read the content of a file into a buffer
char* read_file(const char* filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Could not open file");
        return NULL;
    }

    // Go to the end of the file to determine its size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    // Allocate memory to hold the file content
    char *file_content = malloc(file_size + 1); // +1 for the null terminator
    if (file_content == NULL) {
        perror("Memory allocation failed");
        fclose(file);
        return NULL;
    }

    // Read the file into the buffer
    fread(file_content, 1, file_size, file);
    file_content[file_size] = '\0'; // Null terminate the string

    fclose(file);
    return file_content;
}

void handle_request(int client_sock, struct sockaddr_in client_addr) {
    char buffer[BUFF_SIZE];
    int received_bytes;

    while (1) {
        received_bytes = recv(client_sock, buffer, BUFF_SIZE - 1, 0); // Leave space for null terminator

        if (received_bytes < 0) {
            perror("Error receiving data");
            break;
        } else if (received_bytes == 0) {
            // Client closed the connection
            printf("Client disconnected.\n");
            break;
        }

        // Add a null terminator to avoid undefined behavior
        buffer[received_bytes] = '\0';
        time_t start_time = time(NULL);

        // Handle the request as a string
        if (strncmp(buffer, "GET / ", 6) == 0) {
            // Serve the HTML file
            char *html_content = read_file("index.html");  // Read the HTML file

            if (html_content != NULL) {
                char response[BUFF_SIZE];
                snprintf(response, sizeof(response), "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n%s", html_content);
                send(client_sock, response, strlen(response), 0);  // Send the HTML content
                free(html_content);  // Free the allocated memory for the file content
            } else {
                // In case of error reading the file, send a 500 Internal Server Error
                char error_response[] = "HTTP/1.1 500 Internal Server Error\r\n\r\nCould not load the page.";
                send(client_sock, error_response, sizeof(error_response) - 1, 0);
            }
        } else if (strncmp(buffer, "GET /style.css", 14) == 0) {
            // Serve the CSS file
            char *css_content = read_file("style.css");
            if (css_content != NULL) {
                char response[BUFF_SIZE];
                snprintf(response, sizeof(response), "HTTP/1.1 200 OK\r\nContent-Type: text/css\r\n\r\n%s", css_content);
                send(client_sock, response, strlen(response), 0);  // Send the CSS content
                free(css_content);  // Free the allocated memory for the file content
            } else {
                char error_response[] = "HTTP/1.1 500 Internal Server Error\r\n\r\nCould not load the stylesheet.";
                send(client_sock, error_response, sizeof(error_response) - 1, 0);
            }
        } else if (strncmp(buffer, "GET /script.js", 14) == 0) {
            // Serve the JavaScript file
            char *js_content = read_file("script.js");
            if (js_content != NULL) {
                char response[BUFF_SIZE];
                snprintf(response, sizeof(response), "HTTP/1.1 200 OK\r\nContent-Type: application/javascript\r\n\r\n%s", js_content);
                send(client_sock, response, strlen(response), 0);  // Send the JS content
                free(js_content);  // Free the allocated memory for the file content
            } else {
                char error_response[] = "HTTP/1.1 500 Internal Server Error\r\n\r\nCould not load the script.";
                send(client_sock, error_response, sizeof(error_response) - 1, 0);
            }
        } else if (strncmp(buffer, "GET /favicon.ico", 16) == 0) {
            // Handle favicon request
            char response[] = "HTTP/1.1 204 No Content\r\n\r\n";
            send(client_sock, response, sizeof(response) - 1, 0);
            continue;
        } else if (strncmp(buffer, "POST /send ", 11) == 0) {
            
            // Extract message from body
            char *message = strstr(buffer, "\r\n\r\n");
            if (message != NULL) {
                message += 4; // Move past the headers to get to the body

                // Parse JSON data
                struct json_object *parsed_json;
                parsed_json = json_tokener_parse(message);
                if (parsed_json == NULL) {
                    printf("Invalid JSON format.\n");
                    char error_response[] = "HTTP/1.1 400 Bad Request\r\n\r\nInvalid JSON format.";
                    send(client_sock, error_response, sizeof(error_response) - 1, 0);
                    continue;
                }

                // Check for message or choice
                struct json_object *msg_obj, *choice_obj;
                if (json_object_object_get_ex(parsed_json, "message", &msg_obj)) {
                    const char *msg_str = json_object_get_string(msg_obj);
                    printf("[From %s:%d]: %s\n",inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), msg_str);

                    // Create response with the received message
                    char response[BUFF_SIZE];
                    snprintf(response, sizeof(response), "HTTP/1.1 200 OK\r\n\r\n[From %s:%d]: %s",
                            inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), msg_str);
                    send(client_sock, response, strlen(response), 0);
                } else if (json_object_object_get_ex(parsed_json, "choice", &choice_obj)) {
                    const char *choice_str = json_object_get_string(choice_obj);
                    printf("[From %s:%d] choice: %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), choice_str);

                    // Create response with the received choice
                    char response[BUFF_SIZE];
                    snprintf(response, sizeof(response), "HTTP/1.1 200 OK\r\n\r\n[From %s:%d] choice: %s", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), choice_str);
                    send(client_sock, response, strlen(response), 0);
                } else {
                    printf("Invalid data format.\n");
                    char error_response[] = "HTTP/1.1 400 Bad Request\r\n\r\nInvalid data format.";
                    send(client_sock, error_response, sizeof(error_response) - 1, 0);
                }

                // Free the JSON object
                json_object_put(parsed_json);
            } else {
                printf("Invalid POST request format.\n");
                char error_response[] = "HTTP/1.1 400 Bad Request\r\n\r\nInvalid POST request format.";
                send(client_sock, error_response, sizeof(error_response) - 1, 0);
            }
        }

        // Log response time
        time_t end_time = time(NULL);
        // printf("Response sent. Time taken to handle request: %ld seconds\n", end_time - start_time);
    }

    close(client_sock);
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_sock, 5);

    printf("Server listening on port %d\n", PORT);
    while (1) {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size);
        handle_request(client_sock, client_addr);
    }

    close(server_sock);
    return 0;
}