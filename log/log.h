#ifndef __LOG_H__
#define __LOG_H__
#include <iostream>
#include <string>
#include <stdint.h>
#include <memory>
#include <list>
#include <sstream>
#include <fstream>
#include <vector>
#include <stdarg.h>
#include <map>
#include <pthread.h>
#include <functional>
#include "mutex.h"

namespace ekko {

class Logger;
class LoggerManager;

class LogLevel {
public:
    enum Level {
        UNKNOW = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5
    };

    static const char* ToString(LogLevel::Level level);
    
    static LogLevel::Level FromString(const std::string& str);
};

//no lock
class LogEvent {
public:
    typedef std::shared_ptr<LogEvent> ptr;
    
    LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level
            ,const char* file, int32_t line, uint32_t elapse
            ,uint32_t thread_id, uint32_t fiber_id, uint64_t time
            ,const std::string& thread_name);

    const char* GetFile() const { return file_;}

    int32_t GetLine() const { return line_;}

    uint32_t GetElapse() const { return elapse_;}

    uint32_t GetThreadId() const { return thread_id_;}

    uint32_t GetFiberId() const { return fiber_id_;}

    uint64_t GetTime() const { return time_;}

    const std::string& GetThreadName() const { return thread_name_;}

    std::string GetContent() const { return str_stream_.str();}

    std::shared_ptr<Logger> GetLogger() const { return logger_;}

    LogLevel::Level GetLevel() const { return level_;}

    std::stringstream& GetStrStream() { return str_stream_;}

    void Format(const char* fmt, ...);

    void Format(const char* fmt, va_list al);
private:
    const char* file_ = nullptr;
    
    int32_t line_ = 0;
    
    uint32_t elapse_ = 0;
    
    uint32_t thread_id_ = 0;
    
    uint32_t fiber_id_ = 0;
    
    uint64_t time_ = 0;
    
    std::string thread_name_;
    
    std::stringstream str_stream_;
    
    std::shared_ptr<Logger> logger_;
    
    LogLevel::Level level_;
};

class LogEventWrap {
public:

    LogEventWrap(LogEvent::ptr e);

    ~LogEventWrap();

    LogEvent::ptr GetEvent() const { return event_;}

    std::stringstream& GetStrStream() { return event_->GetStrStream();}
private:

    LogEvent::ptr event_;
};

//no lock
class LogFormatter {
public:

    class FormatItem {
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        
        virtual ~FormatItem() {}
        
        virtual void Format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
    };
    typedef std::shared_ptr<LogFormatter> ptr;

    LogFormatter(const std::string& pattern);

    std::string Format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);

    std::ostream& Format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);

    void Init();

    bool IsError() const { return error_;}

    const std::string GetPattern() const { return pattern_;}
private:

    std::string pattern_;

    //parsed format
    std::vector<FormatItem::ptr> items_;
    
    bool error_ = false;
};

class LogAppender {
friend class Logger;
public:
    typedef std::shared_ptr<LogAppender> ptr;
    typedef Spinlock MutexType;

    virtual ~LogAppender() {}

    virtual void Log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;

    void SetFormatter(LogFormatter::ptr val);

    LogFormatter::ptr GetFormatter();

    LogLevel::Level GetLevel() const { return level_;}

    void SetLevel(LogLevel::Level val) { level_ = val;}

    bool HasFormatter() const { return formatter_ == 0;}
protected:
    
    LogLevel::Level level_ = LogLevel::DEBUG;
    
    MutexType mutex_;
    
    LogFormatter::ptr formatter_ = 0;
};

class Logger : public std::enable_shared_from_this<Logger> {
friend class LoggerManager;
public:
    typedef std::shared_ptr<Logger> ptr;
    typedef Spinlock MutexType;

    Logger(const std::string& name = "root");

    void Log(LogLevel::Level level, LogEvent::ptr event);

    void Debug(LogEvent::ptr event);

    void Info(LogEvent::ptr event);

    void Warn(LogEvent::ptr event);

    void Error(LogEvent::ptr event);

    void Fatal(LogEvent::ptr event);

    void AddAppender(LogAppender::ptr appender);

    void DelAppender(LogAppender::ptr appender);

    void ClearAppenders();

    LogLevel::Level GetLevel() const { return level_;}

    void SetLevel(LogLevel::Level val) { level_ = val;}

    const std::string& GetName() const { return name_;}

    void SetFormatter(LogFormatter::ptr val);

    void SetFormatter(const std::string& val);

    LogFormatter::ptr GetFormatter();

private:
    std::string name_;
    
    LogLevel::Level level_;
    
    MutexType mutex_;
    
    std::list<LogAppender::ptr> appenders_;
    
    LogFormatter::ptr formatter_;
    
    Logger::ptr root_;
};

class StdoutLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    void Log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
};

class FileLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const std::string& filename);
    void Log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;

    /**
     * @return success with true
     */
    bool reopen();
private:
    std::string file_name_;
    std::ofstream file_stream_;
    uint64_t last_time_ = 0;
};

class LoggerManager {
public:
    typedef Spinlock MutexType;

    static std::shared_ptr<LoggerManager> GetInstance() {
        static std::shared_ptr<LoggerManager> v(new LoggerManager);
        return v;
    }

    Logger::ptr GetLogger(const std::string& name);

    Logger::ptr GetRoot() const { return root_;}

private:
    LoggerManager();
    
    MutexType mutex_;
    std::map<std::string, Logger::ptr> logger_s;
    Logger::ptr root_;
};


}

#endif
