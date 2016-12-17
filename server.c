#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

int main(int argc, char **argv) 
{
        read_conf();
        check_args(argc, argv);
        slog("Program started.\n");
        int create_socket, new_socket;
        socklen_t addrlen;
        int bufsize = 1024*1024;
        char *buffer = malloc(bufsize);
        struct sockaddr_in address;

        create_socket = socket(AF_INET, SOCK_STREAM, 0); 
        if(create_socket > 0) {
                slog("Created socket\n");
        } else {
                slog("ERROR: Can not create socket\n");
                exit(1);
        }
        
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(global_args.port);
        slog("Address: %d:%d\n", address.sin_addr, ntohs(address.sin_port));

        if (bind(create_socket, (struct sockaddr *) &address, sizeof(address)) == 0){    
                slog("Bound socket.\n");
        } else {
                perror("bind");
                slog("ERROR: Can't bind socket to port %d\n", global_args.port);
                exit(1);
        }

        while (1) {    
                if (listen(create_socket, 10) < 0) {    
                        perror("listen");    
                        exit(1);    
                }   
                          
                if ((new_socket = accept(create_socket, (struct sockaddr *) &address, &addrlen)) < 0) {    
                        perror("accept");    
                        exit(1);   
                }    
                        
                if (new_socket > 0){    
                        printf("The Client is connected...\n");
                } 
                                                     
                recv(new_socket, buffer, bufsize, 0);    
                write(new_socket, "HTTP/1.1 200 OK\n", 16);
                write(new_socket, "Content-length: 46\n", 19);
                write(new_socket, "Content-Type: text/html\n\n", 25);
                write(new_socket, "<html><body><H1>Hello world</H1></body></html>",46);
                printf("%s\n", buffer);    
                close(new_socket);    
        }    
        close(create_socket);
        return 0;
}
