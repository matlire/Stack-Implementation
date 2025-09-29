#ifdef __DEBUG__
  #define IFDEBUG(...) __VA_ARGS__
#else
  #define IFDEBUG(...)
#endif

#ifndef STACK_H
#define STACK_H

#include <stdint.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../logging/logging.h"

#ifdef LOGGING_H
  #define IFLOG(level, fmt, ...)  log_printf((level), (fmt), ##__VA_ARGS__)
#else
  #include <stdio.h>
  #define IFLOG(level, fmt, ...)  printf((fmt), ##__VA_ARGS__)
#endif

#define INITIAL_CAPACITY 4
#define STR_CAT_MAX_SIZE 4096

#define STACK_CANARY ((long long)0xDEDEBADEDAEBADDLL)

typedef struct
{
    const char* name;
    IFDEBUG(
        const char* func;
        const char* file;
        int   line;
    )
} stack_info_t;

typedef struct
{
    const char* elem_name;

    size_t elem_size;
    size_t elem_align;
    size_t elem_stride;
} element_info_t;

typedef int  (*stack_sprint_fn)(char* dst, size_t dstsz, const void* elem);
typedef void (*stack_print_fn) (FILE* file, const void* elem);

typedef struct
{
    stack_info_t   stack_info;
    element_info_t elem_info;

    void*  data;
    size_t size;
    size_t capacity;

    void*  raw_data;
    size_t alloc_size;

    stack_print_fn printer; 
    stack_sprint_fn sprinter; 
} stack_t;

typedef enum 
{
    OK          = 0,
    ERR_BAD_ARG = 1,
    ERR_CORRUPT = 2,
    ERR_ALLOC   = 3,
} err_t;

extern const char * const err_msgs[];
const char* err_str(const err_t e);

err_t stack_ctor(stack_t* st, element_info_t info,
                 const stack_print_fn printer, const stack_sprint_fn sprinter,
                 const stack_info_t stack_info);
err_t stack_dtor(stack_t* st);

err_t stack_push(stack_t* st, const void* elem);
err_t stack_pop (stack_t* st, void* elem);

err_t stack_print(const stack_t* st);

err_t stack_dump (logging_level level, const stack_t* st, err_t code, const char* comment);
#define STACK_DUMP(level, st, code, comment) \
    stack_dump((level), (st), (code), (comment))

err_t stack_verify(const stack_t* st);

#define ELEMENT_INFO_INIT(T)    ((element_info_t) { .elem_name = #T, .elem_size = sizeof(T),     \
                                                    .elem_align = alignof(T), .elem_stride = 0 })

#define STACK_INFO_INIT(m_name) ((stack_info_t) { .name = #m_name,                        \
                                                   IFDEBUG(.func = __PRETTY_FUNCTION__, \
                                                  .file = __FILE__, .line = __LINE__) })

#define STACK_INIT(m_name, T, printer)                                              \
    stack_t m_name = {  };                                                          \
    (void)stack_ctor(&(m_name), ELEMENT_INFO_INIT(T), (printer), sprint_##T,        \
                     STACK_INFO_INIT(m_name))

#define DEFINE_STACK_PRINTER_SIMPLE(T, FMT)                                     \
    static void print_##T(FILE* __OUT__, const void* __PTR)                     \
    {                                                                           \
        fprintf(__OUT__, FMT, *(const T*)__PTR);                                \
    }                                                                           \
    static int sprint_##T(char* __dst, size_t __dstsz, const void* __PTR)       \
    {                                                                           \
        size_t __len = (__dst && __dstsz) ? strnlen(__dst, __dstsz) : 0;        \
        if (__len >= __dstsz) return 0;                                         \
        return snprintf(__dst + __len, __dstsz - __len,                         \
                        FMT, *(const T*)__PTR);                                 \
    }

#define DEFINE_STACK_PRINTER(T, BODY)                       \
    static void print_##T(FILE* __OUT__, const void* __VP)  \
    {                                                       \
        const T* __PTR = (const T*)__VP;                    \
        BODY;                                               \
    }

#define DEFINE_STACK_SPRINTER(T, BODY)                                        \
    static int sprint_##T(char* __dst, size_t __dstsz, const void* __VP)      \
    {                                                                         \
        const T* __PTR = (const T*)__VP;                                      \
        size_t __len = (__dst && __dstsz) ? strnlen(__dst, __dstsz) : 0;      \
        if (__len >= __dstsz) return 0;                                       \
        BODY;                                                                 \
    }

#define STACK_PUSH_T(S, T, VAL)          \
    stack_push((S), &(T){ (VAL) })

#define STACK_PUSH_S(S, T, ...)          \
    stack_push((S), &(T){ __VA_ARGS__ })

#define STACK_POP_T(S, T, m_name) \
    T m_name; (void)stack_pop((S), &(m_name))

#define STACK_PUSH(S, V)                                                   \
    _Generic((V),                                                          \
        char:                 stack_push((S), &(char){(V)}),               \
        signed char:          stack_push((S), &(signed char){(V)}),        \
        unsigned char:        stack_push((S), &(unsigned char){(V)}),      \
        short:                stack_push((S), &(short){(V)}),              \
        unsigned short:       stack_push((S), &(unsigned short){(V)}),     \
        int:                  stack_push((S), &(int){(V)}),                \
        unsigned:             stack_push((S), &(unsigned){(V)}),           \
        long:                 stack_push((S), &(long){(V)}),               \
        unsigned long:        stack_push((S), &(unsigned long){(V)}),      \
        long long:            stack_push((S), &(long long){(V)}),          \
        unsigned long long:   stack_push((S), &(unsigned long long){(V)}), \
        float:                stack_push((S), &(float){(V)}),              \
        double:               stack_push((S), &(double){(V)}),             \
        long double:          stack_push((S), &(long double){(V)})         \
    )

#define STACK_POP(S, VAR) \
    stack_pop((S), &(VAR))

#define STACK_CHECK(level, cond, stack, errcode, fmt, ...)      \
    if (!CHECK((level), (cond), (fmt), ##__VA_ARGS__)) {        \
        char buf[STR_CAT_MAX_SIZE] = {  };                      \
        (void)snprintf(buf, sizeof(buf), (fmt), ##__VA_ARGS__); \
        stack_dump((level), (stack), (errcode), (buf));         \
        return (errcode);                                       \
    }

#define STACK_VERIFY(st)                   \
    do {                                   \
        err_t sv_res = stack_verify((st)); \
        if (sv_res != OK)                  \
        {                                  \
            printf("%s", err_str(sv_res)); \
            exit(1);                       \
            return (sv_res);               \
        }                                  \
    } while (0)
    

#endif
