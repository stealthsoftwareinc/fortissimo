
/* C and POSIX Headers */

/* C++ Headers */
#include <cerrno>
#include <functional>

/* 3rd Party Headers */
#include <gtest/gtest.h>

#define LOG_ENABLE_TRACE
#define LOG_ENABLE_DEBUG
#include <ff/logging.h>

TEST(logging, logging) {
  log_trace("trace logs are blue");
  log_debug("debug logs are magenta");
  log_info("info logs are cyan");
  log_warn("warn logs are orange");
  log_error("error logs are red");
}

TEST(logging, perror) {
  errno = EINVAL;
  log_perror();
}

TEST(logging, indicateDebugChecks) {
  if_debug {
    log_info("debug checks enabled");
  }
  else {
    log_info("debug checks disabled");
  }
}

TEST(logging, testLoggingTime) {
  /* function that takes a long time, for testing timings. */
  std::function<unsigned long(unsigned long)> fib;
  fib = [&fib](unsigned long n) -> unsigned long {
    if (n == 0UL || n == 1UL) {
      return 1UL;
    } else {
      return fib(n - 1) + fib(n - 2);
    }
  };

  LogTimer timer = log_time_start("fib");
  fib(25);
  log_time_update(timer, "fib(25)");
  fib(26);
  log_time_update(timer, "fib(26)");
  fib(28);
  log_time_update(timer, "fib(28)");

  log_info("sizeof(LogTimer) = %zu", sizeof(LogTimer));
};
