#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define EOL "\r\n"
#define EOL_SIZE 2

void fatal(char *message){char em[100];strcpy(em,"[!!] Fatal Error ");strncat(em,message,83);perror(em);exit(-1);}

int gfs(int fd){struct stat stat_struct;if(fstat(fd, &stat_struct) == -1){return -1;}return (int) stat_struct.st_size;}

int recv_line(int sockfd, unsigned char *dest_buffer) {
   unsigned char *ptr;
   int eol_matched = 0;   
   ptr = dest_buffer;
   while(recv(sockfd, ptr, 1, 0) == 1) {
      if(*ptr == EOL[eol_matched]) { 
         eol_matched++;
         if(eol_matched == EOL_SIZE) {
            *(ptr+1-EOL_SIZE) = '\0'; 
            return strlen(dest_buffer);
         }
      } else {eol_matched = 0;}
      ptr++;
   }return 0;}

void hc(int sockfd, struct sockaddr_in *client_addr_ptr) {
   unsigned char *ptr, request[500], resource[500];
   int fd, length;

   length = recv_line(sockfd, request);

   printf("Got request from %s:%d \"%s\"\n", inet_ntoa(client_addr_ptr->sin_addr), ntohs(client_addr_ptr->sin_port), request);

   ptr = strstr(request, " HTTP/"); 
   if(ptr == NULL) { 
      printf(" NOT HTTP!\n");
   } else {
      *ptr = 0; 
      ptr = NULL; 
      if(strncmp(request, "GET ", 4) == 0){ptr = request+4;}
      if(strncmp(request, "HEAD ", 5) == 0){ptr = request+5;}
      if(ptr == NULL) {
         printf("\tUNKNOWN REQUEST!\n");
      } else { 
         if (ptr[strlen(ptr) - 1] == '/')  
            strcat(ptr, "index.html");     
         strcpy(resource, "./webroot");
         strcat(resource, ptr);         
         fd = open(resource, O_RDONLY, 0); 
         printf("\tOpening \'%s\'\t", resource);
         if(fd == -1) { 
            printf(" 404 Not Found\n");
            s_str(sockfd, "HTTP/1.0 404 NOT FOUND\r\n");
            s_str(sockfd, "Server: Tiny webserver\r\n\r\n");
            s_str(sockfd, "<html><head><title>404 Not Found</title></head>");
            s_str(sockfd, "<body><h1>URL not found</h1></body></html>\r\n");
         } else {      
            printf(" 200 OK\n");
            s_str(sockfd, "HTTP/1.0 200 OK\r\n");
            s_str(sockfd, "Server: Tiny webserver\r\n\r\n");
            if(ptr == request + 4) { 
               if( (length = gfs(fd)) == -1)
                  fatal("getting resource file size");
               if( (ptr = (unsigned char *) malloc(length)) == NULL)
                  fatal("allocating memory for reading resource");
               read(fd, ptr, length); 
               send(sockfd, ptr, length, 0);  
               free(ptr); 
            }
            close(fd); 
         } 
      } 
   } 
   shutdown(sockfd, SHUT_RDWR); 
}

int s_str(int sockfd, unsigned char *buffer) {
   int sent_bytes, bytes_to_send;
   bytes_to_send = strlen(buffer);
   while(bytes_to_send > 0) {
      sent_bytes = send(sockfd, buffer, bytes_to_send, 0);
      if(sent_bytes == -1)
         return 0; // return 0 on send error
      bytes_to_send -= sent_bytes;
      buffer += sent_bytes;
   }return 1;}

int main(void) {
   int sockfd, new_sockfd, yes=1; 
   struct sockaddr_in host_addr, client_addr;   
   socklen_t sin_size;

   printf("Server started\n");

   if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1){fatal("in socket");}
   if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
      fatal("setting socket option SO_REUSEADDR");

   host_addr.sin_family = AF_INET;      
   host_addr.sin_port = htons(8080);    
   host_addr.sin_addr.s_addr = INADDR_ANY; 
   memset(&(host_addr.sin_zero), '\0', 8); 

   if (bind(sockfd, (struct sockaddr *)&host_addr, sizeof(struct sockaddr)) == -1)
      fatal("binding to socket");
   if (listen(sockfd, 20) == -1){fatal("listening on socket");}

   while(1) {   
      sin_size = sizeof(struct sockaddr_in);
      new_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
      if(new_sockfd == -1){fatal("accepting connection");}
      hc(new_sockfd, &client_addr);
   }
   return 0;
}

