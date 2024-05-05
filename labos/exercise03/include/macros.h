#pragma once

#define TRACE 6
#define DEBUG 5
#define INFO 4
#define WARN 3
#define ERROR 2
#define FATAL 1

#define LOG_LVL DEBUG

// #if defined(__has_feature)
// #if __has_feature(thread_sanitizer)
// #define TSAN_ENABLED
// #endif
// #endif

#ifdef __SANITIZE_THREAD__
#define TSAN_ENABLED
#endif

#ifdef TSAN_ENABLED

extern void AnnotateBenignRaceSized(const char *f, int l, void *mem,
                                    unsigned int size, const char *desc);
extern void AnnotateHappensAfter(const char *f, int l, void *addr);

#define TSAN_ANNOTE_BENIGN_RACE_SIZED(addr, size, desc)                        \
  AnnotateBenignRaceSized(__FILE__, __LINE__, (void *)addr, size, desc)

#define TSAN_ANNOTE_HAPPENS_AFTER(addr)                                        \
  AnnotateHappensAfter(__FILE__, __LINE__, (void *)addr)

#else

#define TSAN_ANNOTE_BENIGN_RACE_SIZED(addr, size, desc)

#define TSAN_ANNOTE_HAPPENS_AFTER(addr)

#endif
