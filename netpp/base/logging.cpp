#include <netpp/base/logging.h>
#include <mutex>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/attributes/named_scope.hpp>
#pragma warning(disable: 4244)
#include <boost/log/support/date_time.hpp>
#pragma warning(default: 4244)

namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;

namespace netpp{
template< typename CharT, typename TraitsT >
inline std::basic_ostream< CharT, TraitsT >& operator<< (
	std::basic_ostream< CharT, TraitsT >& strm, LogLevel lvl)
{
	static const char* const str[] =
	{
		"trace",
		"debug",
		"info",
		"warning",
		"error",
		"fatal"
	};
	if (static_cast<std::size_t>(lvl) < (sizeof(str) / sizeof(*str)))
		strm << str[lvl];
	else
		strm << static_cast<int>(lvl);
	return strm;
}

BOOST_LOG_ATTRIBUTE_KEYWORD(_severity, "Severity", LogLevel);
BOOST_LOG_ATTRIBUTE_KEYWORD(_timestamp, "TimeStamp", boost::posix_time::ptime);
BOOST_LOG_ATTRIBUTE_KEYWORD(_scope, "Scope", attrs::named_scope::value_type);

bool initialized_ = false;
std::mutex mutex_;
src::severity_logger_mt<LogLevel> lg_;

void initLogging_unlock(bool console_on
	, LogLevel console_level
	, bool log_file_on
	, LogLevel log_file_level
	, const std::string& log_file_dir
	, const std::string& log_file_name_prefix) {

	//set console log
	if (console_on) {
		auto console_sink = logging::add_console_log(std::clog, keywords::format = expr::stream
			<< expr::format_date_time(_timestamp, "[%Y-%m-%d,%H:%M:%S.%f]")
			<< " [" << _severity
			<< "]: " << expr::message);		
	}
	
	//set file log
	if (log_file_on) {
		std::stringstream log_file_path;
		log_file_path << log_file_dir << "/" << log_file_name_prefix << "_%Y%m%d.log";
		auto file_sink = logging::add_file_log
			(
			keywords::file_name = log_file_path.str(),
			keywords::rotation_size = 10 * 1024 * 1024,
			keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
			keywords::open_mode = std::ios::app,
			keywords::auto_flush = false
			);
		file_sink->locked_backend()->set_file_collector(sinks::file::make_collector(
			keywords::target = log_file_dir,
			keywords::max_size = 50 * 1024 * 1024,
			keywords::min_free_space = 100 * 1024 * 1024
			));

		file_sink->set_filter(_severity >= log_file_level);
		file_sink->locked_backend()->scan_for_files();
		file_sink->set_formatter
			(
			expr::format("[%1%][%2%][%3%][%4%]: %5%")
			% expr::attr< attrs::current_thread_id::value_type >("ThreadID")
			% expr::format_date_time(_timestamp, "%Y-%m-%d,%H:%M:%S.%f")
			% expr::format_named_scope(_scope, keywords::format = "%n (%f:%l)", keywords::depth = 1)
			% _severity
			% expr::smessage
			);		
		logging::core::get()->add_sink(file_sink);
	}

	logging::add_common_attributes();
	logging::core::get()->add_thread_attribute("Scope", attrs::named_scope());
}

void initLogging(bool console_on
	, LogLevel console_level
	, bool log_file_on
	, LogLevel log_file_level
	, const std::string& log_file_dir
	, const std::string& log_file_name_prefix) {

	if (initialized_ == false) {
		std::unique_lock<std::mutex> lock(mutex_);
		assert(initialized_ == false);
		if (initialized_ == false) {
			initLogging_unlock(console_on, console_level, log_file_on, log_file_level, log_file_dir, log_file_name_prefix);
			initialized_ = true;
		}
	}
	else {
		assert(false);
	}	
}

src::severity_logger_mt<LogLevel>& lg() {
	if (initialized_ == false){
		std::unique_lock<std::mutex> lock(mutex_);
		if (initialized_ == false) {
			initLogging_unlock(true, LogLevel::trace, true, LogLevel::trace
				, boost::dll::program_location().parent_path().string() + "/logs"
				, boost::dll::program_location().stem().string());
			initialized_ = true;
		}
	}
	return lg_;
}
}