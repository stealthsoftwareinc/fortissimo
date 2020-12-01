/**
 * Copyright Stealth Software Technologies, Inc.
 */

/* C and POSIX headers */
#include <cstdio>
#define _POSIX_THREAD_SAFE_FUNCTIONS
#include <ctime>

#ifdef __GLIBC__
#include <execinfo.h>
#endif //__GLIBC__

/* C++ Headers */
#include <chrono>

/* Local headers */
#include <ff/logging.h>

thread_local std::string LOG_ORGANIZATION = "unknown";
FILE * LOG_FILE = stderr;

thread_local char LOG_TIME_STR_private[LOG_TIME_FMT_LEN];

void log_time_update_PRIVATE() {
  time_t now;
  tm time_obj;
  time(&now);
  localtime_r(&now, &time_obj);
  strftime(
      LOG_TIME_STR_private, LOG_TIME_FMT_LEN, LOG_TIME_FMT, &time_obj);
}

void log_stack_trace() {
#ifdef __GLIBC__
  void * backtraces[100];
  int num_backtraces = backtrace(backtraces, 100);
  backtrace_symbols_fd(backtraces, num_backtraces, fileno(LOG_FILE));
  fflush(LOG_FILE);
#endif //__GLIBC__
}

#ifdef LOG_ENABLE_TIMER
LogTimer::LogTimer(char const * name) :
    start(::std::chrono::steady_clock::now()), name(name) {
}

long LogTimer::time() {
  ::std::chrono::microseconds diff =
      ::std::chrono::duration_cast<::std::chrono::microseconds>(
          ::std::chrono::steady_clock::now() - this->start);
  return diff.count();
}
#endif // LOG_ENABLE_TIMER
