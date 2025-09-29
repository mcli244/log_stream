#include "log_stream.h"
#include <chrono>
#include <ctime>
#include <string>
#include <sstream>
#include <iomanip>
#include <thread>
#include <csignal>
#include <iostream>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#include <sys/timeb.h>

int gettimeofday(struct timeval* tp, void* tzp) {
    if (!tp) return -1;

    struct _timeb timebuffer;
    _ftime_s(&timebuffer); 

    tp->tv_sec = (long)timebuffer.time;
    tp->tv_usec = timebuffer.millitm * 1000;
    return 0;
}
#else
#include <sys/time.h>
#endif

static bool fileExists(const std::string& filename) {
    std::ifstream f(filename);
    return f.good();
}

static size_t fileLine(const std::string& filename){
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "无法打开文件\n";
        return -1;
    }

    std::string line;
    size_t lineCount = 0;
    while (std::getline(file, line)) {
        ++lineCount;
    }

    return lineCount;
}

static size_t fileSize(const std::string& filename){
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "无法打开文件\n";
        return -1;
    }

    size_t size = 0;
    file.seekg(0, std::ios::end);
    size = file.tellg();
    file.close();
    
    return size;
}

// Logger -------------------------
Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::~Logger() {
    if (asyncThread_.joinable()) {
        asyncOutput_ = false;
        log_cv_.notify_all(); // 唤醒异步线程以便它可以退出
        asyncThread_.join();
    }
    
    if (logFile_.is_open()) {
        logFile_.close();
    }
}

void Logger::setLogFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (logFile_.is_open()) logFile_.close();
    currentFileSize_ = fileSize(filename);
    logFile_.open(filename, std::ios::app);
    fileName_ = filename;
}

void Logger::setConsoleOutput(bool enable) {
    consoleOutput_ = enable;
}

void Logger::logFlush() {
    if (!logBuffer_.empty()) {
        if(!logFile_.is_open()) {
            fileName_ = "async.log";
            currentFileSize_ = fileSize(fileName_);
            logFile_.open(fileName_.c_str(), std::ios::app);
        }

        for(const auto& message : logBuffer_) {
            std::string legacy = "";
            if(currentFileSize_ < logFileMaxSize_ && currentFileSize_ + message.size() >= logFileMaxSize_) {
                std::string msg = message.substr(0, logFileMaxSize_ - currentFileSize_);
                legacy = message.substr(logFileMaxSize_ - currentFileSize_);
                logFile_ << msg;
                currentFileSize_ += msg.size();
            }else{
                logFile_ << message;
                currentFileSize_ += message.size();
            }
            
            if(currentFileSize_ >= logFileMaxSize_) {
                logFile_.close();
                for(int i = logFileMaxCount_ - 1; i >= 0; --i) {
                    std::string oldName = fileName_ + std::to_string(i);
                    std::string newName = fileName_ + std::to_string(i + 1);
                    if(fileExists(oldName)) {
                        if(i + 1 >= logFileMaxCount_) {
                            std::remove(oldName.c_str());
                        } else {
                            std::rename(oldName.c_str(), newName.c_str());
                        }
                    }
                }
                std::string baseName = fileName_;
                std::string firstRotated = fileName_ + "0";
                std::rename(baseName.c_str(), firstRotated.c_str());
                logFile_.open(baseName, std::ios::app);

                if(!legacy.empty()) {
                    logFile_ << legacy;
                    currentFileSize_ = legacy.size();
                } else {
                    currentFileSize_ = 0;
                }
            }
        }
        logBuffer_.clear();
        logFile_.flush();
    }
}

void Logger::setLogFileMaxSize(size_t bytes) {
    logFileMaxSize_ = bytes;
}

void Logger::setLogFileMaxCount(size_t count) {
    logFileMaxCount_ = count;
}

void Logger::asyncOutputLineMax(size_t count) {
    asyncOutputLineMax_ = count;
}

void Logger::asyncOutputTimeOutSec(size_t sec) {
    asyncOutputTimeOutSec_ = sec;
}


void Logger::crashHandler(int signum) {
    Logger::getInstance().logFlush();
    std::exit(signum);
}

void Logger::setupCrashHandlers() {
    signal(SIGSEGV, Logger::crashHandler);  // 段错误
    signal(SIGABRT, Logger::crashHandler);  // abort()
    signal(SIGFPE,  Logger::crashHandler);  // 浮点错误
    signal(SIGILL,  Logger::crashHandler);  // 非法指令
#ifndef _WIN32
    signal(SIGBUS, Logger::crashHandler);  // 总线错误
#endif 
    signal(SIGINT,  Logger::crashHandler);  // Ctrl+C
    signal(SIGTERM, Logger::crashHandler);  // kill 默认
}

void Logger::asyncThreadFunction() {
    asyncThreadIsRunning_ = true;
    while (asyncOutput_) {
        std::unique_lock<std::mutex> lock(mutex_);
        bool result = log_cv_.wait_for(lock, std::chrono::seconds(asyncOutputTimeOutSec_), [this] {
            return !logBuffer_.empty();
        });
        // 不论结果是超时还是有数据，都会尝试写入日志
        logFlush();
    }
}

bool Logger::setAsyncOutput(bool enable) {
    if(enable) {
        if(asyncThreadIsRunning_) {
            return true;
        } else {
            asyncOutput_ = true;
            asyncThread_ = std::thread(&Logger::asyncThreadFunction, this);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } else {
        // Disable async logging
        asyncOutput_ = false;
        return true;
    }

    return true;
}

void Logger::write(LogLevel level, const std::string& message) {
    if(level > output_level_) {
        return;
    }

    std::string line = "[" + getTimestamp() + "][" + levelToString(level) + "] " + message;
    std::string color_line;
    switch (level) {
        case LogLevel::WARNING: 
            color_line = "\033[33m" + line + "\033[0m"; 
            break;
        case LogLevel::ERR:
            color_line = "\033[31m" + line +  "\033[0m"; 
            break;
        case LogLevel::DEBUG: 
            color_line = "\033[32m" + line +  "\033[0m"; 
            break;
        case LogLevel::FATAL: 
            color_line = "\033[41;97m" + line + "\033[0m"; 
            break;
        case LogLevel::INFO: 
        default: 
            color_line = line; 
            break;
    }
    color_line += NEWLINE_SIGN;
    line += NEWLINE_SIGN;

    if (consoleOutput_) std::cout << color_line;
    if (asyncOutput_) {
        std::lock_guard<std::mutex> lock(mutex_);
        logBuffer_.push_back(line);
        if(logBuffer_.size() >= asyncOutputLineMax_) {
            log_cv_.notify_one();
        }
    } else {
        logFile_ << line;
    }
    
}

std::string Logger::getTimestamp() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);

    struct tm tm;
#ifdef _WIN32
    time_t now = time(NULL);
    localtime_s(&tm, &now);
#else
    localtime_r(&tv.tv_sec, &tm);
#endif
    char buf[64];
    snprintf(buf, sizeof(buf),
             "%04d-%02d-%02d %02d:%02d:%02d.%06ld",
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec, (long)tv.tv_usec);
    return std::string(buf);
}


std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARN";
        case LogLevel::ERR: return "ERROR";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

void Logger::setOutputLevel(LogLevel level) {
        output_level_ = level;
}

void Logger::setOutputLevel(std::string levelStr) {
    if (levelStr == "INFO") {
        output_level_ = LogLevel::INFO;
    } else if (levelStr == "DEBUG") {
        output_level_ = LogLevel::DEBUG;
    } else if (levelStr == "ERROR") {
        output_level_ = LogLevel::ERR;
    } else if (levelStr == "WARNING") {
        output_level_ = LogLevel::WARNING;
    } else {
        output_level_ = LogLevel::INFO; // 默认级别
    }
}

void Logger::hexDump(LogLevel level, const void* data, size_t length, const std::string& prefix, HexDumpFormat fmt) {
    if (level > output_level_) return ;

    const uint8_t* buf = static_cast<const uint8_t*>(data);
    std::ostringstream oss;
    std::string out;
    int cnt = 8;

    switch (fmt)
    {
    case HexDumpFormat::HEX_DUMP_16B: cnt = 16; break;
    case HexDumpFormat::HEX_DUMP_8B: cnt = 8; break;
    case HexDumpFormat::HEX_DUMP_4B: cnt = 4; break;
    case HexDumpFormat::HEX_DUMP_2B: cnt = 2; break;
    case HexDumpFormat::HEX_DUMP_1B: cnt = 1; break;
    
    default:
        break;
    }
    

    for (size_t i = 0; i < length; i += 16) {
        oss.str("");
        oss.clear();

        // 打印偏移
        oss << prefix << "(" << std::setw(6) << std::setfill('0') << std::hex << i << ") ";

        // 打印十六进制
        for (size_t j = 0; j < 16; j++) {
            if (i + j < length) {
                if((j % cnt) == (cnt - 1) && j != 0 || cnt == 1)
                    oss << std::setw(2) << std::setfill('0') << std::hex << (int)buf[i + j] << " ";
                else
                    oss << std::setw(2) << std::setfill('0') << std::hex << (int)buf[i + j];
            } else {
                oss << "   ";
            }
        }

        oss << " ";

        // 打印 ASCII
        for (size_t j = 0; j < 16; j++) {
            if (i + j < length) {
                char c = buf[i + j];
                if (std::isprint(static_cast<unsigned char>(c)))
                    oss << c;
                else
                    oss << ".";
            }
        }

        // 输出这一行
        write(level, oss.str());
    }
}

// LoggerStream ---------------------
LoggerStream::LoggerStream(LogLevel level) : level_(level) {}

LoggerStream::~LoggerStream() {
    Logger::getInstance().write(level_, stream_.str());
}

