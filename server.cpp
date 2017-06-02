#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
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

#include <map>
#include <sstream>
#include <string>

volatile sig_atomic_t Stop = 0;

const int buffer_size = 4096;
const int MaxEvents = 10000;

void stop(int blank)
{
    Stop = 1;
}

int write_headers(int fd, int status, char* comment, std::vector<http_header_t> headers) {
    std::ostringstream sstr;
    sstr << "HTTP/1.1 " << status << " " << comment << "\r\n";
    for (size_t i = 0; i < headers.size(); ++i) {
        sstr << headers[i].name << ": " << headers[i].value << "\r\n";
    }
    sstr << "\r\n";
    return write(fd, sstr.str().c_str(), sstr.str().size());
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
                const uint32_t emask = events[i].events;
                const bool e_error = emask & EPOLLERR;
                const bool e_hup = emask & EPOLLHUP;
                const bool e_rdhup = emask & EPOLLRDHUP;
                const bool e_out = emask & EPOLLOUT;
                const bool e_in = emask & EPOLLIN;
                const int fd = events[i].data.fd;                   
                
                if (e_error || e_hup) {
                    if (e_error || e_hup) {
                        slog((char*)"Error with some socket\n");
                    }
                    close(fd);
                    if (fdpairs.count(fd)) {
                        close(fdpairs[fd]);
                        fdpairs.erase(fd);
                    }
                    continue;
                }
                if (fd == socket_fd) {
                    while(1) {
                        struct sockaddr_in in_addr;
                        socklen_t in_addr_size = sizeof(in_addr);
                        int incoming_fd = accept(socket_fd, (sockaddr*)&in_addr, &in_addr_size);
                        if (-1 == incoming_fd) {
                            if (EAGAIN == errno || EWOULDBLOCK == errno) {
                                break;
                            }
                            else {
                                perror("accept");
                                requests.erase(incoming_fd);
                                close(socket_fd);
                                exit(1);
                            }
                        }
                        else {
                            make_non_blocking(incoming_fd);
                            event.data.fd = incoming_fd;
                            event.events = EPOLLIN | EPOLLOUT;
                            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, incoming_fd, &event) < 0) {
                                perror("epoll_ctl");
                                close(socket_fd);
                                exit(1);
                            }
                            http_request_t r;
                            r.ready = 0;
                            requests.insert({incoming_fd, r});
                            requests_raw.insert({incoming_fd, ""});
                            fdpairs.insert({incoming_fd, -1});
                            slog("A client has connected...\n");
                        }
                    }
                } else if (e_out && requests[fd].ready) {
                    if (fdpairs[fd] < 0) {
                        std::vector<http_header_t> headers{{"Content-Type", "text/html"}, {"Content-Length", "49"}};
                        write_headers(fd, 404, "Not Found", headers);
                        write(fd, "<html><body><h1>404: Not Found</h1></body></html>", 49);
                        fdpairs.erase(fd);
                        requests.erase(fd);
                        requests_raw.erase(fd);
                        close(fd);       
                    } else {
                        if (requests[fd].ready == 1) {
                            requests[fd].ready = 2;
                            if (!global_args.proxy_mode) {
                                struct stat stats;
                                if (stat(requests[fd].path, &stats) < 0) {
                                    perror("fstat");
                                    close(fdpairs[fd]);
                                    fdpairs.erase(fd);
                                    requests.erase(fd);
                                    requests_raw.erase(fd);
                                    close(fd);
                                    continue;
                                }
                                char size[15];
                                snprintf(size, 15, "%d", stats.st_size);
                                std::vector<http_header_t> headers{{"Content-Type", "text/html"}, {"Content-Length", size}};
                                write_headers(fd, 200, "OK", headers);
                            }
                        }
                        char buf[buffer_size];
                        int read_size = read(fdpairs[fd], buf, buffer_size);
                        if (read_size < 0) {
                            perror("read");
                            close(fdpairs[fd]);
                            fdpairs.erase(fd);
                            requests.erase(fd);
                            requests_raw.erase(fd);
                            close(fd);
                            continue;
                        } else if (read_size > 0) {
                            write(fd, buf, read_size);
                        } else {
                            close(fdpairs[fd]);
                            fdpairs.erase(fd);
                            requests.erase(fd);
                            requests_raw.erase(fd);
                            close(fd);
                        }
                    }
                } else if (e_in) {
                    while (1) {
                        int recv_size = recv(fd, buffer, bufsize, 0);    
                        
                        if (recv_size == 0 || (recv_size < 0 && errno == EAGAIN)) {
                            if (http_parse_request(requests_raw[fd].c_str(), &requests[fd]) < 0) {
                                slog("Bad request:\n");
                                std::vector<http_header_t> headers{{"Content-Type", "text/html"}, {"Content-Length", "51"}};
                                write_headers(fd, 500, "Bad Request", headers);
                                write(fd, "<html><body><h1>500: Bad Request</h1></body></html>", 49);
                                close(fdpairs[fd]);
                                fdpairs.erase(fd);
                                requests.erase(fd);
                                requests_raw.erase(fd);
                                close(fd);
                                slog("%s\n", buffer);
                                break;
                            }
                            if (global_args.proxy_mode) {
                                fdpairs[fd] = socket(PF_INET, SOCK_STREAM, 0);
                                connect(fdpairs[fd], proxy_addrinfo->ai_addr, proxy_addrinfo->ai_addrlen);
                                size_t hbegin = requests_raw[fd].find("Host");
                                size_t hlen = requests_raw[fd].substr(hbegin).find("\n");
                                requests_raw[fd].replace(hbegin + 6, hlen - 6, std::string(proxy_site)); //len("Host: ") == 6
                                slog("<<%s>>\n", requests_raw[fd].c_str());
                                write(fdpairs[fd], requests_raw[fd].c_str(), requests_raw[fd].length());
                            } else {
                                slog("<<%s>>\n", requests_raw[fd].c_str());
                                fdpairs[fd] = open(requests[fd].path, O_RDONLY);
                                if (open >= 0) {
                                    printf("\ncheck!\n\n");
                                    struct stat buf;
                                    fstat(fdpairs[fd], &buf);
                                    if (S_ISDIR(buf.st_mode)) {
                                        printf("\ncheck2!\n\n");
                                        fdpairs[fd] = -1;
                                    }
                                }
                            }
                            break;
                        }
                        if (recv_size < 0 ) {
                            perror("recv");
                            close(fd);
                        }
                        requests_raw[fd].append(buffer); 
                    }   
                } 
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

