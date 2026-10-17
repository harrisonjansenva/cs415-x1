#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_PORT "80"
#define DEFAULT_PATH "/"
#define RECV_CHUNK_SIZE 16384

typedef struct
{
   char *host;
   char *port;
   char *path;
   char *host_header;
} ParsedURL;

typedef struct
{
   char *data;
   size_t len;
   size_t cap;
} ByteBuffer;

static void byte_buffer_init(ByteBuffer *buf)
{
   buf->data = NULL;
   buf->len = 0;
   buf->cap = 0;
}

static int byte_buffer_append(ByteBuffer *buf, const char *chunk, size_t size)
{
   if (size == 0)
      return 0;
   if (buf->len + size + 1 > buf->cap)
   {
      size_t new_cap = buf->cap ? buf->cap * 2 : 32768;
      while (buf->len + size + 1 > new_cap)
      {
         new_cap *= 2;
      }
      char *new_data = realloc(buf->data, new_cap);
      if (!new_data)
      {
         perror("realloc");
         return -1;
      }
      buf->data = new_data;
      buf->cap = new_cap;
   }
   memcpy(buf->data + buf->len, chunk, size);
   buf->len += size;
   buf->data[buf->len] = '\0';
   return 0;
}

static void byte_buffer_free(ByteBuffer *buf)
{
   if (buf->data)
   {
      free(buf->data);
      buf->data = NULL;
   }
   buf->len = 0;
   buf->cap = 0;
}

static double timespec_diff_ms(const struct timespec *start, const struct timespec *end)
{
   double sec = (double)(end->tv_sec - start->tv_sec);
   double nsec = (double)(end->tv_nsec - start->tv_nsec);
   return sec * 1000.0 + nsec / 1000000.0;
}

static void print_usage(const char *prog_name)
{
   fprintf(stderr, "Usage: %s [-4 | -6] [-n trials] [-o file] http://host[:port][/path]\n",
           prog_name);
}

static int parse_url(const char *url_str, ParsedURL *parsed)
{
   memset(parsed, 0, sizeof(*parsed));

   // Reject non-http schemes explicitly
   if (strncasecmp(url_str, "https://", 8) == 0)
   {
      fprintf(stderr, "Error: https is not supported, only plain http:// URLs are accepted: %s\n",
              url_str);
      return -1;
   }
   if (strncasecmp(url_str, "http://", 7) != 0)
   {
      fprintf(stderr, "Error: Invalid URL scheme. Only http:// URLs are accepted: %s\n", url_str);
      return -1;
   }

   const char *p = url_str + 7;
   if (*p == '\0')
   {
      fprintf(stderr, "Error: Malformed URL, missing host: %s\n", url_str);
      return -1;
   }

   const char *host_start = p;
   const char *host_end = NULL;
   const char *port_start = NULL;
   const char *port_end = NULL;
   const char *path_start = NULL;

   // Handle IPv6 bracketed address: http://[2001:db8::1]:80/path
   if (*p == '[')
   {
      host_start = p + 1;
      const char *closing = strchr(host_start, ']');
      if (!closing)
      {
         fprintf(stderr, "Error: Malformed IPv6 URL, missing closing bracket ']': %s\n", url_str);
         return -1;
      }
      host_end = closing;
      p = closing + 1;
      if (*p == ':')
      {
         port_start = p + 1;
         p = port_start;
         while (*p && *p != '/')
            p++;
         port_end = p;
      }
      if (*p == '/')
      {
         path_start = p;
      }
      else if (*p != '\0')
      {
         fprintf(stderr, "Error: Malformed URL after host: %s\n", url_str);
         return -1;
      }
   }
   else
   {
      // Standard hostname or IPv4
      while (*p && *p != ':' && *p != '/')
      {
         p++;
      }
      host_end = p;
      if (host_end == host_start)
      {
         fprintf(stderr, "Error: Malformed URL, empty host: %s\n", url_str);
         return -1;
      }

      if (*p == ':')
      {
         port_start = p + 1;
         p = port_start;
         while (*p && *p != '/')
            p++;
         port_end = p;
      }
      if (*p == '/')
      {
         path_start = p;
      }
      else if (*p != '\0')
      {
         fprintf(stderr, "Error: Malformed URL after host: %s\n", url_str);
         return -1;
      }
   }

   // Allocate and store host
   size_t host_len = (size_t)(host_end - host_start);
   parsed->host = malloc(host_len + 1);
   if (!parsed->host)
   {
      perror("malloc");
      return -1;
   }
   memcpy(parsed->host, host_start, host_len);
   parsed->host[host_len] = '\0';

   // Allocate and store port
   if (port_start && port_end > port_start)
   {
      size_t port_len = (size_t)(port_end - port_start);
      parsed->port = malloc(port_len + 1);
      if (!parsed->port)
      {
         perror("malloc");
         free(parsed->host);
         return -1;
      }
      memcpy(parsed->port, port_start, port_len);
      parsed->port[port_len] = '\0';

      // Validate numeric port
      char *endptr = NULL;
      long port_val = strtol(parsed->port, &endptr, 10);
      if (*endptr != '\0' || port_val < 1 || port_val > 65535)
      {
         fprintf(stderr, "Error: Invalid port number '%s' in URL\n", parsed->port);
         free(parsed->host);
         free(parsed->port);
         return -1;
      }
   }
   else
   {
      parsed->port = strdup(DEFAULT_PORT);
   }

   // Allocate and store path
   if (path_start && *path_start != '\0')
   {
      parsed->path = strdup(path_start);
   }
   else
   {
      parsed->path = strdup(DEFAULT_PATH);
   }

   // Construct Host header value: host[:port]
   if (strcmp(parsed->port, "80") == 0)
   {
      parsed->host_header = strdup(parsed->host);
   }
   else
   {
      size_t hh_len = strlen(parsed->host) + strlen(parsed->port) + 2;
      parsed->host_header = malloc(hh_len);
      if (parsed->host_header)
      {
         snprintf(parsed->host_header, hh_len, "%s:%s", parsed->host, parsed->port);
      }
   }

   return 0;
}

static void free_parsed_url(ParsedURL *parsed)
{
   if (parsed->host)
      free(parsed->host);
   if (parsed->port)
      free(parsed->port);
   if (parsed->path)
      free(parsed->path);
   if (parsed->host_header)
      free(parsed->host_header);
   memset(parsed, 0, sizeof(*parsed));
}

static void format_sockaddr(const struct sockaddr *sa, char *out_str, size_t out_len)
{
   char ip_str[INET6_ADDRSTRLEN] = {0};
   uint16_t port = 0;

   if (sa->sa_family == AF_INET)
   {
      const struct sockaddr_in *sin = (const struct sockaddr_in *)sa;
      inet_ntop(AF_INET, &sin->sin_addr, ip_str, sizeof(ip_str));
      port = ntohs(sin->sin_port);
      snprintf(out_str, out_len, "%s:%u", ip_str, port);
   }
   else if (sa->sa_family == AF_INET6)
   {
      const struct sockaddr_in6 *sin6 = (const struct sockaddr_in6 *)sa;
      inet_ntop(AF_INET6, &sin6->sin6_addr, ip_str, sizeof(ip_str));
      port = ntohs(sin6->sin6_port);
      snprintf(out_str, out_len, "[%s]:%u", ip_str, port);
   }
   else
   {
      snprintf(out_str, out_len, "unknown");
   }
}

static int compare_doubles(const void *a, const void *b)
{
   double da = *(const double *)a;
   double db = *(const double *)b;
   if (da < db)
      return -1;
   if (da > db)
      return 1;
   return 0;
}

int main(int argc, char *argv[])
{
   int opt;
   int family = AF_UNSPEC;
   int trials = 1;
   const char *output_file = NULL;

   opterr = 0; // custom error reporting
   while ((opt = getopt(argc, argv, "46n:o:")) != -1)
   {
      switch (opt)
      {
      case '4':
         if (family == AF_INET6)
         {
            fprintf(stderr, "Error: Cannot specify both -4 and -6\n");
            return 1;
         }
         family = AF_INET;
         break;
      case '6':
         if (family == AF_INET)
         {
            fprintf(stderr, "Error: Cannot specify both -4 and -6\n");
            return 1;
         }
         family = AF_INET6;
         break;
      case 'n':
      {
         char *endptr = NULL;
         trials = (int)strtol(optarg, &endptr, 10);
         if (*endptr != '\0' || trials <= 0)
         {
            fprintf(stderr, "Error: Invalid number of trials: '%s'\n", optarg);
            return 1;
         }
         break;
      }
      case 'o':
         output_file = optarg;
         break;
      default:
         fprintf(stderr, "Error: Unknown option '-%c'\n", optopt ? optopt : '?');
         print_usage(argv[0]);
         return 1;
      }
   }

   if (optind >= argc)
   {
      fprintf(stderr, "Error: Missing URL argument\n");
      print_usage(argv[0]);
      return 1;
   }
   if (optind + 1 < argc)
   {
      fprintf(stderr, "Error: Extraneous argument '%s'\n", argv[optind + 1]);
      print_usage(argv[0]);
      return 1;
   }

   const char *raw_url = argv[optind];
   ParsedURL parsed_url;
   if (parse_url(raw_url, &parsed_url) != 0)
   {
      return 1;
   }

   double *connect_times = malloc(sizeof(double) * (size_t)trials);
   if (!connect_times)
   {
      perror("malloc");
      free_parsed_url(&parsed_url);
      return 1;
   }

   for (int t = 1; t <= trials; t++)
   {
      printf("trial %d  %s\n", t, raw_url);

      // 1. DNS Resolution
      struct timespec t_dns_start, t_dns_end;
      struct addrinfo hints;
      struct addrinfo *res = NULL;
      memset(&hints, 0, sizeof(hints));
      hints.ai_family = family;
      hints.ai_socktype = SOCK_STREAM;

      clock_gettime(CLOCK_MONOTONIC, &t_dns_start);
      int gai_err = getaddrinfo(parsed_url.host, parsed_url.port, &hints, &res);
      clock_gettime(CLOCK_MONOTONIC, &t_dns_end);

      if (gai_err != 0 || !res)
      {
         fprintf(stderr, "Error: getaddrinfo failed for %s:%s: %s\n", parsed_url.host,
                 parsed_url.port, gai_strerror(gai_err));
         free(connect_times);
         free_parsed_url(&parsed_url);
         return 2;
      }

      double dns_time_ms = timespec_diff_ms(&t_dns_start, &t_dns_end);

      char remote_ip_port[INET6_ADDRSTRLEN + 16];
      format_sockaddr(res->ai_addr, remote_ip_port, sizeof(remote_ip_port));
      const char *family_name = (res->ai_family == AF_INET) ? "IPv4" : "IPv6";

      printf("  %-10s %7.1f ms  %s -> %s (%s)\n", "dns", dns_time_ms, parsed_url.host,
             remote_ip_port, family_name);

      // 2. Open TCP Socket and Connect (time ONLY connect call)
      int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
      if (sockfd < 0)
      {
         perror("socket");
         freeaddrinfo(res);
         free(connect_times);
         free_parsed_url(&parsed_url);
         return 3;
      }

      struct timespec t_conn_start, t_conn_end;
      clock_gettime(CLOCK_MONOTONIC, &t_conn_start);
      int conn_res = connect(sockfd, res->ai_addr, res->ai_addrlen);
      clock_gettime(CLOCK_MONOTONIC, &t_conn_end);

      if (conn_res < 0)
      {
         perror("connect");
         close(sockfd);
         freeaddrinfo(res);
         free(connect_times);
         free_parsed_url(&parsed_url);
         return 4;
      }

      double connect_time_ms = timespec_diff_ms(&t_conn_start, &t_conn_end);
      connect_times[t - 1] = connect_time_ms;

      // Query 4-tuple
      struct sockaddr_storage local_ss;
      socklen_t local_len = sizeof(local_ss);
      char local_ip_port[INET6_ADDRSTRLEN + 16] = {0};
      if (getsockname(sockfd, (struct sockaddr *)&local_ss, &local_len) == 0)
      {
         format_sockaddr((struct sockaddr *)&local_ss, local_ip_port, sizeof(local_ip_port));
      }
      else
      {
         snprintf(local_ip_port, sizeof(local_ip_port), "unknown");
      }

      printf("  %-10s %7.1f ms  %s -> %s\n", "connect", connect_time_ms, local_ip_port,
             remote_ip_port);

      // 3. Send HTTP/1.1 GET Request
      char request[4096];
      int req_len = snprintf(request, sizeof(request),
                             "GET %s HTTP/1.1\r\n"
                             "Host: %s\r\n"
                             "Connection: close\r\n"
                             "User-Agent: pageload/1.0\r\n"
                             "\r\n",
                             parsed_url.path, parsed_url.host_header);

      if (req_len < 0 || (size_t)req_len >= sizeof(request))
      {
         fprintf(stderr, "Error: Request buffer overflow\n");
         close(sockfd);
         freeaddrinfo(res);
         free(connect_times);
         free_parsed_url(&parsed_url);
         return 5;
      }

      size_t total_sent = 0;
      while (total_sent < (size_t)req_len)
      {
         ssize_t n = send(sockfd, request + total_sent, (size_t)req_len - total_sent, 0);
         if (n < 0)
         {
            perror("send");
            close(sockfd);
            freeaddrinfo(res);
            free(connect_times);
            free_parsed_url(&parsed_url);
            return 6;
         }
         total_sent += (size_t)n;
      }

      struct timespec t_req_sent;
      clock_gettime(CLOCK_MONOTONIC, &t_req_sent);

      // 4. Time to first byte and full transfer
      ByteBuffer resp_buf;
      byte_buffer_init(&resp_buf);

      char chunk[RECV_CHUNK_SIZE];
      struct timespec t_first_byte;
      int got_first_byte = 0;

      // Receive first chunk
      ssize_t n = recv(sockfd, chunk, sizeof(chunk), 0);
      if (n > 0)
      {
         clock_gettime(CLOCK_MONOTONIC, &t_first_byte);
         got_first_byte = 1;
         if (byte_buffer_append(&resp_buf, chunk, (size_t)n) != 0)
         {
            close(sockfd);
            freeaddrinfo(res);
            free(connect_times);
            free_parsed_url(&parsed_url);
            return 7;
         }
      }
      else if (n == 0)
      {
         // Server closed immediately without data
         clock_gettime(CLOCK_MONOTONIC, &t_first_byte);
      }
      else
      {
         perror("recv (first byte)");
         close(sockfd);
         freeaddrinfo(res);
         free(connect_times);
         free_parsed_url(&parsed_url);
         return 7;
      }

      double first_byte_time_ms = 0.0;
      if (got_first_byte)
      {
         first_byte_time_ms = timespec_diff_ms(&t_req_sent, &t_first_byte);
      }

      // Read remaining response until EOF
      while ((n = recv(sockfd, chunk, sizeof(chunk), 0)) > 0)
      {
         if (byte_buffer_append(&resp_buf, chunk, (size_t)n) != 0)
         {
            close(sockfd);
            freeaddrinfo(res);
            byte_buffer_free(&resp_buf);
            free(connect_times);
            free_parsed_url(&parsed_url);
            return 7;
         }
      }

      struct timespec t_transfer_done;
      clock_gettime(CLOCK_MONOTONIC, &t_transfer_done);

      if (n < 0)
      {
         perror("recv");
         close(sockfd);
         freeaddrinfo(res);
         byte_buffer_free(&resp_buf);
         free(connect_times);
         free_parsed_url(&parsed_url);
         return 7;
      }

      close(sockfd);
      freeaddrinfo(res);

      double transfer_time_ms = timespec_diff_ms(&t_first_byte, &t_transfer_done);

      // 5. Parse HTTP headers and body
      char status_line[512] = "UNKNOWN";
      size_t body_offset = 0;
      size_t body_bytes = 0;

      if (resp_buf.len > 0)
      {
         // Find status line
         const char *crlf = strstr(resp_buf.data, "\r\n");
         const char *lf = strchr(resp_buf.data, '\n');
         const char *line_end = crlf ? crlf : lf;
         if (line_end)
         {
            size_t status_len = (size_t)(line_end - resp_buf.data);
            if (status_len >= sizeof(status_line))
            {
               status_len = sizeof(status_line) - 1;
            }
            memcpy(status_line, resp_buf.data, status_len);
            status_line[status_len] = '\0';
         }

         // Find header/body boundary (\r\n\r\n or \n\n)
         const char *header_end = strstr(resp_buf.data, "\r\n\r\n");
         if (header_end)
         {
            body_offset = (size_t)(header_end - resp_buf.data) + 4;
         }
         else
         {
            header_end = strstr(resp_buf.data, "\n\n");
            if (header_end)
            {
               body_offset = (size_t)(header_end - resp_buf.data) + 2;
            }
            else
            {
               body_offset = resp_buf.len;
            }
         }

         if (resp_buf.len >= body_offset)
         {
            body_bytes = resp_buf.len - body_offset;
         }
      }

      // Calculate throughput in Mbit/s
      // (body_bytes * 8) / (transfer_time_seconds * 1,000,000)
      double throughput_mbps = 0.0;
      double transfer_time_sec = transfer_time_ms / 1000.0;
      if (transfer_time_sec > 1e-9 && body_bytes > 0)
      {
         throughput_mbps = ((double)body_bytes * 8.0) / (transfer_time_sec * 1000000.0);
      }

      printf("  %-10s %7.1f ms\n", "first byte", first_byte_time_ms);
      printf("  %-10s %7.1f ms  %zu bytes, %.2f Mbit/s\n", "transfer", transfer_time_ms, body_bytes,
             throughput_mbps);
      printf("  %-10s %s\n", "status", status_line);

      // On the LAST trial, write body to output file if requested
      if (t == trials && output_file != NULL)
      {
         FILE *fout = fopen(output_file, "wb");
         if (!fout)
         {
            fprintf(stderr, "Error: Could not open output file '%s' for writing: %s\n", output_file,
                    strerror(errno));
         }
         else
         {
            if (body_bytes > 0)
            {
               size_t written = fwrite(resp_buf.data + body_offset, 1, body_bytes, fout);
               if (written != body_bytes)
               {
                  fprintf(stderr, "Error: Incomplete write to '%s'\n", output_file);
               }
            }
            fclose(fout);
         }
      }

      byte_buffer_free(&resp_buf);
   }

   // Min and median connect time
   qsort(connect_times, (size_t)trials, sizeof(double), compare_doubles);
   double min_connect = connect_times[0];
   double median_connect;
   if (trials % 2 == 1)
   {
      median_connect = connect_times[trials / 2];
   }
   else
   {
      median_connect = (connect_times[trials / 2 - 1] + connect_times[trials / 2]) / 2.0;
   }

   printf("connect over %d trials: min %.1f ms, median %.1f ms\n", trials, min_connect,
          median_connect);

   free(connect_times);
   free_parsed_url(&parsed_url);
   return 0;
}
