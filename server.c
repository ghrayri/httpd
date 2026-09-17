#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 8080

int main(void)
{
    int server_fd;

    /*
     * ============================================================
     * 1. CREATE TCP SOCKET
     * ============================================================
     *
     * AF_INET      -> IPv4
     * SOCK_STREAM  -> TCP
     * 0            -> protocol choisi automatiquement (TCP ici)
     */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1)
    {
        perror("socket");
        return EXIT_FAILURE;
    }

    printf("socket created: fd=%d\n", server_fd);


    /*
     * ============================================================
     * 2. CONFIGURE SERVER ADDRESS
     * ============================================================
     */

    struct sockaddr_in server_addr;

    /*
     * Initialise toute la structure à 0.
     * Cela évite d'avoir des valeurs indéfinies dans les champs.
     */
    memset(&server_addr, 0, sizeof(server_addr));

    /* IPv4 */
    server_addr.sin_family = AF_INET;

    /*
     * PORT doit être converti en ordre réseau.
     */
    server_addr.sin_port = htons(PORT);

    /*
     * INADDR_ANY = accepter les connexions sur toutes
     * les interfaces réseau disponibles.
     */
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);


    /*
     * ============================================================
     * 3. BIND SOCKET TO ADDRESS AND PORT
     * ============================================================
     */

    if (bind(server_fd,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) == -1)
    {
        perror("bind");
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("socket bound to port %d\n", PORT);


    /*
     * ============================================================
     * 4. LISTEN
     * ============================================================
     *
     * Le serveur commence maintenant à écouter les connexions
     * entrantes.
     *
     * 10 = taille du backlog
     */
    if (listen(server_fd, 10) == -1)
    {
        perror("listen");
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("server is listening on port %d\n", PORT);


    /*
     * ============================================================
     * 5. ACCEPT A CLIENT
     * ============================================================
     */

    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    /*
     * accept() attend ici jusqu'à ce qu'un client se connecte.
     *
     * server_fd -> socket du serveur
     * client_fd -> nouvelle socket dédiée à ce client
     */
    client_fd = accept(server_fd,
                       (struct sockaddr *)&client_addr,
                       &client_len);

    if (client_fd == -1)
    {
        perror("accept");
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("client connected\n");


    /*
     * ============================================================
     * 6. RECEIVE HTTP REQUEST
     * ============================================================
     */

    char buffer[4096];
    ssize_t bytes_received;

    /*
     * On garde 1 octet libre pour '\0'
     * afin de pouvoir traiter buffer comme une chaîne C.
     */
    bytes_received = recv(client_fd,
                          buffer,
                          sizeof(buffer) - 1,
                          0);

    if (bytes_received == -1)
    {
        perror("recv");
        close(client_fd);
        close(server_fd);
        return EXIT_FAILURE;
    }

    /*
     * recv() retourne le nombre exact d'octets reçus.
     *
     * On ajoute '\0' pour terminer la chaîne.
     */
    buffer[bytes_received] = '\0';

    printf("received %zd bytes:\n", bytes_received);
    printf("%s\n", buffer);


    /*
     * ============================================================
     * 7. PARSE HTTP REQUEST LINE
     * ============================================================
     *
     * Exemple :
     *
     * GET / HTTP/1.1
     *
     * On veut récupérer :
     *
     * method  = GET
     * target  = /
     * version = HTTP/1.1
     */

    char method[16];
    char target[1024];
    char version[16];

    if (sscanf(buffer,
               "%15s %1023s %15s",
               method,
               target,
               version) != 3)
    {
        printf("Invalid HTTP request line\n");
    }
    else
    {
        printf("Method: %s\n", method);
        printf("Target: %s\n", target);
        printf("Version: %s\n", version);
    }


    /*
     * ============================================================
     * 8. FIND END OF REQUEST LINE
     * ============================================================
     *
     * HTTP utilise :
     *
     * \r\n
     *
     * pour terminer une ligne.
     *
     * Exemple :
     *
     * GET / HTTP/1.1\r\n
     * Host: 127.0.0.1:8080\r\n
     */

    char *line_end;

    line_end = strstr(buffer, "\r\n");

    if (line_end == NULL)
    {
        printf("Invalid HTTP request\n");
    }
    else
    {
        printf("Request line ends here\n");


        /*
         * ========================================================
         * 9. FIND START OF HEADERS
         * ========================================================
         *
         * line_end pointe sur le '\r' de "\r\n".
         *
         * Donc :
         *
         * line_end + 2
         *
         * pointe sur le début de la première ligne de header.
         */

        char *headers_start;

        headers_start = line_end + 2;

        printf("Headers start here:\n");
        printf("%s", headers_start);


        /*
         * ========================================================
         * 10. FIND FIRST HEADER
         * ========================================================
         *
         * Exemple :
         *
         * Host: 127.0.0.1:8080
         *
         * ':' sépare :
         *
         * header name  -> Host
         * header value -> 127.0.0.1:8080
         */

        char *colon;

        colon = strchr(headers_start, ':');

        if (colon == NULL)
        {
            printf("Invalid header\n");
        }
        else
        {
            /*
             * Le nom du header se trouve entre :
             *
             * headers_start
             *       ↓
             *       Host: ...
             *           ↑
             *         colon
             *
             * %.*s permet d'afficher seulement un nombre
             * précis de caractères.
             */

            printf("Header name: %.*s\n",
                   (int)(colon - headers_start),
                   headers_start);


            /*
             * On cherche maintenant la fin de cette ligne de header.
             *
             * Exemple :
             *
             * Host: 127.0.0.1:8080\r\n
             *                       ↑
             */

            char *header_end;

            header_end = strstr(headers_start, "\r\n");

            if (header_end == NULL)
            {
                printf("Invalid header\n");
            }
            else
            {
                /*
                 * colon + 2 :
                 *
                 * colon pointe sur ':'
                 *
                 * colon + 1 -> espace
                 * colon + 2 -> début de la valeur
                 *
                 * On affiche uniquement jusqu'à header_end.
                 */

                printf("Header value: %.*s\n",
                       (int)(header_end - (colon + 2)),
                       colon + 2);
            }
        }
    }


    /*
     * ============================================================
     * 11. SEND HTTP RESPONSE
     * ============================================================
     *
     * Réponse HTTP :
     *
     * HTTP/1.1 200 OK
     * Content-Type: text/plain
     * Content-Length: 12
     * Connection: close
     *
     * Hello World!
     */

    const char *response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 12\r\n"
        "Connection: close\r\n"
        "\r\n"
        "Hello World!";


    ssize_t bytes_sent;

    bytes_sent = send(client_fd,
                      response,
                      strlen(response),
                      0);

    if (bytes_sent == -1)
    {
        perror("send");
        close(client_fd);
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("sent %zd bytes\n", bytes_sent);


    /*
     * ============================================================
     * 12. CLOSE CONNECTIONS
     * ============================================================
     *
     * client_fd -> connexion avec le client
     * server_fd -> socket du serveur
     */

    close(client_fd);
    close(server_fd);

    return EXIT_SUCCESS;
}

