#include "logger.h"

// #include <math.h>
// #include <stdio.h>
#include <stdarg.h>
// #include <time.h>
#include <string.h>

static FILE *log_file = NULL;
static LogLevel current_min_level = LOG_INFO;
static const char *level_names[] = {"DEBUG", "INFO", "WARNING", "ERROR", "FATAL"};

int log_init(const char *log_path, LogLevel min_level) {
    if (log_file != NULL) {
        fclose(log_file);
    }

    log_file = fopen(log_path, "a");
    if (log_file == NULL) {
        fprintf(stderr, "Failed to open log file %s\n", log_path);
        return 0;
    }

    current_min_level = min_level;

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[26];
    strftime(time_str, 26, "%Y-%m-%D %H:%M:%S", tm_info);

    fprintf(log_file, "\n\n");
    fprintf(log_file, "==========================================================\n");
    fprintf(log_file, "=== New Session Started %s ===\n", time_str);
    fprintf(log_file, "==========================================================\n");

    return 1;
}

void log_close(void) {
    if (log_file != NULL) {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char time_str[26];
        strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);

        fprintf(log_file, "=== End session: %s ===\n", time_str);
        fprintf(log_file, "==========================================================\n");

        fclose(log_file);
        log_file = NULL;
    }
}

void log_message(LogLevel level, char *fmt, ...) {
    if (log_file == NULL || level < current_min_level) {
        return;
    }

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[26];
    strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(log_file, "[%s] [%s] ", time_str, level_names[level]);

    va_list args;
    va_start(args, fmt);
    vfprintf(log_file,fmt, args);
    va_end(args);

    fprintf(log_file, "\n");
    fflush(log_file);
}

void log_debug(const char *fmt, ...) {
    if (log_file == NULL || LOG_DEBUG < current_min_level) {
        return;
    }

    va_list args;
    va_start(args, fmt);

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[26];
    strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(log_file, "[%s] [DEBUG] ", time_str);
    vfprintf(log_file, fmt, args);
    fprintf(log_file, "\n");

    va_end(args);
    fflush(log_file);
}

void log_info(const char *fmt, ...) {
    if (log_file == NULL || LOG_INFO < current_min_level) {
        return;
    }

    va_list args;
    va_start(args, fmt);

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[26];
    strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(log_file, "[%s] [INFO] ", time_str);
    vfprintf(log_file, fmt, args);
    fprintf(log_file, "\n");

    va_end(args);
    fflush(log_file);
}

void log_warning(const char *fmt, ...) {
    if (log_file == NULL || LOG_WARNING < current_min_level) {
        return;
    }

    va_list args;
    va_start(args, fmt);

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[26];
    strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(log_file, "[%s] [WARNING] ", time_str);
    vfprintf(log_file, fmt, args);
    fprintf(log_file, "\n");

    va_end(args);
    fflush(log_file);
}

void log_error(const char *fmt, ...) {
    if (log_file == NULL || LOG_ERROR < current_min_level) {
        return;
    }

    va_list args;
    va_start(args, fmt);

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[26];
    strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(log_file, "[%s] [ERROR] ", time_str);
    vfprintf(log_file, fmt, args);
    fprintf(log_file, "\n");

    va_end(args);
    fflush(log_file);
}

void log_fatal(const char *fmt, ...) {
    if (log_file == NULL || LOG_FATAL < current_min_level) {
        return;
    }

    va_list args;
    va_start(args, fmt);

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[26];
    strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(log_file, "[%s] [FATAL] ", time_str);
    vfprintf(log_file, fmt, args);
    fprintf(log_file, "\n");

    va_end(args);
    fflush(log_file);
}