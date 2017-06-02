#include "utils.h"
#include <stdbool.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

global_args_t global_args;

static const char *opt_string = "dp:s:c:i:a:h?";

void display_help()
{
        printf("server [-d][-h][-p port][-a target-site-address]\n");
}

void slog(char* str, ...)
{
        if(global_args.debug_mode) {
            va_list args;
            va_start(args,str);
            vprintf(str,args);
            va_end(args);
        }
}

bool read_conf_file(const char *path) {
        FILE* f = fopen(path, "r");
        if (f != NULL) {
                const int line_size = 100;
                const int key_size = 100;
                const int value_size = 100;
                char line[line_size], key[key_size], value[value_size];
                while(fgets(line, line_size, f) != NULL) {
                        for(char *c = line; *c ; ++c) {
                                tolower(*c);
                                if (*c == '=') *c = ' ';       
                        }
                        sscanf (line, "%s %s", key, value);
                        if (!strcmp(key, "debug")) {
                                global_args.debug_mode = atoi(value);
                        } else if (!strcmp(key, "port")) {
                                global_args.port = atoi(value);
                        } else {
                                slog("Unknown value in %s: %s", path, key);
                        }
                } 
                return true;
        } 
        return false;
}

void read_conf() {
        global_args.debug_mode = 0;
        global_args.proxy_mode = 0;
        global_args.port = 15000;
        read_conf_file("/etc/caser/caser.conf");
        read_conf_file("/usr/local/etc/caser/caser.conf");
        read_conf_file("/usr/local/caser/caser.conf");
        read_conf_file("caser.conf");
}

void check_args(int argc, char **argv) {
        char cmd_flags;
        while ((cmd_flags = getopt(argc, argv, opt_string)) != -1) {
               switch(cmd_flags) {
               case('d'):
   
                                global_args.debug_mode = true;
                                break;
                        case('p'):
                                global_args.port = atoi(optarg);
                                break;
                        case('c'):
                                read_conf_file(optarg);
                                break;
                        case('a'):
                                global_args.site_address = strdup(optarg);
                                global_args.proxy_mode = 1;
                                break;
                        case('s'):
                                global_args.site_path = strdup(optarg);
                                break;
                        case('h'):
                        case('?'):
                                display_help();
                                exit(1);
                        default:
                                break;
               }
        }
}

int create_and_bind()
{
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0); 
    if(socket_fd > 0) {
            slog("Created socket\n");
    } else {
            slog("ERROR: Can not create socket\n");
            exit(1);
    }
    
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(global_args.port);
    slog("Address: %d:%d\n", address.sin_addr, ntohs(address.sin_port));

    if (bind(socket_fd, (struct sockaddr *) &address, sizeof(address)) == 0){    
            slog("Bound socket to port: %d\n", global_args.port);
    } else {
            perror("bind");
            slog("ERROR: Can't bind socket to port %d\n", global_args.port);
            exit(1);
    }
    return socket_fd;
}

void make_non_blocking(int sd)
{
    int flags = fcntl(sd, F_GETFL);
    if (flags < 0) {
        perror("fcntl getfl");
        exit(1);
    }
    if (fcntl(sd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl setfl");
        exit(1);
    }
}
