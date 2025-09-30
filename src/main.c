#include "logging/logging.h"
#include "stack/stack.h"

void terminate_prepare();

DEFINE_STACK_PRINTER_SIMPLE(int, "%d ")

typedef struct
{
    double x;
    double y;
} Point_t;

DEFINE_STACK_PRINTER(Point_t, {
    return fprintf(__OUT__, "(%.3lf, %.3lf) ", __PTR->x, __PTR->y);
})

DEFINE_STACK_SPRINTER(Point_t, {
    return snprintf(__dst + __len, __dstsz - __len, "(%.3lf, %.3lf) ", __PTR->x, __PTR->y);
})

int main()
{
    init_logging("log.log", DEBUG);

    // Simple type stack
    STACK_INIT(sint, int);
    
    STACK_PUSH(sint, 1);
    stack_print(sint);

    STACK_PUSH(sint, 2);
    stack_print(sint);

    int a = 6;
    STACK_PUSH(sint, a);

    STACK_PUSH(sint, 3);
    stack_print(sint);
    STACK_PUSH(sint, 4);
    stack_print(sint);

    STACK_DUMP(INFO, sint, OK, "Test simple type stack dump");

    STACK_POP_T(sint, int, i);
    printf("Popped: %d\n", i);
    STACK_POP(sint, i);
    printf("Popped: %d\n", i);
    stack_print(sint);

    /*STACK_POP(sint, i);
    printf("Popped: %d\n", i);
    STACK_POP(sint, i);
    printf("Popped: %d\n", i);
    STACK_POP(sint, i);
    printf("Popped: %d\n", i);
    STACK_POP(sint, i);
    printf("Popped: %d\n", i);
    STACK_POP(sint, i);
    printf("Popped: %d\n", i);*/

    STACK_PUSH(sint, 4);
    stack_print(sint);
    stack_dtor(sint);

    // Struct stack
    STACK_INIT(spoints, Point_t);

    STACK_PUSH_S(spoints, Point_t, .x = 1.0,  .y = 2.0);

    Point_t p0 = {  };
    STACK_PUSH_S(spoints, Point_t, .x = 3.5,  .y = -4.25);
    STACK_POP(spoints, p0);
    STACK_PUSH_S(spoints, Point_t, .x = -7.0, .y = 0.5);

    Point_t p1 = { .x = 3.543, .y = -0.00923 };
    STACK_PUSH_VAR(spoints, p1);

    stack_print(spoints);
    STACK_DUMP(INFO, spoints, OK, "Test struct stack dump");    
    stack_dtor(spoints);

    terminate_prepare();
    return 0;
}

void terminate_prepare()
{
    close_log_file();
}
