#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include <memory>
#include <string_view>

class Logging {
 public:
 enum class LogLevel {
    Error,
    Info,
    Debug
  };

  class LoggerHelper {
    public:
      LoggerHelper(Logging& logger, Logging::LogLevel level)
        : logger_(logger)
        , level_(level)
      {}

      ~LoggerHelper() {
        switch (level_) {
          case LogLevel::Error:
            logger_.Error(msg_);
            break;
          case LogLevel::Info:
            logger_.Info(msg_);
            break;
          case LogLevel::Debug:
            logger_.Debug(msg_);
            break;
        }
      }

      void Log(std::string_view msg) const {
        msg_ += msg;
      }

    private:
      Logging& logger_;
      Logging::LogLevel level_;
      mutable std::string msg_;
  };

  static Logging& Logger() {
    static Logging logger = Logging();
    return logger;
  }

  LoggerHelper Error() { return LoggerHelper(*this, LogLevel::Error); }
  LoggerHelper Info() { return LoggerHelper(*this, LogLevel::Info); }
  LoggerHelper Debug() { return LoggerHelper(*this, LogLevel::Debug); }

  void Error(std::string_view msg) { logger_->error(msg); }
  void Info(std::string_view msg) { logger_->info(msg); }
  void Debug(std::string_view msg) { logger_->debug(msg); }

  void Init(const std::string& log_path) {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::debug);
    console_sink->set_pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] %v");

    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_path, true);
    file_sink->set_level(spdlog::level::debug);
    file_sink->set_pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] %v");

    logger_ = std::make_shared<spdlog::logger>("multi_sink", std::initializer_list<spdlog::sink_ptr>{file_sink, console_sink});
    logger_->set_level(spdlog::level::debug);
    logger_->flush_on(spdlog::level::debug);
  }

 private:
  Logging() = default;
  std::shared_ptr<spdlog::logger> logger_;
};

template <typename T>
const Logging::LoggerHelper& operator<<(const Logging::LoggerHelper& logger, const T& value) {
  std::ostringstream os;
  os << value;

  logger.Log(os.str());

  return logger;
}
