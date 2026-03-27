/**
 * @file xvr_debug.h
 * @brief Debug utilities and macros for XVR compiler development
 *
 * This header provides debugging utilities that can be enabled/disabled
 * at compile time using build flags.
 *
 * Usage:
 *   - Compile with -DDEBUG to enable debug output
 *   - Use XVR_DEBUG_MSG() for debug messages
 *   - Use XVR_ASSERT() for runtime assertions
 *   - Use XVR_TRACE() for function call tracing
 *
 * Example:
 *   make DEBUG=1           # Enable debug mode
 *   make DEBUG=1 VERBOSE=1 # Enable with verbose output
 */

#ifndef XVR_DEBUG_H
#define XVR_DEBUG_H

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Main debug enable flag
 *
 * Set via -DDEBUG or -DXVR_DEBUG at compile time
 */
#ifdef DEBUG
#    define XVR_DEBUG_ENABLED 1
#elif defined(XVR_DEBUG)
#    define XVR_DEBUG_ENABLED XVR_DEBUG
#else
#    define XVR_DEBUG_ENABLED 0
#endif

/**
 * @brief Enable debug assertions
 *
 * Set via -DDEBUG_ASSERTIONS=1 or use make DEBUG_ASSERTIONS=1
 */
#ifndef DEBUG_ASSERTIONS
#    define XVR_DEBUG_ASSERTIONS 0
#else
#    define XVR_DEBUG_ASSERTIONS 1
#endif

/**
 * @brief Enable function call tracing
 *
 * Set via -DXVR_TRACE=1
 */
#ifndef XVR_TRACE_ENABLED
#    define XVR_TRACE_ENABLED 0
#endif

/**
 * @brief Debug output macro
 *
 * Usage: XVR_DEBUG_MSG("format", args...);
 *
 * Example:
 *   XVR_DEBUG_MSG("Processing node at %p\n", (void*)node);
 */
#if XVR_DEBUG_ENABLED
#    define XVR_DEBUG_MSG(...)                                          \
        do {                                                            \
            fprintf(stderr, "[XVR-DEBUG] %s:%d: ", __FILE__, __LINE__); \
            fprintf(stderr, __VA_ARGS__);                               \
        } while (0)
#else
#    define XVR_DEBUG_MSG(...) ((void)0)
#endif

/**
 * @brief Debug print macro (simpler version)
 *
 * Usage: XVR_DPRINT(expr);
 *
 * Example:
 *   XVR_DPRINT(node->type);
 */
#if XVR_DEBUG_ENABLED
#    define XVR_DPRINT(expr)                                               \
        do {                                                               \
            fprintf(stderr, "[XVR-DEBUG] %s:%d %s = ", __FILE__, __LINE__, \
                    #expr);                                                \
            print_debug_val(expr);                                         \
            fprintf(stderr, "\n");                                         \
        } while (0)
#else
#    define XVR_DPRINT(expr) ((void)0)
#endif

/**
 * @brief Runtime assertion macro
 *
 * Usage: XVR_ASSERT(condition, "error message");
 *
 * Example:
 *   XVR_ASSERT(node != NULL, "node must not be NULL");
 */
#if XVR_DEBUG_ASSERTIONS
#    define XVR_ASSERT(cond, msg)                                     \
        do {                                                          \
            if (!(cond)) {                                            \
                fprintf(stderr, "[XVR-ASSERT] %s:%d: %s\n", __FILE__, \
                        __LINE__, msg);                               \
                abort();                                              \
            }                                                         \
        } while (0)
#else
#    define XVR_ASSERT(cond, msg) ((void)0)
#endif

/**
 * @brief Function entry/exit tracing
 *
 * Usage: XVR_TRACE_ENTER(); at function start
 *        XVR_TRACE_EXIT(); at function end
 *
 * Example:
 *   void foo() {
 *       XVR_TRACE_ENTER();
 *       // function body
 *       XVR_TRACE_EXIT();
 *   }
 */
#if XVR_TRACE_ENABLED
#    define XVR_TRACE_ENTER()                                         \
        do {                                                          \
            fprintf(stderr, "[XVR-TRACE] --> %s (%s:%d)\n", __func__, \
                    __FILE__, __LINE__);                              \
        } while (0)

#    define XVR_TRACE_EXIT()                                          \
        do {                                                          \
            fprintf(stderr, "[XVR-TRACE] <-- %s (%s:%d)\n", __func__, \
                    __FILE__, __LINE__);                              \
        } while (0)

#    define XVR_TRACE_MSG(msg)                                      \
        do {                                                        \
            fprintf(stderr, "[XVR-TRACE] %s: %s\n", __func__, msg); \
        } while (0)
#else
#    define XVR_TRACE_ENTER() ((void)0)
#    define XVR_TRACE_EXIT() ((void)0)
#    define XVR_TRACE_MSG(msg) ((void)0)
#endif

/**
 * @brief Error logging with context
 *
 * Usage: XVR_ERROR("component", "error message");
 *
 * Example:
 *   XVR_ERROR("Lexer", "Unexpected token");
 */
#if XVR_DEBUG_ENABLED
#    define XVR_ERROR(comp, msg)                                       \
        do {                                                           \
            fprintf(stderr, "[XVR-ERROR] %s: %s (%s:%d)\n", comp, msg, \
                    __FILE__, __LINE__);                               \
        } while (0)
#else
#    define XVR_ERROR(comp, msg) ((void)0)
#endif

/**
 * @brief Warning logging
 *
 * Usage: XVR_WARN("component", "warning message");
 */
#if XVR_DEBUG_ENABLED
#    define XVR_WARN(comp, msg)                                       \
        do {                                                          \
            fprintf(stderr, "[XVR-WARN] %s: %s (%s:%d)\n", comp, msg, \
                    __FILE__, __LINE__);                              \
        } while (0)
#else
#    define XVR_WARN(comp, msg) ((void)0)
#endif

/**
 * @brief Performance measurement start
 *
 * Usage:
 *   XVR_TIMER_START(my_timer);
 *   // code to measure
 *   XVR_TIMER_END(my_timer);
 */
#if XVR_DEBUG_ENABLED
#    include <time.h>

typedef struct {
    clock_t start;
    const char* name;
} XVR_Timer;

#    define XVR_TIMER_START(name)                        \
        do {                                             \
            static XVR_Timer _timer_##name = {0, #name}; \
            _timer_##name.start = clock();               \
        } while (0)

#    define XVR_TIMER_END(name)                                          \
        do {                                                             \
            static XVR_Timer _timer_##name = {0, #name};                 \
            clock_t _end = clock();                                      \
            double _elapsed =                                            \
                ((double)(_end - _timer_##name.start)) / CLOCKS_PER_SEC; \
            fprintf(stderr, "[XVR-TIMER] %s: %.6f seconds\n", #name,     \
                    _elapsed);                                           \
        } while (0)
#else
#    define XVR_TIMER_START(name) ((void)0)
#    define XVR_TIMER_END(name) ((void)0)
#endif

/**
 * @brief Conditional compilation helper for development vs production
 */
#if XVR_DEBUG_ENABLED
#    define XVR_DEV_CODE(code) code
#    define XVR_PROD_CODE(code)
#else
#    define XVR_DEV_CODE(code)
#    define XVR_PROD_CODE(code) code
#endif

#endif /* XVR_DEBUG_H */