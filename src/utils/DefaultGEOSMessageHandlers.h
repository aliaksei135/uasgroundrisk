/*
 * DefaultGEOSMessageHandlers.h
 *
 *  Created by A.Pilko on 23/04/2021.
 */

#ifndef UASGROUNDRISK_SRC_UTILS_DEFAULTGEOSMESSAGEHANDLERS_H_
#define UASGROUNDRISK_SRC_UTILS_DEFAULTGEOSMESSAGEHANDLERS_H_

#include <cstdarg>

static void notice(const char *fmt, ...) {
  va_list ap;

  fprintf(stdout, "NOTICE: ");

  va_start(ap, fmt);
  vfprintf(stdout, fmt, ap);
  va_end(ap);
  fprintf(stdout, "\n");
}

static void log_and_exit(const char *fmt, ...) {
  va_list ap;

  fprintf(stdout, "ERROR: ");

  va_start(ap, fmt);
  vfprintf(stdout, fmt, ap);
  va_end(ap);
  fprintf(stdout, "\n");
  exit(1);
}

#endif // UASGROUNDRISK_SRC_UTILS_DEFAULTGEOSMESSAGEHANDLERS_H_
