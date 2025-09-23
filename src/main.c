#include "logging/logging.h"

void terminate_prepare();

int main()
{
    init_logging("log.log", DEBUG);

    terminate_prepare();
    return 0;
}

void terminate_prepare()
{
    close_log_file();
}
