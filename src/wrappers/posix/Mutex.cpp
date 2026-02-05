#include "Mutex.h"

using namespace WebSocketCpp;

pthread_mutex_t* Mutex::GetMutex()
{
    return &m_writeMutex;
}
