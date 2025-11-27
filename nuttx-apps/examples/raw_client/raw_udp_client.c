#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define SERVER_PORT 3000
#define SERVER_IP   "127.0.0.1"
#define MY_PORT     54321
#define MAX_PAYLOAD_SIZE 1024

#define IP_TTL 255
#define IP_PROTO 17 
#define CHECKSUM 0

int main(int argc, FAR char *argv[])
{
  int sockfd;
  struct sockaddr_in server_addr;
  struct sockaddr_in client_addr;

  static char payload[MAX_PAYLOAD_SIZE];

  printf("Iniciando raw_client...\n");

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
    {
      printf("ERROR: No se pudo crear el socket UDP: %d\n", errno);
      return 1;
    }
  printf("Socket UDP creado.\n");

  memset(&client_addr, 0, sizeof(struct sockaddr_in));
  client_addr.sin_family      = AF_INET;
  client_addr.sin_port        = htons(MY_PORT);
  client_addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0)
    {
      printf("ERROR: bind() falló para el puerto %d: %d\n", MY_PORT, errno);
      close(sockfd);
      return 1;
    }

  /* Configurar la dirección del servidor (127.0.0.1:3000) */
  memset(&server_addr, 0, sizeof(struct sockaddr_in));
  server_addr.sin_family      = AF_INET;
  server_addr.sin_port        = htons(SERVER_PORT);
  if (inet_aton(SERVER_IP, &server_addr.sin_addr) == 0)
    {
      printf("ERROR: Dirección IP de servidor inválida.\n");
      close(sockfd);
      return 1;
    }

  /* Bucle principal para enviar mensajes */
  while (1)
    {
      printf("\nMensaje (escriba 'exit' para salir): ");
      if (fgets(payload, sizeof(payload), stdin) == NULL)
        {
          break; 
        }

      payload[strcspn(payload, "\n")] = 0;

      if (strcmp(payload, "exit") == 0)
        {
          break;
        }

      size_t payload_len = strlen(payload);
      if (payload_len == 0)
        {
          continue;
        }


      /* Imprimir el Header IP */
      printf("\n=============================================\n");
      printf("--- HEADER IP ---\n");
      printf("  IP Origen:    %s\n", "127.0.0.1");
      printf("  IP Destino:   %s\n", SERVER_IP);
      printf("  TTL:          %d\n", IP_TTL);
      printf("  Protocolo:    %d (UDP)\n", IP_PROTO);
      printf("  Checksum:     %d\n",CHECKSUM);

      /* 2. Imprimir el Header UDP */
      printf("\n--- HEADER UDP ---\n");
      printf("  Puerto Origen: %d\n", MY_PORT);
      printf("  Puerto Destino: %d\n", SERVER_PORT);
      printf("  Longitud:     %zu bytes (Header + Payload)\n", payload_len + 8);
      printf("  Checksum:     %d\n",CHECKSUM);
      printf("=============================================\n");

      printf("Enviando payload (%zu bytes) a %s:%d...\n",
             payload_len, SERVER_IP, SERVER_PORT);

      ssize_t nbytes = sendto(sockfd,
                              payload,   
                              payload_len,
                              0,
                              (struct sockaddr *)&server_addr,
                              sizeof(struct sockaddr_in));

      if (nbytes < 0)
        {
          printf("ERROR: sendto falló: %d\n", errno);
        }
      else
        {
          printf("¡Éxito! Se enviaron %zd bytes.\n", nbytes);
        }
    }

  printf("Cerrando socket.\n");
  close(sockfd);
  return 0;
}