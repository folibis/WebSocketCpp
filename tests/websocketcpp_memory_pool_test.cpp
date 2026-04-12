#include <gtest/gtest.h>
#include "MemoryPool.h"
#include <thread>
#include <vector>
#include <atomic>

using WebSocketCpp::MemoryPool;

TEST(MemoryPool, AllocateFromEmptyPool) {
    MemoryPool pool(128);
    uint8_t* p = pool.allocate(128);
    ASSERT_NE(p, nullptr);
    pool.free(p);
}

TEST(MemoryPool, AllocateZeroReturnsNull) {
    MemoryPool pool(128);
    EXPECT_EQ(pool.allocate(0), nullptr);
}

TEST(MemoryPool, AllocateTooLargeReturnsNull) {
    MemoryPool pool(64);
    EXPECT_EQ(pool.allocate(65), nullptr);
}

TEST(MemoryPool, AllocateExactSize) {
    MemoryPool pool(64);
    uint8_t* p = pool.allocate(64);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(pool.allocate(1), nullptr);  // pool full
    pool.free(p);
}

TEST(MemoryPool, MultipleSequentialAllocations) {
    MemoryPool pool(300);
    uint8_t* a = pool.allocate(100);
    uint8_t* b = pool.allocate(100);
    uint8_t* c = pool.allocate(100);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    ASSERT_NE(c, nullptr);
    // must not overlap
    EXPECT_NE(a, b);
    EXPECT_NE(b, c);
    EXPECT_NE(a, c);
    pool.free(a);
    pool.free(b);
    pool.free(c);
}

TEST(MemoryPool, PointersAreInsideBuffer) {
    MemoryPool pool(256);
    uint8_t* a = pool.allocate(64);
    uint8_t* b = pool.allocate(64);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    EXPECT_LT(a, b);          // sorted: b starts after a
    EXPECT_GE(b - a, 64);     // at least 64 bytes apart
    pool.free(a);
    pool.free(b);
}

// ─── Free and reuse ──────────────────────────────────────────────────────────

TEST(MemoryPool, FreeNullIsNoOp) {
    MemoryPool pool(64);
    EXPECT_NO_THROW(pool.free(nullptr));
}

TEST(MemoryPool, FreedSpaceIsReused) {
    MemoryPool pool(64);
    uint8_t* a = pool.allocate(64);
    ASSERT_NE(a, nullptr);
    pool.free(a);
    uint8_t* b = pool.allocate(64);
    ASSERT_NE(b, nullptr);
    pool.free(b);
}

TEST(MemoryPool, GapBetweenRegionsIsUsed) {
    MemoryPool pool(200);
    uint8_t* a = pool.allocate(100);   // [0..100)
    uint8_t* b = pool.allocate(100);   // [100..200)
    pool.free(a);                       // gap [0..100)
    uint8_t* c = pool.allocate(50);    // should fill [0..50)
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(c, b - 100);             // c is at offset 0
    pool.free(b);
    pool.free(c);
}

TEST(MemoryPool, TailSpaceUsedAfterLastRegion) {
    MemoryPool pool(200);
    uint8_t* a = pool.allocate(100);   // [0..100)
    uint8_t* b = pool.allocate(50);    // [100..150), tail = [150..200)
    pool.free(a);                       // gap [0..100), tail [150..200)
    // 110 bytes fit neither in gap (100) nor tail (50)
    EXPECT_EQ(pool.allocate(110), nullptr);
    // 50 bytes fit in the gap at the front
    uint8_t* c = pool.allocate(50);
    ASSERT_NE(c, nullptr);
    pool.free(b);
    pool.free(c);
}

TEST(MemoryPool, FullBufferAfterAllFrees) {
    MemoryPool pool(200);
    uint8_t* a = pool.allocate(100);
    uint8_t* b = pool.allocate(50);
    pool.free(a);
    pool.free(b);
    // all space returned — should be able to allocate everything
    uint8_t* full = pool.allocate(200);
    ASSERT_NE(full, nullptr);
    pool.free(full);
}

// ─── Buffer is writable ──────────────────────────────────────────────────────

TEST(MemoryPool, AllocatedMemoryIsWritable) {
    MemoryPool pool(256);
    uint8_t* p = pool.allocate(256);
    ASSERT_NE(p, nullptr);
    memset(p, 0xAB, 256);
    for (int i = 0; i < 256; ++i)
        EXPECT_EQ(p[i], 0xAB);
    pool.free(p);
}

TEST(MemoryPool, AllocationsDoNotOverlap) {
    MemoryPool pool(1024);
    uint8_t* a = pool.allocate(512);
    uint8_t* b = pool.allocate(512);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    memset(a, 0xAA, 512);
    memset(b, 0xBB, 512);
    for (int i = 0; i < 512; ++i) EXPECT_EQ(a[i], 0xAA);
    for (int i = 0; i < 512; ++i) EXPECT_EQ(b[i], 0xBB);
    pool.free(a);
    pool.free(b);
}

// ─── Thread safety ───────────────────────────────────────────────────────────

TEST(MemoryPool, ConcurrentAllocateFree) {
    const int NUM_THREADS    = 8;
    const int OPS_PER_THREAD = 200;
    const int ALLOC_SIZE     = 16;

    MemoryPool pool(NUM_THREADS * OPS_PER_THREAD * ALLOC_SIZE);
    std::atomic<int> errors{0};

    auto worker = [&]() {
        std::vector<uint8_t*> ptrs;
        ptrs.reserve(OPS_PER_THREAD);
        for (int i = 0; i < OPS_PER_THREAD; ++i) {
            uint8_t* p = pool.allocate(ALLOC_SIZE);
            if (p) {
                p[0] = 0xAB;
                if (p[0] != 0xAB) ++errors;
                ptrs.push_back(p);
            }
        }
        for (auto p : ptrs)
            pool.free(p);
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; ++i)
        threads.emplace_back(worker);
    for (auto& t : threads)
        t.join();

    EXPECT_EQ(errors.load(), 0);
    EXPECT_EQ(pool.used_size(), 0u);
}

TEST(MemoryPool, ConcurrentMixedSizes) {
    MemoryPool pool(1024 * 1024);  // 1 MB
    std::atomic<int> errors{0};
    std::size_t sizes[] = {16, 64, 128, 512};

    auto worker = [&](std::size_t sz) {
        std::vector<uint8_t*> ptrs;
        for (int i = 0; i < 100; ++i) {
            uint8_t* p = pool.allocate(sz);
            if (p) {
                memset(p, 0xCD, sz);
                ptrs.push_back(p);
            }
        }
        for (auto p : ptrs)
            pool.free(p);
    };

    std::vector<std::thread> threads;
    for (auto sz : sizes)
        threads.emplace_back(worker, sz);
    for (auto& t : threads)
        t.join();

    EXPECT_EQ(errors.load(), 0);
    EXPECT_EQ(pool.used_size(), 0u);
}
