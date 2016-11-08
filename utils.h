#include <stdbool.h>

struct global_args_t_ {
        bool debug_mode;
        int port;
        char *conf_path;
};
typedef struct global_args_t_ global_args_t; 
extern global_args_t global_args;


void display_help();

void slog(char* str, ...);

void check_args(int argc, char **argv);

