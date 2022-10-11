#include <filesystem>
#include <list>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
// #include <Urho3D/IO/FileSystem.h>

#include "logging.h"

struct logger_store
{
    std::shared_ptr<spdlog::logger> default_logger;
    std::vector<std::shared_ptr<spdlog::logger>> custom_loggers;
};

logger_store * log_init()
{
    return new logger_store{};
}

void log_add_logger(logger_store *ls, const log_level_info &log_level_inf, const std::string &logger_name, const std::string &pattern)
{
    std::list<spdlog::sink_ptr> multi_sink_list;

#ifdef DEBUG_VERSION
    int console_level = log_level_inf.console_debug;
    int daily_file_level = log_level_inf.daily_file_debug;
#else
    int console_level = log_level_inf.console_release;
    int daily_file_level = log_level_inf.daily_file_release;
#endif

    if (console_level != log_level_info::OFF)
    {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(static_cast<spdlog::level::level_enum>(console_level));
        multi_sink_list.push_back(console_sink);
    }
    
    if (daily_file_level != log_level_info::OFF)
    {
        std::string fname(logger_name);
        if (fname.empty())
            fname = "jackal_control";
        fname += ".log";
        auto file_sink_local = std::make_shared<spdlog::sinks::basic_file_sink_mt>(fname, false);
        file_sink_local->set_level(static_cast<spdlog::level::level_enum>(daily_file_level));
        multi_sink_list.push_back(file_sink_local);
    }

    auto logger = std::make_shared<spdlog::logger>(logger_name, multi_sink_list.begin(), multi_sink_list.end());
    logger->set_level(spdlog::level::trace);
    logger->flush_on(spdlog::level::trace);
    spdlog::flush_every(std::chrono::seconds(3));


    if (!pattern.empty())
        logger->set_pattern(pattern);
    else
        logger->set_pattern("[%I:%M:%S.%e %p TID:%t] [%s:%# (%!)] %^[%l]%$ %v");

    if (logger_name.empty())
    {
        ls->default_logger = logger;
        spdlog::set_default_logger(ls->default_logger);
    }
    else
    {
        ls->custom_loggers.push_back(logger);
        spdlog::register_logger(ls->custom_loggers.back());
    }
}

void log_terminate(logger_store *ls)
{
    ls->default_logger->flush();
    for (auto sink : ls->custom_loggers)
        sink->flush();
    delete ls;
}