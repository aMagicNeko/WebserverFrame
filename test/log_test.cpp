#include "log.h"
#include <time.h>
using namespace ekko;
int main() {
    auto logmanager = LoggerManager::GetInstance();
    auto logger = logmanager->GetLogger("root");
    auto logevent = std::shared_ptr<LogEvent> (new LogEvent(logger, LogLevel::DEBUG, "log_test.cpp", 0, 0, 0, 0, time(NULL), "thread1"));
    auto appender = std::shared_ptr<LogAppender>(new FileLogAppender("test.txt"));
    auto formatter = std::shared_ptr<LogFormatter>(new LogFormatter("Test Message%T%p%T%d"));
    appender->SetFormatter(formatter);
    logger->AddAppender(appender);
    logger->Log(LogLevel::DEBUG, logevent);
}