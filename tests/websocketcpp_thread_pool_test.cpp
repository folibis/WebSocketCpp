/*
 * Copyright (c) 2026 ruslan@muhlinin.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

#include "ThreadPool.h"

using namespace WebSocketCpp;

TEST(ThreadPoolTest, InitAndRun)
{
    ThreadPool<> pool;
    ASSERT_TRUE(pool.Init([]() {}));
    ASSERT_TRUE(pool.Run());
    ASSERT_TRUE(pool.IsRunning());
    pool.Stop();
    ASSERT_FALSE(pool.IsRunning());
}

TEST(ThreadPoolTest, DoubleRunFails)
{
    ThreadPool<> pool;
    pool.Init([]() {});
    ASSERT_TRUE(pool.Run());
    ASSERT_FALSE(pool.Run());
    pool.Stop();
}

TEST(ThreadPoolTest, DoubleInitFails)
{
    ThreadPool<> pool;
    ASSERT_TRUE(pool.Init([]() {}));
    pool.Run();
    ASSERT_FALSE(pool.Init([]() {}));
    pool.Stop();
}

TEST(ThreadPoolTest, DoubleStopSafe)
{
    ThreadPool<> pool;
    pool.Init([]() {});
    pool.Run();
    pool.Stop();
    ASSERT_NO_THROW(pool.Stop());
}

TEST(ThreadPoolTest, SubmitNoArgs)
{
    std::atomic<int> counter{0};
    ThreadPool<>     pool;
    pool.Init([&counter]() { counter++; });
    pool.Run();

    const int count = 10;
    for (int i = 0; i < count; i++)
    {
        pool.Submit();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    pool.Stop();

    EXPECT_EQ(count, counter.load());
}

TEST(ThreadPoolTest, SubmitWithArgs)
{
    std::atomic<int> sum{0};
    ThreadPool<int>  pool;
    pool.Init([&sum](int val) { sum += val; });
    pool.Run();

    const int count = 10;
    for (int i = 1; i <= count; i++)
    {
        pool.Submit(i);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    pool.Stop();

    EXPECT_EQ(55, sum.load());
}

TEST(ThreadPoolTest, SubmitMultipleArgs)
{
    std::atomic<int>          sum{0};
    ThreadPool<int, int, int> pool;
    pool.Init([&sum](int a, int b, int c) { sum += a + b + c; });
    pool.Run();

    pool.Submit(1, 2, 3);
    pool.Submit(4, 5, 6);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    pool.Stop();

    EXPECT_EQ(21, sum.load());
}

TEST(ThreadPoolTest, SubmitBeforeRunDropped)
{
    std::atomic<int> counter{0};
    ThreadPool<>     pool;
    pool.Init([&counter]() { counter++; });

    pool.Submit();

    pool.Run();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    pool.Stop();

    EXPECT_EQ(0, counter.load());
}

TEST(ThreadPoolTest, SubmitAfterStopDropped)
{
    std::atomic<int> counter{0};
    ThreadPool<>     pool;
    pool.Init([&counter]() { counter++; });
    pool.Run();
    pool.Stop();

    pool.Submit();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_EQ(0, counter.load());
}

TEST(ThreadPoolTest, MultipleThreads)
{
    std::atomic<int> counter{0};
    ThreadPool<>     pool(4);
    pool.Init([&counter]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        counter++;
    });
    pool.Run();

    const int count = 20;
    for (int i = 0; i < count; i++)
    {
        pool.Submit();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    pool.Stop();

    EXPECT_EQ(count, counter.load());
}

TEST(ThreadPoolTest, MoveArgs)
{
    std::atomic<size_t>          total{0};
    ThreadPool<std::vector<int>> pool;
    pool.Init([&total](std::vector<int> v) {
        for (auto& i : v)
        {
            total += i;
        }
    });
    pool.Run();

    pool.Submit(std::vector<int>{1, 2, 3});
    pool.Submit(std::vector<int>{4, 5, 6});

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    pool.Stop();

    EXPECT_EQ(21, total.load());
}

TEST(ThreadPoolTest, ThreadCount)
{
    ThreadPool<> pool(4);
    pool.Init([]() {});
    pool.Run();
    EXPECT_EQ(4, pool.ThreadCount());
    pool.Stop();
}

TEST(ThreadPoolTest, QueueSize)
{
    std::mutex              mtx;
    std::condition_variable cv;
    bool                    blocked = true;

    ThreadPool<> pool(1);
    pool.Init([&]() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&] { return !blocked; });
    });
    pool.Run();

    pool.Submit();
    pool.Submit();
    pool.Submit();

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_GT(pool.QueueSize(), 0);

    {
        std::unique_lock<std::mutex> lock(mtx);
        blocked = false;
    }
    cv.notify_all();

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    pool.Stop();
    EXPECT_EQ(0, pool.QueueSize());
}

TEST(ThreadPoolTest, ConcurrentSubmit)
{
    std::atomic<int> counter{0};
    const int        thread_count     = 10;
    const int        tasks_per_thread = 100;
    ThreadPool<int>  pool(4);
    pool.Init([&counter](int val) {
        counter += val;
    });
    pool.Run();

    std::vector<std::thread> submitters(thread_count);
    for (int i = 0; i < thread_count; i++)
    {
        submitters[i] = std::thread([&pool, tasks_per_thread]() {
            for (int j = 0; j < tasks_per_thread; j++)
            {
                pool.Submit(1);
            }
        });
    }
    for (auto& t : submitters)
    {
        t.join();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    pool.Stop();

    EXPECT_EQ(thread_count * tasks_per_thread, counter.load());
}

TEST(ThreadPoolTest, ConcurrentSubmitWithHeavyTask)
{
    std::atomic<int> counter{0};
    const int        task_count   = 50;
    const int        pool_threads = 4;
    const int        task_ms      = 10;
    ThreadPool<int>  pool(pool_threads);
    pool.Init([&counter, task_ms](int val) {
        std::this_thread::sleep_for(std::chrono::milliseconds(task_ms));
        counter += val;
    });
    pool.Run();

    for (int i = 0; i < task_count; i++)
    {
        pool.Submit(1);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds((task_count / pool_threads + 1) * task_ms + 200));
    pool.Stop();

    EXPECT_EQ(task_count, counter.load());
}

TEST(ThreadPoolTest, StopWhileBusy)
{
    std::atomic<int> counter{0};
    ThreadPool<>     pool(4);
    pool.Init([&counter]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        counter++;
    });
    pool.Run();

    const int count = 20;
    for (int i = 0; i < count; i++)
    {
        pool.Submit();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    pool.Stop();

    EXPECT_FALSE(pool.IsRunning());
    EXPECT_GT(counter.load(), 0);
}

TEST(ThreadPoolTest, OrderOfExecution)
{
    std::vector<int> order;
    std::mutex       order_mtx;
    ThreadPool<int>  pool(1);
    pool.Init([&order, &order_mtx](int val) {
        std::unique_lock<std::mutex> lock(order_mtx);
        order.push_back(val);
    });
    pool.Run();

    const int count = 10;
    for (int i = 0; i < count; i++)
    {
        pool.Submit(i);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    pool.Stop();

    ASSERT_EQ(count, (int)order.size());
    for (int i = 0; i < count; i++)
    {
        EXPECT_EQ(i, order[i]) << "task #" << i << " out of order";
    }
}

TEST(ThreadPoolTest, RestartPool)
{
    std::atomic<int> counter{0};
    ThreadPool<>     pool;
    pool.Init([&counter]() { counter++; });

    // first run
    pool.Run();
    for (int i = 0; i < 5; i++)
    {
        pool.Submit();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    pool.Stop();
    EXPECT_EQ(5, counter.load());

    // second run
    pool.Init([&counter]() { counter++; });
    pool.Run();
    for (int i = 0; i < 5; i++)
    {
        pool.Submit();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    pool.Stop();
    EXPECT_EQ(10, counter.load());
}

TEST(ThreadPoolTest, NoTaskLost)
{
    const int        task_count = 1000;
    std::atomic<int> counter{0};
    ThreadPool<>     pool(8);
    pool.Init([&counter]() { counter++; });
    pool.Run();

    for (int i = 0; i < task_count; i++)
    {
        pool.Submit();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    pool.Stop();

    EXPECT_EQ(task_count, counter.load());
}

TEST(ThreadPoolTest, MoveOnlyArgs)
{
    std::atomic<size_t>              total{0};
    ThreadPool<std::unique_ptr<int>> pool(2);
    pool.Init([&total](std::unique_ptr<int> val) {
        total += *val;
    });
    pool.Run();

    for (int i = 1; i <= 10; i++)
    {
        pool.Submit(std::make_unique<int>(i));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    pool.Stop();

    EXPECT_EQ(55, total.load());
}
