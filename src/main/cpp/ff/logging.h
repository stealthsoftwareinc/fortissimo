/**
 * Copyright Stealth Software Technologies, Inc.
 */

/**
 * WARNING: This file is abusive of preprocesor macros. If you're including
 * this file from a header please uninclude after use:
 *
 * #define LOG_UNCLUDE
 * #include <util/logging.h>
 *
 * This defines the following logging functions with printf arguments.
 *  - log_trace (disabled default, enable with #define LOG_ENABLE_TRACE)
 *  - log_debug (disabled default, enable with #define LOG_ENABLE_DEBUG)
 *  - log_info  (enabled default, disable with #define LOG_DISABLE_INFO)
 *  - log_warn  (enabled default, disable with #define LOG_DISABLE_WARN)
 *  - log_error (enabled default, disable with #define LOG_DISABLE_ERROR)
 *  - log_fatal (enabled always, cannot be disabled)
 *  - log_assert(conditionally invoke fatal, cannot be disabled)
 *
 * Use the LOG_FILE (FILE*) pointer to set where to log to, and use
 * LOG_ORGANIZATION to print the name of the organization on each log line.
 *
 * To change the time format set LOG_TIME_FMT as a compiler argument using
 * strftime(3), and LOG_TIME_FMT_LEN to be the maximum string length
 * (including null terminator). Do not change these on a per-file basis,
 * as the log uses a shared thread-local buffer for the time string.
 */

/* This line tells Mingw not to use printf from MSVCRT, which only conforms
 * to C89, and can't do things like %zx to print a size_t or pointer value */
#ifdef _WIN32
#ifndef __USE_MINGW_ANSI_STDIO
#define __USE_MINGW_ANSI_STDIO 1
#endif // __USE_MINGW_ANSI_STDIO
#endif // _WIN32

#include <cerrno>
#include <chrono>
#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <string>

/* Here begins the abuse of macros */
#ifndef LOG_UNCLUDE

#define LOG_COLOR_RED "\033[0;31m"
#define LOG_COLOR_RED_INVERT "\033[0;41m"
#define LOG_COLOR_GREEN "\033[0;32m"
#define LOG_COLOR_GREEN_INVERT "\033[0;42m"
#define LOG_COLOR_YELLOW "\033[0;33m"
#define LOG_COLOR_BLUE "\033[0;34m"
#define LOG_COLOR_MAGENTA "\033[0;35m"
#define LOG_COLOR_CYAN "\033[0;36m"
#define LOG_COLOR_DEFAULT "\033[0m"

#ifndef LOG_TIME_FMT
#define LOG_TIME_FMT "%Y-%m-%d %H:%M:%S"
#define LOG_TIME_FMT_LEN 20 // length of ^^ formatted time
#define LOG_TIME_FMT_DEFAULT
#endif

/* this short section is standard function/variable pre-definitions */
#ifndef FF_LOGGING_H_
#define FF_LOGGING_H_

extern thread_local std::string LOG_ORGANIZATION;
extern FILE * LOG_FILE;

extern thread_local char LOG_TIME_STR_private[LOG_TIME_FMT_LEN];
void log_time_update_PRIVATE();

void log_stack_trace();

/**
 * A helper class to work with printing time objects.
 */
class LogTimer {
#ifdef LOG_ENABLE_TIMER

private:
  ::std::chrono::steady_clock::time_point start;

public:
  ::std::string name;

  LogTimer(char const * name = "");
  long time();

#endif // LOG_ENABLE_TIMER
};

#endif //FF_LOGGING_H_
/* And continue the abuse of macros */

#define stringify_(...) #__VA_ARGS__
#define stringify(...) stringify_(__VA_ARGS__)

#ifndef NDEBUG // ifdef DEBUG
#ifdef LOG_NO_COLOR
#define log_PRIVATE(level, color, fmt, ...) \
  do { \
    log_time_update_PRIVATE(); \
    fprintf( \
        LOG_FILE, \
        "%s " level " (" __FILE__ \
        ":" stringify(__LINE__) ") (%s): " fmt "%s", \
        LOG_TIME_STR_private, \
        LOG_ORGANIZATION.c_str(), \
        __VA_ARGS__); \
    fflush(LOG_FILE); \
  } while (0)
#else
#define log_PRIVATE(level, color, fmt, ...) \
  do { \
    log_time_update_PRIVATE(); \
    fprintf( \
        LOG_FILE, \
        color "%s " level " (" __FILE__ \
              ":" stringify(__LINE__) ") (%s): " LOG_COLOR_DEFAULT fmt \
                                      "%s", \
        LOG_TIME_STR_private, \
        LOG_ORGANIZATION.c_str(), \
        __VA_ARGS__); \
    fflush(LOG_FILE); \
  } while (0)
#endif // LOG_NO_COLOR
#else
#ifdef LOG_NO_COLOR
#define log_PRIVATE(level, color, fmt, ...) \
  do { \
    log_time_update_PRIVATE(); \
    fprintf( \
        LOG_FILE, \
        "%s " level " (%s): " fmt "%s", \
        LOG_TIME_STR_private, \
        LOG_ORGANIZATION.c_str(), \
        __VA_ARGS__); \
    fflush(LOG_FILE); \
  } while (0)
#else
#define log_PRIVATE(level, color, fmt, ...) \
  do { \
    log_time_update_PRIVATE(); \
    fprintf( \
        LOG_FILE, \
        color "%s " level " (%s): " LOG_COLOR_DEFAULT fmt "%s", \
        LOG_TIME_STR_private, \
        LOG_ORGANIZATION.c_str(), \
        __VA_ARGS__); \
    fflush(LOG_FILE); \
  } while (0)
#endif // LOG_NO_COLOR
#endif // NDEBUG

#ifdef LOG_ENABLE_TRACE
#undef log_trace
#define log_trace(...) \
  log_PRIVATE("TRACE", LOG_COLOR_BLUE, __VA_ARGS__, "\n")
#else
#undef log_trace
#define log_trace(...)
#endif

#ifdef LOG_ENABLE_DEBUG
#undef log_debug
#define log_debug(...) \
  log_PRIVATE("DEBUG", LOG_COLOR_MAGENTA, __VA_ARGS__, "\n")
#else
#undef log_debug
#define log_debug(...)
#endif

#ifndef LOG_DISABLE_INFO
#undef log_info
#define log_info(...) \
  log_PRIVATE(" INFO", LOG_COLOR_CYAN, __VA_ARGS__, "\n")
#else
#undef log_info
#define log_info(...)
#endif

#ifndef LOG_DISABLE_WARN
#undef log_warn
#define log_warn(...) \
  log_PRIVATE(" WARN", LOG_COLOR_YELLOW, __VA_ARGS__, "\n")
#else
#undef log_warn
#define log_warn(...)
#endif

#ifndef LOG_DISABLE_ERROR
#undef log_error
#define log_error(...) \
  log_PRIVATE("ERROR", LOG_COLOR_RED, __VA_ARGS__, "\n")
#else
#undef log_error
#define log_error(...)
#endif

/*
 TODO(kimee): Figure out how to properly do the following, and update line 194 accordingly.
 TODO(kimee): Find a cross platform thread safe way to do this.
    locale_t log_local = duplocale(LC_GLOBAL_LOCALE); \
    log_error("%s", strerror_l(errno, log_local)); \
    freelocale(log_local); \
  */

#ifndef LOG_DISABLE_ERROR
#undef log_perror
#define log_perror() \
  do { \
    log_error("%s", strerror(errno)); \
  } while (0)
#else
#undef log_perror
#define log_perror(...)
#endif

/* log_fatal cannot be disabled. */
#undef log_fatal
#define log_fatal(...) \
  do { \
    log_PRIVATE("FATAL", LOG_COLOR_RED_INVERT, __VA_ARGS__, "\n"); \
    log_stack_trace(); \
    abort(); \
  } while (0)

/**
 * The intention of log_assert is that in correct code, the condition
 * is always false, thus it is safe to remove them during release.
 */
#ifndef NDEBUG
#undef log_assert
#define log_assert(cond, ...) \
  do { \
    if (!(cond)) { \
      log_fatal( \
          "Assertion Failure (" stringify(cond) ") " __VA_ARGS__); \
    } \
  } while (0)
#else
#undef log_assert
#define log_assert(cond, ...)
#endif // NDEBUG

/**
 * The if_debug helper will conditionally enable a statement or block
 * depending on if NDEBUG is set (indicating no debug mode).
 *
 * Most likely dead-code elimination will compile the condition to either
 * a no-op or a non-condition.
 */
#undef if_debug
#ifndef NDEBUG
// unwinding the double-negative: ifdef DEBUG
#define if_debug if (true)
#else
#define if_debug if (false)
#endif // NDEBUG

/**
 * The log timer macros work in conjunction with the LogTimer object.
 *
 * log_time_start creates (and returns) a LogTimer. Its intended to be used
 * with an assignment statement to create a LogTimer, and print out a start
 * message.
 *
 * log_time_update will print out an update message, including time since
 * start, and time since last update (if prior update has occured).
 *
 * log_time_finish prints out a finish message including time since start
 * and time since last update (if prior update has occured).
 *
 * All log_time features are disabled with LOG_DISABLE_TIME, which is
 * currently set in release mode.
 */
#ifndef LOG_ENABLE_TIMER
#define log_time_start(name) LogTimer()
#define log_time_start_ctx(name, ctx) LogTimer()
#define log_time_update(timer, msg) (void)(timer)
#else
#define log_time_start(name) \
  LogTimer((name)); \
  log_PRIVATE( \
      " TIME", LOG_COLOR_GREEN, "Starting Timer \"%s\"", (name), "\n")

#define log_time_start_ctx(name, ctx) \
  LogTimer((name)); \
  log_PRIVATE( \
      " TIME", \
      LOG_COLOR_GREEN, \
      "Starting Timer \"%s\", %s", \
      (name), \
      (ctx), \
      "\n")

#define log_time_update(timer, msg) \
  log_PRIVATE( \
      " TIME", \
      LOG_COLOR_GREEN, \
      "Timer \"%s\": %luus, %s", \
      (timer).name.c_str(), \
      (timer).time(), \
      (msg), \
      "\n")
#endif

/* undefine all of the above macros, so that another inclusion of this file
 * can redefine them. */
#else //LOG_UNCLUDE

#undef LOG_COLOR_RED
#undef LOG_COLOR_RED_INVERT
#undef LOG_COLOR_GREEN
#undef LOG_COLOR_GREEN_INVERT
#undef LOG_COLOR_YELLOW
#undef LOG_COLOR_BLUE
#undef LOG_COLOR_MAGENTA
#undef LOG_COLOR_CYAN
#undef LOG_COLOR_DEFAULT

#ifdef LOG_TIME_FMT_DEFAULT
#undef LOG_TIME_FMT
#undef LOG_TIME_FMT_LEN
#undef LOG_TIME_FMT_DEFAULT
#endif //LOG_TIME_FMT_DEFAULT

#undef _stringify
#undef stringify

#undef log_PRIVATE

#undef LOG_ENABLE_TRACE
#undef log_trace

#undef LOG_ENABLE_DEBUG
#undef log_debug

#undef LOG_DISABLE_INFO
#undef log_info

#undef LOG_DISABLE_WARN
#undef log_warn

#undef LOG_DISABLE_ERROR
#undef log_error
#undef log_perror

#undef log_fatal
#undef log_assert

#undef if_debug

#undef log_time_start
#undef log_time_start_ctx
#undef log_time_update

#undef LOG_UNCLUDE

#endif //LOG_UNCLUDE
