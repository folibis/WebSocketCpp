#include "Platform.h"

#include "unistd.h"

void WebSocketCpp::SleepMs(uint32_t delay)
{
    usleep(delay * 1000);
}

void WebSocketCpp::Sleep(uint32_t delay)
{
    sleep(delay);
}
