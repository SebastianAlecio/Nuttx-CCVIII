#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip.h>  
#include <netinet/udp.h> 
#include <netinet/in.h>  

#define BUFLEN 1024

static void parse_arguments(int argc, char *argv[], char **server_ip, int *server_port, char **my_ip);
static void die(const char *s);

int main(int argc, char *argv[])
{
  int sock;
  char *server_ip = CONFIG_EXAMPLES_MY_CLIENT_RAW_TARGET_IP;
  int server_port = CONFIG_EXAMPLES_MY_CLIENT_RAW_PORT;
  char *my_ip = "127.0.0.1";
  int my_port = 65000;   

  char packet[BUFLEN];    
  char recv_buffer[BUFLEN]; 
  char message[BUFLEN];     

  /* Direcciones */
  struct sockaddr_in dest_addr, recv_addr;
  socklen_t addrlen = sizeof(recv_addr);
  ssize_t sent_len;

  printf("Hola Cliente RAW\n");
  parse_arguments(argc, argv, &server_ip, &server_port, &my_ip);

  printf("Configuración Final: Servidor=%s:%d, Mi IP=%s\n",
         server_ip, server_port, my_ip);

  /* Crear el Raw Socket */
  if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP)) < 0)
    {
      die("Error: socket() falló");
    }

  /* Configurar la dirección de destino Servidor */
  memset(&dest_addr, 0, sizeof(dest_addr));
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(server_port);
  if (inet_aton(server_ip, &dest_addr.sin_addr) == 0)
    {
      die("Error: IP de servidor inválida");
    }

  printf("Cliente RAW configurado. Listo para enviar.\n");

  while (1)
    {
      ssize_t recv_len;
      struct iphdr *ip_resp;
      unsigned short ip_hdr_len;
      struct udphdr *udp_resp;
      char *data_resp;
      int data_len;

      
      printf("Ingrese la operación (o 'EXIT'): ");
      if (fgets(message, BUFLEN, stdin) == NULL)
        {
          break; 
        }
      message[strcspn(message, "\n")] = 0;

      if (strcmp(message, "EXIT") == 0)
        {
          break;
        }

      /* Preparar el buffer del paquete */
      memset(packet, 0, BUFLEN);

      struct udphdr *udph = (struct udphdr *)packet;
      char *data = packet + sizeof(struct udphdr);

      strcpy(data, message);

      udph->uh_sport = htons(my_port); /* Puerto Origen */
      udph->uh_dport = htons(server_port); /* Puerto Destino */
      udph->uh_ulen = htons(sizeof(struct udphdr) + strlen(data)); /* Longitud UDP */
      udph->uh_sum = 0; /* Checksum */

      sent_len = sendto(sock, packet, sizeof(struct udphdr) + strlen(data), 0,
                        (struct sockaddr *)&dest_addr, sizeof(dest_addr));

      if (sent_len < 0)
        {
          die("Error: sendto() falló");
        }

      printf("< RAW/UDP: Enviados %d bytes a %s:%d\n",
             (int)sent_len, server_ip, server_port);

      printf("...esperando respuesta...\n");

      while (1)
        {
          recv_len = recvfrom(sock, recv_buffer, BUFLEN, 0,
                              (struct sockaddr *)&recv_addr, &addrlen);

          if (recv_len <= 0)
            {
              printf("recvfrom() falló o conexión cerrada\n");
              break;
            }

          ip_resp = (struct iphdr *)recv_buffer;

          if (ip_resp->protocol != IPPROTO_UDP)
            {
              continue;
            }

          if (ip_resp->saddr != dest_addr.sin_addr.s_addr)
            {
              continue;
            }

          ip_hdr_len = ip_resp->ihl * 4;
          udp_resp = (struct udphdr *)(recv_buffer + ip_hdr_len);

          if (udp_resp->uh_dport != htons(my_port))
            {
              continue;
            }

          data_resp = recv_buffer + ip_hdr_len + sizeof(struct udphdr);
          data_len = ntohs(udp_resp->uh_ulen) - sizeof(struct udphdr);

          if (data_len < BUFLEN - (ip_hdr_len + sizeof(struct udphdr)))
            {
              data_resp[data_len] = '\0';
            }
          else
            {
              data_resp[BUFLEN - (ip_hdr_len + sizeof(struct udphdr)) - 1] = '\0';
            }

          printf("> Servidor dice: %s\n", data_resp);
          break;
        }
    }

  printf("Cerrando socket RAW...\n");
  close(sock);
  return 0;
}

static void die(const char *s)
{
  fprintf(stderr, "FATAL: %s. errno=%d\n", s, errno);
  exit(1);
}

static void parse_arguments(int argc, char *argv[],
                            char **server_ip, int *server_port,
                            char **my_ip)
{
  int i;
  for (i = 1; i < argc; i++)
    {
      if (strcmp(argv[i], "server") == 0 && i + 1 < argc)
        {
          *server_ip = argv[++i];
        }
      else if (strcmp(argv[i], "port") == 0 && i + 1 < argc)
        {
          *server_port = atoi(argv[++i]);
        }
      else if (strcmp(argv[i], "myip") == 0 && i + 1 < argc)
        {
          *my_ip = argv[++i];
        }
      else
        {
          fprintf(stderr, "Argumento no reconocido: %s\n", argv[i]);
          fprintf(stderr, "Uso: my_client_raw [server <ip>] [port <num>] [myip <ip>]\n");
          exit(1);
        }
    }
}