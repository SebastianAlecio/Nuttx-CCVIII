#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define BUFLEN 1024
#define ECHO_PORT CONFIG_EXAMPLES_MY_SERVER_PORT 
#define TCP_BACKLOG 5

static int my_isdigit(char c);
static int my_isspace(char c);
static long my_basic_atoi(const char *s);
static void str_reverse(char *s);
static void my_basic_ltoa(long n, char *s);

static void die(const char *s);
static void process_operation(const char *operation, char *result_buffer, size_t buffer_len);
static void run_tcp_server(int port);
static void run_udp_server(int port);
static void handle_tcp_client(int client_sock);

int main(int argc, char *argv[])
{
    const char *protocol = "TCP";
    int port = ECHO_PORT;
    int i;
    printf("Hola Servidor (NuttX)\n"); 
    for (i = 1; i < argc; i += 2) {
        if (i + 1 >= argc) {
            fprintf(stderr, "ERROR: Argumento sin valor para '%s'.\n", argv[i]);
            return 1;
        }
        
        if (strcmp(argv[i], "protocol") == 0) {
            if (strcmp(argv[i + 1], "UDP") == 0) {
                protocol = "UDP";
            } else if (strcmp(argv[i + 1], "TCP") == 0) {
                protocol = "TCP";
            } else {
                printf("Protocolo no válido. Usando TCP por defecto.\n");
            }
        } else if (strcmp(argv[i], "port") == 0) {
            port = my_basic_atoi(argv[i + 1]); 
            if (port < 1 || port > 65535) {
                printf("Puerto fuera de rango. Usando puerto default %d.\n", ECHO_PORT);
                port = ECHO_PORT;
            }
        }
    }

    printf("Configuración: Protocolo=%s, Puerto=%d\n", protocol, port);
    if (strcmp(protocol, "TCP") == 0) {
        run_tcp_server(port);
    } else {
        run_udp_server(port);
    }

    printf("Saliendo del servidor...\n");
    return 0;
}


/* --- Implementaciones de Conversión de C Básicas --- */

static int my_isdigit(char c)
{
    return (c >= '0' && c <= '9');
}

static int my_isspace(char c)
{
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v');
}

static long my_basic_atoi(const char *s)
{
    long result = 0;
    int sign = 1;
    while (my_isspace(*s)) s++;
    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+') {
        s++;
    }
    while (my_isdigit(*s)) {
        result = (result * 10) + (*s - '0');
        s++;
    }
    return result * sign;
}

static void str_reverse(char *s)
{
    char *j;
    char c;
    j = s + strlen(s) - 1;
    while (s < j) {
        c = *s;
        *s++ = *j;
        *j-- = c;
    }
}

static void my_basic_ltoa(long n, char *s)
{
    int i = 0;
    long sign = n;
    if (n == 0) {
        s[i++] = '0';
        s[i] = '\0';
        return;
    }
    if (n < 0) {
        n = -n;
    }
    while (n > 0) {
        s[i++] = (n % 10) + '0';
        n /= 10;
    }
    if (sign < 0) {
        s[i++] = '-';
    }
    s[i] = '\0';
    str_reverse(s);
}


static void process_operation(const char *operation, char *result_buffer, size_t buffer_len)
{
    char temp_op[BUFLEN];
    char operator = 0;
    char *part1_str, *part2_str;
    long num1, num2, result; 
    int operator_index = -1;
    size_t len;
    result_buffer[0] = '\0';
    
    strncpy(temp_op, operation, BUFLEN - 1);
    temp_op[BUFLEN - 1] = '\0';
    len = strlen(temp_op);
    for (int i = len - 1; i > 0; i--) {
        char c = temp_op[i];
        if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%') {
            operator_index = i;
            operator = c;
            break;
        }
    }
    if (operator_index == -1) {
        num1 = my_basic_atoi(temp_op); 
        result = num1;
        goto format_result;
    }

    temp_op[operator_index] = '\0';
    part1_str = temp_op;
    part2_str = temp_op + operator_index + 1;

    while (my_isspace(*part1_str)) part1_str++;
    while (my_isspace(*part2_str)) part2_str++;

    if (*part1_str == '\0' || *part2_str == '\0') {
        goto invalid_format;
    }
    num1 = my_basic_atoi(part1_str); 
    num2 = my_basic_atoi(part2_str); 

    switch (operator) {
        case '+':
            result = num1 + num2;
            break;
        case '-':
            result = num1 - num2;
            break;
        case '*':
            result = num1 * num2;
            break;
        case '/':
            if (num2 == 0) goto division_by_zero; 
            result = num1 / num2;
            break;
        case '%':
            if (num2 == 0) goto division_by_zero;
            result = num1 % num2;
            break;
        default:
            goto invalid_operator;
    }

format_result:
    my_basic_ltoa(result, result_buffer); 
    return;

division_by_zero:
    strncpy(result_buffer, "Division por cero", buffer_len - 1);
    return;
invalid_format:
    strncpy(result_buffer, "Formato de operacion invalido", buffer_len - 1);
    return;
invalid_numbers:
    strncpy(result_buffer, "Numeros invalidos", buffer_len - 1);
    return;
invalid_operator:
    strncpy(result_buffer, "Operador no soportado", buffer_len - 1);
    return;
}

static void run_tcp_server(int port)
{
    int listen_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addrlen = sizeof(client_addr);
    int optval = 1;

    if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        die("socket() falló");
    }

    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        die("setsockopt(SO_REUSEADDR) falló");
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    
    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        die("bind() falló");
    }

    if (listen(listen_sock, TCP_BACKLOG) < 0) {
        die("listen() falló");
    }

    printf("Servidor TCP escuchando en INADDR_ANY:%d\n", port);

    while (1) {
        printf("Esperando conexión...\n");
        if ((client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &addrlen)) < 0) {
            if (errno == EINTR) continue;
            die("accept() falló");
        }
        
        handle_tcp_client(client_sock);
    }
    
    close(listen_sock);
}

static void handle_tcp_client(int client_sock)
{
    char buffer[BUFLEN];
    ssize_t recv_len;
    char result_buffer[BUFLEN]; 
    
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    
    getpeername(client_sock, (struct sockaddr *)&addr, &addrlen);
    printf("[IP: %s, Port: %d] Conexión exitosa. Esperando operación...\n",
           inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

    while ((recv_len = read(client_sock, buffer, BUFLEN - 1)) > 0)
    {
        buffer[recv_len] = '\0'; 
        char *p = strchr(buffer, '\n');
        if (p) *p = '\0';
        p = strchr(buffer, '\r');
        if (p) *p = '\0';
        
        printf("> Cliente dice: [%s]\n", buffer);
        process_operation(buffer, result_buffer, BUFLEN);
        
        if (write(client_sock, result_buffer, strlen(result_buffer)) < 0)
        {
            die("write() [datos resp] falló");
        }
        
        write(client_sock, "\n", 1); 
        
        printf("< Servidor: Respuesta enviada.\n");

        if (strcmp(buffer, "EXIT") == 0)
        {
            break;
        }
    }

    if (recv_len == 0)
    {
        printf("Servidor TCP: Conexión cerrada por el cliente.\n");
    }
    else if (recv_len < 0)
    {
        die("read() falló");
    }

    close(client_sock);
}

static void run_udp_server(int port)
{
    int sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addrlen = sizeof(client_addr);
    char buffer[BUFLEN];
    int optval = 1;
    char result_buffer[BUFLEN];

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        die("socket() falló");
    }
    
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        die("setsockopt(SO_REUSEADDR) falló");
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        die("bind() falló");
    }
    
    printf("Servidor UDP escuchando en INADDR_ANY:%d. Esperando mensaje...\n", port);

    while (1) {
        ssize_t bytes_recv;
        
        bytes_recv = recvfrom(sock, buffer, BUFLEN - 1, 0, 
                              (struct sockaddr *)&client_addr, &addrlen);

        if (bytes_recv < 0) {
            die("recvfrom() falló");
        }
        
        buffer[bytes_recv] = '\0';
      
        char *p = strchr(buffer, '\n');
        if (p) *p = '\0';
        p = strchr(buffer, '\r');
        if (p) *p = '\0';
        
        printf("> Cliente [%s:%d] dice: [%s]\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buffer);
      
        process_operation(buffer, result_buffer, BUFLEN);
        
        if (sendto(sock, result_buffer, strlen(result_buffer), 0, (struct sockaddr *)&client_addr, addrlen) < 0) {
            die("sendto() falló");
        }
        
        printf("< Servidor: Respuesta enviada.\n");
        
        if (strcmp(buffer, "EXIT") == 0) {
            break;
        }
    }
    
    close(sock);
}

static void die(const char *s)
{
    printf("FATAL: %s. errno=%d\n", s, errno); 
    exit(1);
}


