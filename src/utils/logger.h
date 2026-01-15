#ifndef PULSEEM_UTILS_LOGGER_H
#define PULSEEM_UTILS_LOGGER_H

#include <stdio.h>

#define LOG_INFO(...)  do { fprintf(stdout, "[INFO] "); fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n"); } while(0)
#define LOG_WARN(...)  do { fprintf(stdout, "[WARN] "); fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n"); } while(0)
#define LOG_ERROR(...) do { fprintf(stderr, "[ERROR] "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } while(0)

#define log_info(...)    LOG_INFO(__VA_ARGS__)
#define log_warning(...) LOG_WARN(__VA_ARGS__)
#define log_error(...)   LOG_ERROR(__VA_ARGS__)
#define log_debug(...)   LOG_INFO(__VA_ARGS__)

#endif /* PULSEEM_UTILS_LOGGER_H */
