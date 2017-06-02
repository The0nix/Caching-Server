#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/types.h>
#include <unistd.h>

#include "parser.h"
#include "utils.h"
#include "event.h"

#include <map>
#include <string>

volatile sig_atomic_t Stop = 0;

const int buffer_size = 4096;
const int MaxEvents = 10000;

void stop(int blank)
{
    Stop = 1;
}

int main(int argc, char **argv) 
{
        read_conf();
        check_args(argc, argv);
        slog((char*)"Program started.\n");
        socklen_t addrlen;
        int bufsize = 1024*1024;
        char *buffer = (char*)malloc(bufsize); 
        char proxy_site[1024];
        char proxy_service[1024];

        int socket_fd = create_and_bind();
        make_non_blocking(socket_fd);

        int epoll_fd = epoll_create1(0);
        struct epoll_event *events = (struct epoll_event*)calloc(MaxEvents, sizeof(events));
        
        struct epoll_event event;
        memset(&event, 0, sizeof(event));
        event.data.fd = socket_fd;
        event.events = EPOLLIN;

        struct addrinfo *proxy_addrinfo;
        if (global_args.proxy_mode) {
            sscanf(global_args.site_address, "%1023[^:/]://%1023s", proxy_service, proxy_site);
            struct addrinfo hints = {};
            if (int errcode = getaddrinfo(proxy_site, proxy_service, &hints, &proxy_addrinfo)) {
                slog("Error resolving host \"%s\" with service \"%s\": %s\n", 
                        proxy_site, proxy_service, gai_strerror(errcode));
                exit(1);
            }
        }
 
        if (listen(socket_fd, SOMAXCONN) < 0) {    
            perror("listen");    
            close(socket_fd);
            exit(1);    
        }

        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &event) < 0) {
            perror("epoll_ctl");
            close(socket_fd);
            exit(1);
        }

        signal(SIGINT, stop);
        signal(SIGTERM, stop);

        std::map<int, int> fdpairs;
        std::map<int, http_request_t> requests;
        std::map<int, std::string> requests_raw;
        std::map<int, short> fd_ready; // 0 - not ready, 1 - read, 2 - write

        while (!Stop) {    
            int number_of_events = epoll_wait(epoll_fd, events, MaxEvents, -1);
            if (number_of_events < 0) {
                perror("epoll_wait");
                close(socket_fd);
                exit(1);
            }
            
            for (int i = 0; i < number_of_events; ++i) {
                struct event_attrs_t *eat = (struct event_attrs_t*)malloc(sizeof(struct event_attrs_t));
                eat->emask = events[i].events;
                eat->fd = events[i].data.fd; 
                eat->fdpairs = &fdpairs;
                eat->requests = &requests;
                eat->requests_raw = &requests_raw;
                eat->socket_fd = socket_fd;
                eat-> epoll_fd = epoll_fd;
                eat->proxy_site = proxy_site;
                eat->proxy_addrinfo = proxy_addrinfo;
                pthread_t pt;
                pthread_attr_t attr;
                pthread_attr_init(&attr);
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
                pthread_create(&pt, &attr, process_event, &eat);
                pthread_attr_destroy(&attr);
            }   
        }    
        free(events);
        close(socket_fd);
        if (global_args.site_path != NULL) {
            free(global_args.site_path);
        }
        if (global_args.site_address != NULL) {
            free(global_args.site_address);
        }
        printf("\nClosing...\n");
        return 0;
}
