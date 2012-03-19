

/* Generic Extra stuff (GX)
 * Ideas borrowed liberally from the linux kernel, haproxy, and code all over
 * the place.
 */
#ifndef   GX_H
  #define GX_H

  #include <stdio.h>
  #include <stdlib.h>
  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <fcntl.h>
  #include <errno.h>
  #include <string.h>

  /* Some predefined color escape sequences */
  #define GX_C_NORMAL "\e[0m"
  #define GX_C_PANIC  "\e[38;5;198m"
  #define GX_C_FATAL  "\e[38;5;196m"
  #define GX_C_ERROR  "\e[38;5;160m"
  #define GX_C_WARN   "\e[38;5;202m"
  #define GX_C_INFO   "\e[38;5;252m"
  #define GX_C_DEBUG  "\e[38;5;226m"
  #define GX_C_UNKN   "\e[38;5;27m"
  #define GX_C_DELIM  "\e[38;5;236m"
  #define GX_C_BG     "\e[48;5;234m"
  #define GX_C_REF    "\e[38;5;240m"


  /* Error Handling
   *
   * gx_log(log_level, format, ...)        ->  Log adhoc message
   * gx_log_panic/fatal/...(format,...)    ->  Easier to remember versions of gx_log
   * gx_elog(log_level)                    ->  Log message based on errno
   * E_CHECK(  ...expr...,  ...actions...) ->  If expr returns < 0, performs actions
   *   actions - any of the following separated by semicolons:
   *      E_ERROR/PANIC/...   -> Logs message based on errno
   *      E_RAISE             -> Return -1, errno intact
   *      E_EXIT              -> Exit with errno
   *      E_JMP(label)        -> Goto the given label (usually @ end for error handling)
   *
   *
   * Example:
        char *myvar = NULL;
        if(argc<2) gx_log_info("Must specify at least one file to check.");
        Xn(myvar, E_WARN)
        X(open(argv[1], O_RDONLY), E_PANIC;E_EXIT)

   * Would emit something like this:
   * [info:tst.c:7:]  Must specify at least one file to check.
   * [warning:tst.c:8:myvar]  Bad address
   * [panic:tst.c:9:open]  No such file or directory
   *
   *
   * TODO: gx_elog allowed to accept another qualifier (similar to how perror
   *       is usually used)
   * TODO: configuration or maybe even runtime log-level detection.
   * TODO: configuration or env flag for colors
   * TODO: only emit colorcodes when stderr is going to a terminal
   */

  char gx_expression[256];  // Global for holding expressions for error handling

  #define GX_LOG_PANIC         0
  #define GX_LOG_FATAL         1
  #define GX_LOG_ERROR         2
  #define GX_LOG_WARN          3
  #define GX_LOG_INFO          4
  #define GX_LOG_DEBUG         5

  #define GX_D   GX_C_BG GX_C_DELIM ":" GX_C_REF   // Shorthand for internal use

  #define gx_log_prep_(loglvl) \
      int gx_err_lvl = (loglvl);                                            \
      char *gx_epre;                                                        \
      switch(gx_err_lvl) {                                                  \
          case GX_LOG_PANIC:   gx_epre=GX_C_PANIC "panic";   break;         \
          case GX_LOG_FATAL:   gx_epre=GX_C_FATAL "fatal";   break;         \
          case GX_LOG_ERROR:   gx_epre=GX_C_ERROR "error";   break;         \
          case GX_LOG_WARN:    gx_epre=GX_C_WARN  "warning"; break;         \
          case GX_LOG_INFO:    gx_epre=GX_C_INFO  "info";    break;         \
          case GX_LOG_DEBUG:   gx_epre=GX_C_DEBUG "debug";   break;         \
          default:             gx_epre=GX_C_UNKN  "unknown"; break;         \
      }                                                                     \

  #define GX_LOG_FORMAT_BASE_ \
      GX_C_BG GX_C_DELIM "[%s" GX_D "%s" GX_D "%d" GX_D "%s" GX_C_DELIM "]" GX_C_NORMAL "  "

  #define gx_log_simple(loglvl, msg) {                                      \
      gx_log_prep_(loglvl)                                                  \
      fprintf(stderr,GX_LOG_FORMAT_BASE_ "%s\n", gx_epre, __FILE__,         \
              __LINE__, gx_expression, msg);                                \
  }

  #define gx_log(loglvl,msg,...) {                                          \
      gx_log_prep_(loglvl)                                                  \
      fprintf(stderr,GX_LOG_FORMAT_BASE_, gx_epre, __FILE__,                \
              __LINE__, gx_expression);                                     \
      fprintf(stderr,msg, ##__VA_ARGS__ );                                  \
      fprintf(stderr,"\n");                                                 \
  }

  #define gx_elog(loglvl) {                                                 \
      char gx_err_buf[1024];                                                \
      strerror_r(gx_errno, gx_err_buf, sizeof(gx_err_buf)-1);               \
      gx_log_simple((loglvl), gx_err_buf);                                  \
  }

  #define gx_log_panic(msg,...)   gx_log(GX_LOG_PANIC,  msg, ##__VA_ARGS__);
  #define gx_log_fatal(msg,...)   gx_log(GX_LOG_FATAL,  msg, ##__VA_ARGS__);
  #define gx_log_error(msg,...)   gx_log(GX_LOG_ERROR,  msg, ##__VA_ARGS__);
  #define gx_log_warn(msg, ...)   gx_log(GX_LOG_WARN,   msg, ##__VA_ARGS__);
  #define gx_log_info(msg, ...)   gx_log(GX_LOG_INFO,   msg, ##__VA_ARGS__);
  #define gx_log_debug(msg,...)   gx_log(GX_LOG_DEBUG,  msg, ##__VA_ARGS__);

  #define X(expr, actions)  {                                              \
      if(gx_unlikely((int)(expr) < 0)) {                                   \
          int gx_errno = errno;                                            \
          gx_expression[0] = '\0';                                         \
          (void) snprintf(gx_expression, 255, #expr);                      \
          char *gx_exprstrt = strchr(gx_expression,'(');                   \
          if(gx_exprstrt) gx_exprstrt[0]='\0';                             \
          actions                                                          \
      }                                                                    \
  }
  #define Xn(expr, actions)  {                                             \
      if(gx_unlikely((expr)==NULL)) {                                      \
          if(errno == 0) errno = EFAULT;                                   \
          int gx_errno = errno;                                            \
          gx_expression[0] = '\0';                                         \
          snprintf(gx_expression, 255, #expr);                             \
          char *gx_exprstrt = strchr(gx_expression,'(');                   \
          if(gx_exprstrt) gx_exprstrt[0]='\0';                             \
          actions                                                          \
      }                                                                    \
  }

  // Used as actions for E_CHECK
  #define E_PANIC                 gx_elog(GX_LOG_PANIC)
  #define E_FATAL                 gx_elog(GX_LOG_FATAL)
  #define E_ERROR                 gx_elog(GX_LOG_ERROR)
  #define E_WARN                  gx_elog(GX_LOG_WARN)
  #define E_INFO                  gx_elog(GX_LOG_INFO)
  #define E_DEBUG                 gx_elog(GX_LOG_DEBUG)

  #define E_RAISE                 return(-1);
  #define E_EXIT                  exit(gx_errno);
  #define E_JMP(errjmplabel)      goto errjmplabel;




  /* Misc */
  #ifndef MIN
  #define MIN(m,n) ((m) < (n) ? (m) : (n))
  #endif

  #ifndef MAX
  #define MAX(m,n) ((m) > (n) ? (m) : (n))
  #endif


/* Compiler Optimization Cues
 *
 * gx_likely(stmt)   -> Tell compiler the branch is likely
 * gx_unlikely(stmt) -> Tell compiler the branch is rare
 * gx_cold_f         -> As part of a function signature- infrequent function
 * gx_hot_f          -> Frequently called function
 * gx_pure_f         -> Pure function- side-effect free
 * gx_finline_f      -> Force the function to be inlined regardless of compiler settings
 *
 * TODO:
 * gx_prefetch_rw_keep
 * gx_prefetch_rw_toss
 * gx_prefetch_ro_keep
 * gx_prefetch_ro_toss
 *
 */

  #if          __GNUC__ > 3
    #ifndef    gx_likely
      #define  gx_likely(x)   __builtin_expect(!!(x), 1)
      #define  gx_unlikely(x) __builtin_expect(!!(x), 0)
    #endif
    #ifndef    gx_cold_f
      #define  gx_pure_f      __attribute__((pure))
      #define  gx_cold_f      __attribute__((cold))
      #define  gx_hot_f       __attribute__((hot))
    #endif
    #ifndef    gx_finline_f
      #define  gx_finline_f   inline __attribute__((always_inline))
    #endif
  #else
    #ifndef    gx_likely
      #define  gx_likely(x)   (x)
      #define  gx_unlikely(x) (x)
    #endif
    #ifndef    gx_cold_f
      #define  gx_cold_f
      #define  gx_hot_f
      #define  gx_pure_f
    #endif
    #ifndef    gx_finline_f
    #define    gx_finline_f   inline
    #endif
  #endif
#endif
