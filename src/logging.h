#pragma once

#include <vector>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#define tlog SPDLOG_TRACE
#define dlog SPDLOG_DEBUG
#define ilog SPDLOG_INFO
#define wlog SPDLOG_WARN
#define elog SPDLOG_ERROR
#define clog SPDLOG_CRITICAL

#define tlogc(logger_name, log_str, ...) SPDLOG_LOGGER_TRACE(spdlog::get(logger_name.toStdString()), log_str, __VA_ARGS__)
#define dlogc(logger_name, log_str, ...) SPDLOG_LOGGER_DEBUG(spdlog::get(logger_name.toStdString()), log_str, __VA_ARGS__)
#define ilogc(logger_name, log_str, ...) SPDLOG_LOGGER_INFO(spdlog::get(logger_name.toStdString()), log_str, __VA_ARGS__)
#define wlogc(logger_name, log_str, ...) SPDLOG_LOGGER_WARN(spdlog::get(logger_name.toStdString()), log_str, __VA_ARGS__)
#define elogc(logger_name, log_str, ...) SPDLOG_LOGGER_ERROR(spdlog::get(logger_name.toStdString()), log_str, __VA_ARGS__)
#define clogc(logger_name, log_str, ...) SPDLOG_LOGGER_CRITICAL(spdlog::get(logger_name.toStdString()), log_str, __VA_ARGS__)


struct log_level_info
{
    enum level
    {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        CRITICAL,
        OFF
    };

    level console_debug {DEBUG};
    level console_release {INFO};
    level daily_file_debug {DEBUG};
    level daily_file_release {INFO};
};

struct logger_store;

logger_store * log_init();
void log_terminate(logger_store *ls);
void log_add_logger(logger_store *ls, const log_level_info & log_level_inf = log_level_info(), const std::string & logger_name="", const std::string & pattern="");