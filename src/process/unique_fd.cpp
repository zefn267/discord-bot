#include "unique_fd.h"

#include <unistd.h>
#include <utility>


unique_fd::unique_fd(int fd) : fd_(fd) {}

unique_fd::~unique_fd() {
    reset();
}

unique_fd::unique_fd(unique_fd&& other) noexcept : fd_(std::exchange(other.fd_, -1)) {}

unique_fd& unique_fd::operator=(unique_fd&& other) noexcept {
    if (this != &other) {
        reset(std::exchange(other.fd_, -1));
    }
    return *this;
}

unique_fd::operator bool() const noexcept {
    return fd_ >= 0;
}

int unique_fd::get() const {
    return fd_;
}

void unique_fd::reset(int fd) {
    if (fd_ == fd) {
        return;
    }
    if (fd_ >= 0) {
        close(fd_);
    }
    fd_ = fd;
}