#include "player/track_queue.h"

#include <gtest/gtest.h>
#include <chrono>
#include <stop_token>
#include <thread>


TEST(TrackQueueTest, PopReturnsTracksInOrder) {
    track_queue q;
    std::stop_token token;
    q.push({.webpage_url = "1"});
    q.push({.webpage_url = "2"});
    q.push({.webpage_url = "3"});
    auto res = q.pop(token);

    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->webpage_url, "1");

    res = q.pop(token);

    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->webpage_url, "2");

    res = q.pop(token);

    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->webpage_url, "3");

    EXPECT_TRUE(q.snapshot().empty());
}

TEST(TrackQueueTest, SnapshotIsIndependentCopy) {
    track_queue q;
    q.push({.webpage_url = "1"});
    q.push({.webpage_url = "2"});
    auto before_pop = q.snapshot();
    q.pop(std::stop_token());

    ASSERT_EQ(before_pop.size(), 2);
    EXPECT_EQ(before_pop[0].webpage_url, "1");
    EXPECT_EQ(before_pop[1].webpage_url, "2");

    auto after_pop = q.snapshot();

    EXPECT_EQ(after_pop.size(), 1);
}

