#include "log_stream.h"
#include <unistd.h>

int main() {
    Logger::getInstance().setLogFile("test.log");
    Logger::getInstance().setConsoleOutput(true);
    Logger::getInstance().setAsyncOutput(true);
    Logger::getInstance().setOutputLevel(LogLevel::DEBUG);
    // Logger::getInstance().setLogFileMaxSize(1024 * 10);
    // Logger::getInstance().setLogFileMaxCount(3);
    // Logger::getInstance().asyncOutputLineMax(50);
    // Logger::getInstance().asyncOutputTimeOutSec(2);


    // c++ 风格
    LOG(INFO) << "Configuration loaded successfully.";
    LOG(DEBUG) << "Debugging application start.";
    LOG(WARNING) << "Low disk space warning.";
    LOG(ERR) << "Error connecting to database.";
    LOG(FATAL) << "Fatal error: unable to continue.";
    LOG(INFO) << "Number testing: " << 42 << ", Pi: " << 3.14159 << ", Hex: " << std::hex << 255;

    // C语言风格
    LOG_I("Server started on port %d", 8080);
    LOG_W("Memory usage is high: %d%%", 85);
    LOG_D("User %s logged in", "admin");
    
    uint8_t buf[256];
    for (int i = 0; i < sizeof(buf); ++i) {
        buf[i] = i;
    }
    LOG_HEXDUMP(DEBUG, buf, sizeof(buf), "Buffer", Logger::HexDumpFormat::HEX_DUMP_1B);
    LOG_HEXDUMP(INFO, buf, sizeof(buf), "Buffer", Logger::HexDumpFormat::HEX_DUMP_2B);
    LOG_HEXDUMP(WARNING, buf, sizeof(buf), "Buffer", Logger::HexDumpFormat::HEX_DUMP_4B);
    LOG_HEXDUMP(ERR, buf, sizeof(buf), "Buffer", Logger::HexDumpFormat::HEX_DUMP_8B);
}
