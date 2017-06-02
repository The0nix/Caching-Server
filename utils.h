#include <stdbool.h>
#ifndef _UTILSH_
#define _UTILSH_

struct global_args_t_ {
        bool debug_mode;
        int port;
        int proxy_mode;
        char *site_path;
        char *site_address;
};
typedef struct global_args_t_ global_args_t; 
extern global_args_t global_args;

void display_help();

void slog(const char* str, ...);

void read_conf();

bool read_conf_file(const char *path);

void check_args(int argc, char **argv);

int create_and_bind();

void make_non_blocking(int sd);
#endif
