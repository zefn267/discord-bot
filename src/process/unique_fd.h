#pragma once


class unique_fd {
public:
    unique_fd() = default;

    explicit unique_fd(int fd);

    ~unique_fd();

    unique_fd(const unique_fd&) = delete;

    unique_fd& operator=(const unique_fd&) = delete;

    unique_fd(unique_fd&& other) noexcept;

    unique_fd& operator=(unique_fd&& other) noexcept;

    explicit operator bool() const noexcept;

    int get() const;

    void reset(int fd = -1);
private:
    int fd_ = -1;
};