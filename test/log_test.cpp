#include "log.h"
#include <time.h>
using namespace ekko;
int main() {
    auto logmanager = LoggerManager::GetInstance();
    auto logger = logmanager->GetLogger("root");
    logger->Info(std::shared_ptr<LogEvent> (new LogEvent(logger, LogLevel::INFO, "log_test.cpp", 7, 0, 1, 0, time(NULL), "thread main")));
    auto appender = new FileLogAppender("test.txt");
    auto formatter = new LogFormatter("Test Message%T%p%T%d")
    appender->SetFormatter()
    logger->AddAppender(appender);

}