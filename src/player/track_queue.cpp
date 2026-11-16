#include "track_queue.h"


void track_queue::push(track t) {
    {
        std::lock_guard lock(mx_);
        tracks_.push_back(std::move(t));
    }
    cv_.notify_one();
}

std::optional<track> track_queue::pop(std::stop_token token) {
    std::unique_lock lock(mx_);
    bool ok = cv_.wait(lock, token, [&] {
        return !tracks_.empty();
    });
    if (!ok) {
        return std::nullopt;
    }
    track first = std::move(tracks_.front());
    tracks_.pop_front();

    return first;
}

void track_queue::clear() {
    std::lock_guard lock(mx_);
    tracks_.clear();
}

std::vector<track> track_queue::snapshot() const {
    std::lock_guard lock(mx_);
    return {tracks_.begin(), tracks_.end()};
}
