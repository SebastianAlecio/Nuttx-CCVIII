#include <nuttx/config.h>  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>      
#include <errno.h>       
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>   
#include <sys/time.h>    

#define BUFLEN 1024
#define TIMEOUT_SEC 5 
#define DEFAULT_SERVER_PORT CONFIG_EXAMPLES_MY_CLIENT_PORT
#define DEFAULT_SERVER_IP   CONFIG_EXAMPLES_MY_CLIENT_TARGET_IP

void die(const char *s);
void run_tcp(const char *server_ip, int port);
void run_udp(const char *server_ip, int port);

int main(int argc, char *argv[])
{
    const char *protocol = "TCP";
    const char *server_ip_str = NULL;
    int port = DEFAULT_SERVER_PORT; 

    printf("Hola Cliente C (NuttX)\n");

    if (argc == 1)
      {
        uint32_t ip_value = DEFAULT_SERVER_IP;
        
        uint8_t *ip_bytes = (uint8_t *)&ip_value;
        static char ip_buffer[16];

        snprintf(ip_buffer, 16, "%d.%d.%d.%d", 
                 ip_bytes[3], ip_bytes[2], ip_bytes[1], ip_bytes[0]);
        
        server_ip_str = ip_buffer;
        
        printf("Usando valores por defecto de Kconfig: %s:%d\n", server_ip_str, port);
      }
    else
      {
        for (int i = 1; i < (argc - 1); i += 2)
          {
            if (strcmp(argv[i], "protocol") == 0)
              {
                if (strcmp(argv[i + 1], "UDP") == 0)
                  {
                    protocol = "UDP";
                  }
                else if (strcmp(argv[i + 1], "TCP") == 0)
                  {
                    protocol = "TCP";
                  }
                else
                  {
                    printf("Protocolo no válido. Usando TCP por defecto.\n");
                  }
              }
            else if (strcmp(argv[i], "port") == 0)
              {
                port = atoi(argv[i + 1]);
                if (port < 1 || port > 65535)
                  {
                    printf("Puerto fuera de rango. Usando puerto default %d.\n", DEFAULT_SERVER_PORT);
                    port = DEFAULT_SERVER_PORT;
                  }
              }
            else if (strcmp(argv[i], "server") == 0)
              {
                server_ip_str = argv[i + 1];
              }
          }
      }
      
    if (server_ip_str == NULL)
      {
        fprintf(stderr, "ERROR: Debe especificar la IP del servidor ('server <ip>').\n");
        return 1;
      }


    printf("Configuración Final: Protocolo=%s, Servidor=%s, Puerto=%d\n",
           protocol, server_ip_str, port);

    if (strcmp(protocol, "TCP") == 0)
      {
        run_tcp(server_ip_str, port);
      }
    else
      {
        run_udp(server_ip_str, port);
      }

    printf("Saliendo...\n");
    return 0;
}

void run_tcp(const char *server_ip, int port)
{
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFLEN];
    
    struct timeval tv;
    tv.tv_sec = TIMEOUT_SEC; // 5 segundos
    tv.tv_usec = 0;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
      {
        die("socket() falló");
      }

    memset(&server_addr, 0, sizeof(server_addr)); 
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_aton(server_ip, &server_addr.sin_addr) == 0)
      {
        fprintf(stderr, "ERROR: Dirección IP no válida '%s'\n", server_ip);
        close(sock);
        return;
      }

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
      {
        die("connect() falló");
      }
    
    printf("Conexión TCP exitosa\n");
    
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) < 0)
      {
        die("setsockopt(SO_RCVTIMEO) falló");
      }

    while (1)
      {
        printf("Ingrese la operacion a realizar: ");
        if (fgets(buffer, BUFLEN, stdin) == NULL)
          {
            break; 
          }
        buffer[strcspn(buffer, "\n")] = 0; 
        uint16_t len = (uint16_t)strlen(buffer);
        uint16_t net_len = htons(len); 
        
        /* Enviar primero la longitud en TCP*/
        if (write(sock, &net_len, sizeof(net_len)) < 0)
          {
            die("write() [longitud] falló");
          }
        
        if (write(sock, buffer, len) < 0)
          {
            die("write() [datos] falló");
          }
        
        printf("< client C/TCP: %s\n", buffer);
    
        uint16_t resp_len_net;
        ssize_t bytes_read = read(sock, &resp_len_net, sizeof(resp_len_net));
        
        if (bytes_read < 0)
          {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
              {
                printf("El servidor no responde (timeout)\n");
                continue;
              }
            die("read() [longitud resp] falló");
          }
        else if (bytes_read == 0)
          {
            printf("El servidor cerró la conexión.\n");
            break;
          }

        uint16_t resp_len = ntohs(resp_len_net); 

        if (resp_len > BUFLEN - 1)
          {
             printf("Respuesta del servidor demasiado larga (%d bytes)\n", resp_len);
             resp_len = BUFLEN - 1; 
          }
        
        bytes_read = read(sock, buffer, resp_len);
        if (bytes_read < 0)
          {
            die("read() [datos resp] falló");
          }

        buffer[bytes_read] = '\0'; 
        
        printf("> server C/TCP: \n%s\n", buffer); 
        if (strcmp(buffer, "EXIT") == 0)
          {
            break;
          }
      }

    close(sock);
}

void run_udp(const char *server_ip, int port)
{
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFLEN];
    socklen_t slen = sizeof(server_addr);

    struct timeval tv;
    tv.tv_sec = TIMEOUT_SEC; // 5 segundos
    tv.tv_usec = 0;

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
      {
        die("socket() falló");
      }
    
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) < 0)
      {
        die("setsockopt(SO_RCVTIMEO) falló");
      }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_aton(server_ip, &server_addr.sin_addr) == 0)
      {
        fprintf(stderr, "ERROR: Dirección IP no válida '%s'\n", server_ip);
        close(sock);
        return;
      }
    
    printf("Cliente UDP configurado. Enviando a %s:%d\n", server_ip, port);

    while (1)
      {
        printf("Ingrese la operación a realizar: ");
        if (fgets(buffer, BUFLEN, stdin) == NULL)
          {
            break; 
          }
        buffer[strcspn(buffer, "\n")] = 0;

        // enviardatos
        if (sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *)&server_addr, slen) < 0)
          {
            die("sendto() falló");
          }
        
        printf("< client C/UDP: %s\n", buffer);

        // recibir respuesta
        ssize_t bytes_recv = recvfrom(sock, buffer, BUFLEN - 1, 0, NULL, NULL);

        if (bytes_recv < 0)
          {
            // si el error fue un timeout
            if (errno == EWOULDBLOCK || errno == EAGAIN)
              {
                printf("El servidor no responde (timeout)\n");
                continue; 
              }
            else
              {
                die("recvfrom() falló");
              }
          }

        buffer[bytes_recv] = '\0'; 
        
        printf("> server C/UDP: \n%s\n", buffer); 

        // Salir si el *comando enviado* fue EXIT
        if (strcmp(buffer, "EXIT") == 0)
          {
            break;
          }
      }

    close(sock);
}

void die(const char *s)
{
    perror(s); 
    exit(1);
}

