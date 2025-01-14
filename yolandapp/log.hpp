#ifndef YOLANDAPP_LOG_H
#define YOLANDAPP_LOG_H

#include "stdarg.h"

#define YOLANDA_UNUSED(v) (void)v

#define LOG_DEBUG_TYPE 0
#define LOG_MSG_TYPE   1
#define LOG_WARN_TYPE  2
#define LOG_ERR_TYPE   3

void yolanda_log(int severity, const char *msg);
void yolanda_logx(int severity, const char *errstr, const char *fmt, va_list ap);
void yolanda_msgx(const char *fmt, ...);
void yolanda_debugx(const char *fmt, ...);

void error_die(const char *s);
void error(int status, int err, const char *fmt, ...);

#define YOLONDA_LOG_MSG(msg) yolanda_log(LOG_MSG_TYPE, msg)
#define YOLONDA_LOG_ERR(msg) yolanda_log(LOG_ERR_TYPE, msg)

#endif
