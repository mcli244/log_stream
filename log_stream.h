#pragma once
#include <string>
#include <sstream>
#include <mutex>
#include <fstream>
#include <iostream>
#include <memory>
#include <ctime>
#include <vector>
#include <thread>
#include <cstdarg>
#include <condition_variable>

enum class LogLevel {
    FATAL,
    ERR,
    WARNING,
    INFO,
    DEBUG
};

class LoggerStream {
public:
    LoggerStream(LogLevel level);
    ~LoggerStream();

    template<typename T>
    LoggerStream& operator<<(const T& val) {
        stream_ << val;
        return *this;
    }

    void logf(const char* fmt, ...) {
        char buf[512];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        stream_ << buf;
    }
    
private:
    LogLevel level_;
    std::ostringstream stream_;
};

class Logger {
public:
enum HexDumpFormat {
    HEX_DUMP_16B    = 16,
    HEX_DUMP_8B     = 8,
    HEX_DUMP_4B     = 4,
    HEX_DUMP_2B     = 2,
    HEX_DUMP_1B     = 1
};

public:
    static Logger& getInstance();

    void setLogFile(const std::string& filename);
    void setConsoleOutput(bool enable);
    bool setAsyncOutput(bool enable);
    void write(LogLevel level, const std::string& message);
    void asyncThreadFunction();
    void logFlush();
    void setupCrashHandlers();
    void setOutputLevel(LogLevel level);
    void setOutputLevel(std::string levelStr);
    void hexDump(LogLevel level, const void* data, size_t length, const std::string& prefix = "", HexDumpFormat fmt = HEX_DUMP_1B);

private:
    Logger() = default;
    ~Logger();

    std::string getTimestamp();
    std::string levelToString(LogLevel level);
    
    static void crashHandler(int signum);

    std::mutex mutex_;
    std::ofstream logFile_;
    bool consoleOutput_ = true;
    bool asyncOutput_ = true;

    std::vector<std::string> logBuffer_;
    std::thread asyncThread_;
    bool asyncThreadIsRunning_ = false;
    size_t asyncOutputBufferMax_ = 100; // 异步输出缓冲区最大行数
    std::condition_variable log_cv_;
    std::mutex log_mutex_;

    LogLevel output_level_ = LogLevel::DEBUG; // 默认日志级别为INFO
    // 日志轮转参数
    // 可以按照文件大小或者行数进行轮转, 默认按文件大小轮转
    // 当两个都启用时, 以先达到的为准
    size_t logFileMaxSize_ = 10 * 1024 * 1024; 
    size_t logFileMaxLine_ = 0;
    size_t logFileMaxCount_ = 5;

    size_t currentFileSize_ = 0;
    size_t currentFileLine_ = 0;
};


#define NEWLINE_SIGN    "\n"

// C++风格 LOG(INFO) << "message";
#define LOG(level) LoggerStream(LogLevel::level)    

// C语言风格 LOG_I("message %s", "test");
#define LOG_I(format, ...)  LoggerStream(LogLevel::INFO).logf(format, ##__VA_ARGS__)
#define LOG_W(format, ...)  LoggerStream(LogLevel::WARNING).logf(format, ##__VA_ARGS__)
#define LOG_D(format, ...)  LoggerStream(LogLevel::DEBUG).logf(format, ##__VA_ARGS__)
#define LOG_E(format, ...)  LoggerStream(LogLevel::ERR).logf(format, ##__VA_ARGS__)
// HexDump 宏
#define LOG_HEXDUMP(level, data, length, ...)  Logger::getInstance().hexDump(LogLevel::level, data, length, ##__VA_ARGS__)
