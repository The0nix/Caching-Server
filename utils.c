#include "utils.h"
#include <stdbool.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

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

void check_args(int argc, char **argv) {
        global_args.debug_mode = 0;
        global_args.port = 15000;
        char cmd_flags;
        while ((cmd_flags = getopt(argc, argv, opt_string)) != -1) {
               switch(cmd_flags) {
               case('d'):
   
                                global_args.debug_mode = true;
                                break;
                        case('p'):
                                global_args.port = atoi(optarg);
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
