#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>

bool debug_mode = false;

void display_help()
{
        printf("help\n");
}

void slog(char *str) 
{
        if(debug_mode) {
                printf("%s", str);
        }
}

int main(int argc, char **argv) 
{
        char cmd_flags;
        while ((cmd_flags = getopt(argc, argv, "dh?")) != -1) {
               switch(cmd_flags) {
                        case('d'):
                                debug_mode = true;
                                break;
                        case('h'):
                        case('?'):
                                display_help();
                                return 0;
                        default:
                                break;
               }
        }
        slog("Program started.\n");
        while(1) {
                sleep(1);
        }
        return 0;
}
