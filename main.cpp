#include "log_stream.h"
#include <unistd.h>

int main() {
    Logger::getInstance().setLogFile("test.log");
    Logger::getInstance().setConsoleOutput(true);
    Logger::getInstance().setAsyncOutput(true);
    Logger::getInstance().setOutputLevel(LogLevel::DEBUG);

    LOG(INFO) << "Configuration loaded successfully.";
    LOG(DEBUG) << "Debugging application start.";
    LOG(WARNING) << "Low disk space warning.";
    LOG(ERR) << "Error connecting to database.";
    LOG(FATAL) << "Fatal error: unable to continue.";

    uint8_t buf[128];
    for (int i = 0; i < sizeof(buf); ++i) {
        buf[i] = i;
    }
    LOG_HEXDUMP(DEBUG, buf, sizeof(buf), "Buffer", Logger::HexDumpFormat::HEX_DUMP_1B);
}
