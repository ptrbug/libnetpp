#pragma once
#include <boost/log/common.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/dll/runtime_symbol_info.hpp>

namespace netpp{
namespace src = boost::log::sources;

enum LogLevel
{
	trace,
	debug,
	info,
	warning,
	error,
	fatal
};

void initLogging(bool console_on = true
	, LogLevel console_level = LogLevel::trace
	, bool log_file_on = true
	, LogLevel log_file_level = LogLevel::trace
	, const std::string& log_file_dir = boost::dll::program_location().parent_path().string() + "/logs"
	, const std::string& log_file_name_prefix = boost::dll::program_location().stem().string());

src::severity_logger_mt<LogLevel>& lg();
#define LOG_LEVEL(lvl) BOOST_LOG_FUNCTION();BOOST_LOG_SEV(lg(), lvl)
#define LOG_TRACE LOG_LEVEL(LogLevel::trace)
#define LOG_DEBUG LOG_LEVEL(LogLevel::debug)
#define LOG_INFO  LOG_LEVEL(LogLevel::info)
#define LOG_WARN LOG_LEVEL(LogLevel::warning)
#define LOG_ERROR LOG_LEVEL(LogLevel::error)
#define LOG_FATAL LOG_LEVEL(LogLevel::fatal)
#define CHECK_NOTNULL(val) LOG_ERROR << "'" #val "' Must be non nullptr";
}