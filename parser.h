#ifndef _PARSERH_
#define _PARSERH_
#define PARSER_ERROR_BAD_REQUEST 1
#define PARSER_ERROR_NOMEM 2
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <vector>

typedef enum r_type {
    GET,
    POST
} r_type_t;

typedef enum state {
    st_start,
    st_type,
    st_type_space,
    st_uri_start,
    st_uri,
    st_uri_space,
    st_version,
    st_version_major,
    st_version_minor,
    st_version_space,
    st_header_start,
    st_header_name,
    st_header_name_space,
    st_header_value,
    st_header_value_space,
    st_end
} state_t;

typedef struct http_header {
    char *name;
    char *value;
} http_header_t;

typedef struct http_request {
    int ready;
    r_type_t type;
    char *path;
    int version_major;
    int version_minor;
    std::vector<http_header_t> headers;
} http_request_t;

int check_traversal_safe(char *path);

void http_request_copy(http_request_t *dst, http_request_t *src);

int http_parse_request(const char *data, http_request_t *request);
#endif
