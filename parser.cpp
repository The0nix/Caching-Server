#include "parser.h"
#include "utils.h"

const char* REQUEST_TYPE_STRINGS[] = {"GET" , "POST"};
const int MAX_HEADER_NAME = 1024;
const int MAX_HEADER_VALUE = 1024;
const int MAX_SPACES = 1024;

void http_request_copy(http_request_t *dst, http_request_t *src) {
    dst->ready = src->ready;
    dst->type = src->type;
    dst->path = src->path;
    dst->version_major = src->version_major;
    dst->version_minor = src->version_minor;
    dst->headers = src->headers;
}

int check_traversal_safe(char *path) {
    char c[2] = {0, 0};
    size_t size = strlen(path);
    int back_cnt = 0;
    for (size_t i = 0; i < size; ++i) {
        if (path[i] == '/') {
            if (c[0] == '.' && c[1] == '.') {
                ++back_cnt;
            } else if (c[0] == '.' && c[1] != '/') {
                --back_cnt;
            }
        }
        c[1] = c[0];
        c[0] = path[i];
    }
    if (back_cnt > 0) {
        return 0;
    }
    return 1;
}

int http_parse_request(const char *data, http_request_t *request) {
    http_request_t tmp_r;
    ssize_t pos = 0;
    ssize_t pos_save = 0;
    state_t state = st_start;
    int data_len = strlen(data);
    while(state != st_end) {
        if (pos >= data_len) {
            errno = PARSER_ERROR_BAD_REQUEST;
            return -1;
        }
        switch(state) {
            case(st_start):
                switch(data[pos]) {
                    case('G'):
                        tmp_r.type = GET;
                        break;
                    case('P'):
                        tmp_r.type = POST;
                        break;
                    default:
                        errno = PARSER_ERROR_BAD_REQUEST;
                        return -1;
                }
                ++pos;
                state = st_type;
                break;
            case(st_type):
                if (pos >= strlen(REQUEST_TYPE_STRINGS[tmp_r.type])) {
                    pos_save = pos;
                    state = st_type_space;
                } else if (data[pos] != REQUEST_TYPE_STRINGS[tmp_r.type][pos]) {
                    errno = PARSER_ERROR_BAD_REQUEST;
                    return -1;
                }
                ++pos;
                break;
            case(st_type_space):
                if (data[pos] != ' ') {
                    state = st_uri_start;
                } else if (pos - pos_save > MAX_SPACES) {
                    errno = PARSER_ERROR_BAD_REQUEST;
                    return -1; 
                } else {
                    ++pos;
                }
                break;
            case(st_uri_start):
                if (data[pos] != '/') {
                    errno = PARSER_ERROR_BAD_REQUEST;
                    return -1;
                }
                pos_save = pos;
                state = st_uri;
                ++pos;
                break;
            case(st_uri):
                if (data[pos] == ' ') {
                    tmp_r.path = strndup(data + pos_save - 1, pos - pos_save + 1);
                    if (!check_traversal_safe(tmp_r.path + 1)) {
                        free(tmp_r.path);
                        errno = PARSER_ERROR_BAD_REQUEST;
                        return -1;
                    }
                    tmp_r.path[0] = '.';
                    pos_save = pos;
                    state = st_uri_space;
                }
                ++pos;
                break;
            case(st_uri_space):
                if (data[pos] != ' ') {
                    state = st_version;
                    pos_save = pos;
                } else if (pos - pos_save > MAX_SPACES) {
                    errno = PARSER_ERROR_BAD_REQUEST;
                    return -1; 
                } else {
                    ++pos;
                }
                break;
            case(st_version):
                if (pos - pos_save >= 5) {
                    state = st_version_major;
                    pos_save = pos;
                } else if (data[pos] != "HTTP/"[pos-pos_save]) {
                    errno = PARSER_ERROR_BAD_REQUEST;
                    return -1;
                } else {
                    ++pos;
                }
                break;
            case(st_version_major):
                if(data[pos] == '.') {
                    tmp_r.version_major = strtol(data + pos_save, NULL, 0);
                    if (errno == EINVAL || errno == ERANGE) {
                        errno = PARSER_ERROR_BAD_REQUEST;
                        return -1;
                    } 
                    pos_save = pos + 1;
                    state = st_version_minor;
                }
                else if (!isdigit(data[pos]) || pos - pos_save > 10) {
                    errno = PARSER_ERROR_BAD_REQUEST;
                    return -1;
                }
                ++pos;
                break;
            case(st_version_minor):
                if(data[pos] == ' ' || data[pos] == '\r' || data[pos] == '\n') {
                    tmp_r.version_minor = strtol(data + pos_save, NULL, 0);
                    if (errno == EINVAL || errno == ERANGE) {
                        errno = PARSER_ERROR_BAD_REQUEST;
                        return -1;
                    } 
                    pos_save = pos;
                    state = st_version_space;
                }
                else if (!isdigit(data[pos]) || pos - pos_save > 10) {
                    errno = PARSER_ERROR_BAD_REQUEST;
                    return -1;
                }
                ++pos;
                break;
            case(st_version_space):
                if (data[pos] == ' ' || data[pos] == '\r') {
                    if (pos - pos_save > MAX_SPACES) {
                        errno = PARSER_ERROR_BAD_REQUEST;
                        return -1; 
                    }
                    ++pos;
                } else if (data[pos] == '\n') {
                    state = st_header_start;
                    ++pos;
                } else {
                    errno = PARSER_ERROR_BAD_REQUEST;
                    return -1;
                }
                break;
            case(st_header_start):
                if (data[pos] == '\n' || data[pos] == '\r') {
                    state = st_end;
                } 
                else {
                    state = st_header_name;
                }
                break;
            case(st_header_name):
                if (data[pos] == ':') {
                    http_header tmp_h;
                    tmp_h.name = strndup(data + pos_save, pos-pos_save-1);
                    tmp_r.headers.push_back(tmp_h);
                    pos_save = pos;
                    state = st_header_name_space;
                } else if (pos - pos_save > MAX_HEADER_NAME) {
                    errno = PARSER_ERROR_BAD_REQUEST;
                    return -1;
                }
                ++pos;
                break;
            case(st_header_name_space):
                if (data[pos] == ' ') {
                    if (pos - pos_save > MAX_SPACES) {
                        errno = PARSER_ERROR_BAD_REQUEST;
                        return -1;
                    }
                    ++pos;
                } else {
                    pos_save = pos;
                    state = st_header_value;
                } 
                break;
            case(st_header_value):
                if (data[pos] == '\r' || data[pos] == '\n') {
                    tmp_r.headers.back().value = strndup(data + pos_save, pos-pos_save-1);
                    pos_save = pos;
                    state = st_header_value_space;
                } else if (pos - pos_save > MAX_HEADER_VALUE) {
                    errno = PARSER_ERROR_BAD_REQUEST;
                    return -1;
                }
                ++pos;
                break;
            case(st_header_value_space):
                if (data[pos] == '\r') {
                    if (pos - pos_save > MAX_SPACES) {
                        errno = PARSER_ERROR_BAD_REQUEST;
                        return -1;
                    }
                } else if (data[pos] == '\n') {
                    state = st_header_start;
                } else {
                    errno = PARSER_ERROR_BAD_REQUEST;
                    return -1;
                }
                ++pos;
                break;
        }
    }
    tmp_r.ready = 1;
    http_request_copy(request, &tmp_r);
    slog("Parsed\n");
    return 0;
}
