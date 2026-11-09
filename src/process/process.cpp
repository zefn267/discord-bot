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

std::optional<process::result> process::run(const std::vector<std::string>& args) {
    if (args.empty()) {
        spdlog::warn("No argument passed to process");
        return std::nullopt;
    }
    std::vector<char*> argv;
    for (const auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.data()));
    }
    argv.push_back(nullptr);
    int fd[2];
    if (pipe2(fd, O_CLOEXEC) == -1) {
        spdlog::error("pipe2 error: {}", std::strerror(errno));
        return std::nullopt;
    }
    unique_fd pipe_write{fd[1]};
    unique_fd pipe_read{fd[0]};
    try {
        spawn_file_actions actions;
        int res = posix_spawn_file_actions_adddup2(&actions.actions, pipe_write.get(), STDOUT_FILENO);
        if (res != 0) {
            throw std::system_error(res, std::generic_category(), "Failed posix_spawn_file_actions_adddup2");
        }
        res = posix_spawn_file_actions_addopen(&actions.actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0);
        if (res != 0) {
            throw std::system_error(res, std::generic_category(), "Failed posix_spawn_file_actions_addopen");
        }
        pid_t pid;
        res = posix_spawnp(&pid, argv[0], &actions.actions, nullptr, argv.data(), environ);
        if (res != 0) {
            throw std::system_error(res, std::generic_category(), "Failed posix_spawn");
        }
        pipe_write.reset();
        char buffer[4096];
        result res_process;
        bool is_failed = false;
        int err_code = -1;
        int status;
        while (true) {
            auto r = read(pipe_read.get(), buffer, sizeof(buffer));
            if (r == 0) {
                break;
            }
            if (r > 0) {
                res_process.stdout_data.append(buffer, r);
            }
            if (r == -1) {
                if (errno == EINTR) {
                    continue;
                }
                is_failed = true;
                err_code = errno;
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
