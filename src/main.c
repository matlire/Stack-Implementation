#include "logging/logging.h"
#include "stack/stack.h"

void terminate_prepare();

DEFINE_STACK_PRINTER_SIMPLE(int, "%d ")

int main()
{
    init_logging("log.log", DEBUG);

    STACK_INIT(sint, int, print_int);

    STACK_PUSH(&sint, 1);
    stack_print(&sint);
    printf("\n");

    STACK_PUSH(&sint, 2);
    stack_print(&sint);
    printf("\n");

    int a = 6;
    STACK_PUSH(&sint, a);

    STACK_PUSH(&sint, 3);
    stack_print(&sint);

    STACK_PUSH(&sint, 4);

    STACK_DUMP(INFO, &sint, OK, "Test");

    STACK_POP_T(&sint, int, i);
    printf("%d\n", i);
    STACK_POP(&sint, i);
    printf("%d\n", i);
    stack_print(&sint);
    printf("\n");

    sint.data = NULL;
    STACK_PUSH(&sint, 4);
    
    stack_dtor(&sint);

    terminate_prepare();
    return 0;
}

void terminate_prepare()
{
    close_log_file();
}
