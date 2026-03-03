#include "Signal.h"

#include <cerrno>
#include <cstdint>

using namespace WebSocketCpp;

Signal::Signal()
{
}

void Signal::Fire()
{
    pthread_cond_signal(&m_signalCondition);
}

void Signal::Wait(Mutex& mutex)
{
    pthread_cond_wait(&m_signalCondition, mutex.GetMutex());
}

bool Signal::WaitTimeout(Mutex& mutex, uint64_t ms)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += ms / 1000;
    ts.tv_nsec += (ms % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000)
    {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000;
    }
    return (pthread_cond_timedwait(&m_signalCondition, mutex.GetMutex(), &ts) != ETIMEDOUT);
}
