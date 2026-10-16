#ifndef REQUEST_H
#define REQUEST_H

#include <3ds.h>
#include <citro2d.h>
#include <curl/curl.h>
#include "logs.h"
#include "../libs/lodepng.h"
#include <jpeglib.h>
#include <setjmp.h>
#include <jansson.h>


struct my_error_mgr {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

extern bool doing_debug_logs;
typedef struct my_error_mgr* my_error_ptr;




typedef struct {
    char *data;
    size_t size;
    size_t capacity;
    volatile bool done; 
} ResponseBuffer;

typedef struct {
    char url[512];
    char *data;
    struct curl_slist *headers;
    bool owns_data;
    void **response;
    size_t *response_size;
    bool is_binary;
    ResponseBuffer *response_buf; 
    int request_type;
} Request;

typedef enum {
    GET = 0,
    POST = 1,
    PATCH = 2,
    PUT = 3
} RequestType;

extern C2D_SpriteSheet kuponobraz;
extern C2D_Image kuponkurwa;
extern bool obrazekdone;


extern bool requestdone;
extern bool loadingshit;
extern bool czasfuckup;
extern long response_code;
extern LightLock global_response_lock;


void start_request_thread();
void stop_request_thread();
void queue_request(const char *url, const char *data, struct curl_slist *headers,
                   ResponseBuffer *response_buf, bool is_binary, RequestType request_type);

void request_worker(void* arg);
bool refresh_data(CURL *curl, const char *url, const char *data, struct curl_slist *headers, ResponseBuffer *response_buf, RequestType request_type);
void load_image();
size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata);
void log_request_to_file(const char *url, const char *data, struct curl_slist *headers, char *response);
void log_message(const char *format, ...);

#endif 
