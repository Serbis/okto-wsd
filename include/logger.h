#ifndef INCLUDE_LOGGER_H_
#define INCLUDE_LOGGER_H_

#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_ERROR 2
#define LOG_LEVEL_FATAL 3

#define LOG_STDOUT
#define LOG_FILE
#define LEVEL LOG_LEVEL_DEBUG

int Logger_init(const char* fp);
void Logger_debug(char *source, char *str, ...);
void Logger_info(char *source, char *str, ...);
void Logger_error(char *source, char *str, ...);
void Logger_fatal(char *source, char *str, ...);

#endif /* INCLUDE_LOGGER_H_ */
