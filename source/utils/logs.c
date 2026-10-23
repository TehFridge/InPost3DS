#include "logs.h"

bool logplz = false;
static FILE *log_file = NULL;

void init_logger(void)
{
    log_file = fopen(LOG_FILE, "a");
}

u64 get_free_mem(void)
{
    u64 freeMem = 0;
    svcGetSystemInfo(&freeMem, 0, 1);
    return freeMem;
}

void log_to_file(const char *format, ...)
{
    if (!logplz || !log_file)
        return;

    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);
}

void close_logger(void)
{
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}