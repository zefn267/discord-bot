#include "logging.h"
#include <spdlog/spdlog.h>


void set_settings_logs() {
    spdlog::set_pattern("[%Y-%m-%d %T] [%^%l%$] %v");
}

void change_logs_from_dpp(const dpp::log_t& event) {
    spdlog::level::level_enum spdlog_level = spdlog::level::info;
    switch (event.severity) {
        case dpp::ll_trace:
            spdlog_level = spdlog::level::trace;
            break;
        case dpp::ll_debug:
            spdlog_level = spdlog::level::debug;
            break;
        case dpp::ll_info:
            spdlog_level = spdlog::level::info;
            break;
        case dpp::ll_warning:
            spdlog_level = spdlog::level::warn;
            break;
        case dpp::ll_error:
            spdlog_level = spdlog::level::err;
            break;
        case dpp::ll_critical:
            spdlog_level = spdlog::level::critical;
            break;
    }
    spdlog::log(spdlog_level, "{}", event.message);
}
