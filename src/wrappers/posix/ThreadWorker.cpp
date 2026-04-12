#include "ThreadWorker.h"

using namespace WebSocketCpp;

ThreadWorker::ThreadWorker()
{
}

ThreadWorker::~ThreadWorker()
{
    Stop();
}

void ThreadWorker::SetFunction(const std::function<ThreadRoutine>& func)
{
    m_func = func;
}

void ThreadWorker::SetFinishFunction(const std::function<ThreadFinishRoutine>& func)
{
    m_funcFinish = func;
}

bool ThreadWorker::Start()
{
    if (m_isRunning)
    {
        return true;
    }

    ClearError();
    m_isRunning = true;

    m_thread = std::thread([this]() {
        void* res = nullptr;
        if (m_func)
        {
            res = m_func(m_isRunning);
        }
        SetStop();
        if (m_funcFinish)
        {
            m_funcFinish(res);
        }
    });

    return true;
}

void ThreadWorker::Stop(bool wait)
{
    if (m_isRunning)
    {
        m_isRunning = false;
    }
    if (wait && m_thread.joinable())
    {
        m_thread.join();
    }
}

void ThreadWorker::StopNoWait()
{
    m_isRunning = false;
}

void ThreadWorker::Wait()
{
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

void ThreadWorker::SetStop()
{
    m_isRunning = false;
}
