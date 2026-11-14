#include "process.h"
#include "unique_fd.h"

#include <unistd.h>
#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <cstring>
#include <cerrno>
#include <spawn.h>
#include <system_error>
#include <sys/wait.h>
#include <poll.h>


extern char** environ;

namespace {
struct spawn_file_actions {
    posix_spawn_file_actions_t actions;
    spawn_file_actions() {
        int res = posix_spawn_file_actions_init(&actions);
        if (res != 0) {
            throw std::system_error(res, std::generic_category(), "Failed to init posix_spawn_file_actions");
        }
    }

    ~spawn_file_actions() { posix_spawn_file_actions_destroy(&actions); }

    spawn_file_actions(const spawn_file_actions&) = delete;

    spawn_file_actions& operator=(const spawn_file_actions&) = delete;
};
} // namespace

std::optional<process::result> process::run(const std::vector<std::string>& args, std::chrono::milliseconds timeout) {
    if (args.empty()) {
        spdlog::warn("No argument passed to process");
        return std::nullopt;
    }
    std::vector<char*> argv;
    argv.reserve(args.size());
    for (const auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.data()));
    }
    argv.push_back(nullptr);
    int fd_out[2];
    int fd_err[2];

    if (pipe2(fd_out, O_CLOEXEC) == -1) {
        spdlog::error("pipe2 error: {}", std::strerror(errno));
        return std::nullopt;
    }
    if (pipe2(fd_err, O_CLOEXEC) == -1) {
        spdlog::error("pipe2 error: {}", std::strerror(errno));
        return std::nullopt;
    }
    unique_fd pipe_write_out{fd_out[1]};
    unique_fd pipe_read_out{fd_out[0]};
    unique_fd pipe_write_err{fd_err[1]};
    unique_fd pipe_read_err{fd_err[0]};
    try {
        spawn_file_actions actions;
        int res = posix_spawn_file_actions_adddup2(&actions.actions, pipe_write_out.get(), STDOUT_FILENO);
        if (res != 0) {
            throw std::system_error(res, std::generic_category(), "Failed posix_spawn_file_actions_adddup2 for stdout");
        }
        res = posix_spawn_file_actions_adddup2(&actions.actions, pipe_write_err.get(), STDERR_FILENO);
        if (res != 0) {
            throw std::system_error(res, std::generic_category(), "Failed posix_spawn_file_actions_adddup2 for stderr");
        }
        pid_t pid;
        res = posix_spawnp(&pid, argv[0], &actions.actions, nullptr, argv.data(), environ);
        if (res != 0) {
            throw std::system_error(res, std::generic_category(), "Failed posix_spawn");
        }
        pipe_write_out.reset();
        pipe_write_err.reset();

        pollfd fds[2] = {{.fd = pipe_read_out.get(), .events = POLLIN, .revents = 0},
                {.fd = pipe_read_err.get(), .events = POLLIN, .revents = 0}};
        auto deadline = std::chrono::steady_clock::now() + timeout;
        char buffer[4096];
        result res_process;
        bool is_failed = false;
        int err_code = -1;
        int status;
        while (fds[0].fd >= 0 || fds[1].fd >= 0) {
            int left =
                    std::chrono::ceil<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count();
            if (left <= 0) {
                res_process.is_timeout = true;
                kill(pid, SIGKILL);
                break;
            }
            auto n = poll(fds, 2, left);
            if (n == -1) {
                if (errno == EINTR) {
                    continue;
                }
                is_failed = true;
                err_code = errno;
                break;
            }
            if (n == 0) {
                continue;
            }
            for (size_t i = 0; i < 2; ++i) {
                if (fds[i].revents != 0) {
                    auto r = read(fds[i].fd, buffer, sizeof(buffer));
                    if (r == 0) {
                        fds[i].fd = -1;
                    }
                    if (r > 0) {
                        if (i == 0) {
                            res_process.stdout_data.append(buffer, r);
                        } else {
                            res_process.stderr_data.append(buffer, r);
                        }
                    }
                    if (r == -1) {
                        if (errno == EINTR) {
                            continue;
                        }
                        is_failed = true;
                        err_code = errno;
                        fds[i].fd = -1;
                    }
                }
            }
            if (is_failed) {
                break;
            }
        }
        res = waitpid(pid, &status, 0);
        while (res == -1 && errno == EINTR) {
            res = waitpid(pid, &status, 0);
        }
        if (res == -1) {
            err_code = errno;
            throw std::system_error(err_code, std::generic_category(), "waitpid");
        }
        if (is_failed) {
            throw std::system_error(err_code, std::generic_category(), "Failed read");
        }
        if (WIFEXITED(status)) {
            res_process.code = WEXITSTATUS(status);
        }
        if (WIFSIGNALED(status)) {
            res_process.code = 128 + WTERMSIG(status);
        }

        return res_process;
    } catch (const std::system_error& e) {
        spdlog::error("Failed: {}", e.what());
        return std::nullopt;
    }
}
