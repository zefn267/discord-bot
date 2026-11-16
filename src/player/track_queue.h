#pragma once

#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <vector>
#include <optional>
#include <stop_token>
#include <cstdint>


struct track {
    std::string webpage_url;
    uint64_t user_id;
    uint64_t text_id;
};

class track_queue {
public:
    void push(track t);

    std::optional<track> pop(std::stop_token token);

    void clear();

    std::vector<track> snapshot() const;

private:
    std::deque<track> tracks_;
    mutable std::mutex mx_;
    std::condition_variable_any cv_;
};
