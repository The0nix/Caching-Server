#include "utils.h"
#include <stdbool.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

global_args_t global_args;

static const char *opt_string = "dp:c:h?";

void display_help()
{
        printf("server [-d][-h][-p port][-c config-path]\n");
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
                        case('h'):
                        case('?'):
                                display_help();
                                exit(1);
                        default:
                                break;
               }
        }
}
