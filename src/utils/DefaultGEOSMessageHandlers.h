/*
 * DefaultGEOSMessageHandlers.h
 *
 *  Created by A.Pilko on 23/04/2021.
 */

#ifndef UASGROUNDRISK_SRC_UTILS_DEFAULTGEOSMESSAGEHANDLERS_H_
#define UASGROUNDRISK_SRC_UTILS_DEFAULTGEOSMESSAGEHANDLERS_H_

#include <cstdarg>
#include <stdio.h>
#include <cstdlib>

static void notice(const char* fmt, ...)
{
#ifndef NDEBUG
    va_list ap;

    fprintf(stdout, "NOTICE: ");

    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
    fprintf(stdout, "\n");
#endif
}

static void log_and_exit(const char* fmt, ...)
{
#ifndef NDEBUG
    va_list ap;

    fprintf(stdout, "ERROR: ");

    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
    fprintf(stdout, "\n");
    exit(1);
#endif
}

#endif // UASGROUNDRISK_SRC_UTILS_DEFAULTGEOSMESSAGEHANDLERS_H_
