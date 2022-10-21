#include "log.h"
#include "util.h"
namespace ekko{
const char*
LogLevel::ToString(LogLevel::Level level)
{
    switch(level) {
        case DEBUG:
            return "DEBUG";
        case INFO:
            return "INFO";
        case WARN:
            return "WARN";
        case ERROR:
            return "ERROR";
        case FATAL:
            return "FATAL";
        default:
            return "UNKNOW";
    }
}

LogLevel::Level LogLevel::FromString(const std::string& str) {
#define XX(level, v) \
    if(str == #v) { \
        return LogLevel::level; \
    }
    XX(DEBUG, debug);
    XX(INFO, info);
    XX(WARN, warn);
    XX(ERROR, error);
    XX(FATAL, fatal);

    XX(DEBUG, DEBUG);
    XX(INFO, INFO);
    XX(WARN, WARN);
    XX(ERROR, ERROR);
    XX(FATAL, FATAL);
    return LogLevel::UNKNOW;
#undef XX
}

LogEventWrap::LogEventWrap(LogEvent::ptr e)
    :event_(e) {
}

LogEventWrap::~LogEventWrap() {
    event_->GetLogger()->Log(event_->GetLevel(), event_);
}

void LogEvent::Format(const char* fmt, ...) {
    va_list al;
    va_start(al, fmt);
    Format(fmt, al);
    va_end(al);
}

void LogEvent::Format(const char* fmt, va_list al) {
    char* buf = nullptr;
    int len = vasprintf(&buf, fmt, al);
    if(len != -1) {
        str_stream_ << std::string(buf, len);
        free(buf);
    }
}


void LogAppender::SetFormatter(LogFormatter::ptr val) {
    Spinlock::Lock ll(mutex_);
    formatter_ = val;
}

LogFormatter::ptr LogAppender::GetFormatter() {
    Spinlock::Lock ll(mutex_);
    return formatter_;
}

class MessageFormatItem : public LogFormatter::FormatItem {
public:
    MessageFormatItem(const std::string& str = "") {}
    void Format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->GetContent();
    }
};

class LevelFormatItem : public LogFormatter::FormatItem {
public:
    LevelFormatItem(const std::string& str = "") {}
    void Format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << LogLevel::ToString(level);
    }
};

class ElapseFormatItem : public LogFormatter::FormatItem {
public:
    ElapseFormatItem(const std::string& str = "") {}
    void Format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->GetElapse();
    }
};

class NameFormatItem : public LogFormatter::FormatItem {
public:
    NameFormatItem(const std::string& str = "") {}
    void Format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->GetLogger()->GetName();
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
    ThreadIdFormatItem(const std::string& str = "") {}
    void Format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->GetThreadId();
    }
};

class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
    FiberIdFormatItem(const std::string& str = "") {}
    void Format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->GetFiberId();
    }
};

class ThreadNameFormatItem : public LogFormatter::FormatItem {
public:
    ThreadNameFormatItem(const std::string& str = "") {}
    void Format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->GetThreadName();
    }
};

class DateTimeFormatItem : public LogFormatter::FormatItem {
public:
    DateTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S")
        :format_(format) {
        if(format_.empty()) {
            format_ = "%Y-%m-%d %H:%M:%S";
        }
    }

    void Format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        struct tm tm;
        time_t time = event->GetTime();
        localtime_r(&time, &tm);
        char buf[64];
        strftime(buf, sizeof(buf), format_.c_str(), &tm);
        os << buf;
    }
private:
    std::string format_;
};

class FilenameFormatItem : public LogFormatter::FormatItem {
public:
    FilenameFormatItem(const std::string& str = "") {}
    void Format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->GetFile();
    }
};

class LineFormatItem : public LogFormatter::FormatItem {
public:
    LineFormatItem(const std::string& str = "") {}
    void Format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->GetLine();
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem {
public:
    NewLineFormatItem(const std::string& str = "") {}
    void Format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << std::endl;
    }
};


class StringFormatItem : public LogFormatter::FormatItem {
public:
    StringFormatItem(const std::string& str)
        :string_(str) {}
    void Format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << string_;
    }
private:
    std::string string_;
};

class TabFormatItem : public LogFormatter::FormatItem {
public:
    TabFormatItem(const std::string& str = "") {}
    void Format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << "\t";
    }
private:
    std::string string_;
};


LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level
            ,const char* file, int32_t line, uint32_t elapse
            ,uint32_t thread_id, uint32_t fiber_id, uint64_t time
            ,const std::string& thread_name)
    :file_(file)
    ,line_(line)
    ,elapse_(elapse)
    ,thread_id_(thread_id)
    ,fiber_id_(fiber_id_)
    ,time_(time)
    ,thread_name_(thread_name)
    ,logger_(logger)
    ,level_(level) {
}

Logger::Logger(const std::string& name)
    :name_(name)
    ,level_(LogLevel::DEBUG) {
    formatter_.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
}

void Logger::SetFormatter(LogFormatter::ptr val) {
    Spinlock::Lock l(mutex_);
    formatter_ = val;

    for(auto& i : appenders_) {
        MutexType::Lock ll(i->mutex_);
        if(!i->HasFormatter()) {
            i->SetFormatter(formatter_);
        }
    }
}

void Logger::SetFormatter(const std::string& val) {
    LogFormatter::ptr new_val(new LogFormatter(val));
    if(new_val->IsError()) {
        printf("Logger SetFormatter name = %s value = %s invalid formatter\n", name_, val);
        return;
    }
    SetFormatter(new_val);
}


LogFormatter::ptr Logger::GetFormatter() {
    MutexType::Lock lock(mutex_);
    return formatter_;
}

void Logger::AddAppender(LogAppender::ptr appender) {
    MutexType::Lock lock(mutex_);
    if (appender->HasFormatter()) {
        appender->SetFormatter(formatter_);
    }
    appenders_.push_back(appender);
}

void Logger::DelAppender(LogAppender::ptr appender) {
    MutexType::Lock lock(mutex_);
    for(auto it = appenders_.begin();
            it != appenders_.end(); ++it) {
        if(*it == appender) {
            appenders_.erase(it);
            break;
        }
    }
}

void Logger::ClearAppenders() {
    MutexType::Lock lock(mutex_);
    appenders_.clear();
}

void Logger::Log(LogLevel::Level level, LogEvent::ptr event) {
    if (level >= level_) {
        auto self = shared_from_this();
        MutexType::Lock lock(mutex_);
        if (!appenders_.empty()) {
            for(auto& i : appenders_) {
                i->Log(self, level, event);
            }
        } else if (root_) {
            root_->Log(level, event);
        }
    }
}

void Logger::Debug(LogEvent::ptr event) {
    Log(LogLevel::DEBUG, event);
}

void Logger::Info(LogEvent::ptr event) {
    Log(LogLevel::INFO, event);
}

void Logger::Warn(LogEvent::ptr event) {
    Log(LogLevel::WARN, event);
}

void Logger::Error(LogEvent::ptr event) {
    Log(LogLevel::ERROR, event);
}

void Logger::Fatal(LogEvent::ptr event) {
    Log(LogLevel::FATAL, event);
}

FileLogAppender::FileLogAppender(const std::string& filename)
    :file_name_(filename) {
    reopen();
}

void FileLogAppender::Log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    if(level >= level_) {
        uint64_t now = event->GetTime();
        if(now >= (last_time_ + 3)) {
            reopen();
            last_time_ = now;
        }
        MutexType::Lock lock(mutex_);
        if(!formatter_->Format(file_stream_, logger, level, event)) {
            printf("error\n");
        }
    }
}

bool FileLogAppender::reopen() {
    MutexType::Lock lock(mutex_);
    if(file_stream_) {
        file_stream_.close();
    }
    return FSUtil::OpenForWrite(file_stream_, file_name_, std::ios::app);
}

void StdoutLogAppender::Log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    if(level >= level_) {
        MutexType::Lock lock(mutex_);
        formatter_->Format(std::cout, logger, level, event);
    }
}

LogFormatter::LogFormatter(const std::string& pattern)
    :pattern_(pattern) {
    Init();
}

std::string LogFormatter::Format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    std::stringstream ss;
    for(auto& i : items_) {
        i->Format(ss, logger, level, event);
    }
    return ss.str();
}

std::ostream& LogFormatter::Format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    for(auto& i : items_) {
        i->Format(ofs, logger, level, event);
    }
    return ofs;
}

//%xxx %xxx{xxx} %%
void LogFormatter::Init() {
    //str, format, type
    std::vector<std::tuple<std::string, std::string, int> > vec;
    std::string nstr;
    for(size_t i = 0; i < pattern_.size(); ++i) {
        if (pattern_[i] != '%') {
            nstr.append(1, pattern_[i]);
            continue;
        }

        if ((i + 1) < pattern_.size()) {
            if (pattern_[i + 1] == '%') {
                nstr.append(1, '%');
                ++i;
                continue;
            }
        }

        size_t n = i + 1;
        int fmt_status = 0;
        size_t fmt_begin = 0;

        std::string str;
        std::string fmt;
        while (n < pattern_.size()) {
            if (!fmt_status && (!isalpha(pattern_[n]) && pattern_[n] != '{'
                    && pattern_[n] != '}')) {
                str = pattern_.substr(i + 1, n - i - 1);
                break;
            }
            if (fmt_status == 0) {
                if(pattern_[n] == '{') {
                    str = pattern_.substr(i + 1, n - i - 1);
                    fmt_status = 1;
                    fmt_begin = n;
                    ++n;
                    continue;
                }
            } else if (fmt_status == 1) {
                if (pattern_[n] == '}') {
                    fmt = pattern_.substr(fmt_begin + 1, n - fmt_begin - 1);
                    fmt_status = 0;
                    ++n;
                    break;
                }
            }
            ++n;
            if (n == pattern_.size()) {
                if (str.empty()) {
                    str = pattern_.substr(i + 1);
                }
            }
        }

        if (fmt_status == 0) {
            if (!nstr.empty()) {
                vec.push_back(std::make_tuple(nstr, std::string(), 0));
                nstr.clear();
            }
            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n - 1;
        } else if (fmt_status == 1) {
            printf("pattern parse error: %s%s%s\n", pattern_, " - ", pattern_.substr(i));
            error_ = true;
            vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
        }
    }

    if (!nstr.empty()) {
        vec.push_back(std::make_tuple(nstr, "", 0));
    }
    static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)> > s_format_items = {
#define XX(str, C) \
        {#str, [](const std::string& fmt) { return FormatItem::ptr(new C(fmt));}}

        XX(m, MessageFormatItem),
        XX(p, LevelFormatItem),
        XX(r, ElapseFormatItem),
        XX(c, NameFormatItem),
        XX(t, ThreadIdFormatItem),
        XX(n, NewLineFormatItem),
        XX(d, DateTimeFormatItem),
        XX(f, FilenameFormatItem),
        XX(l, LineFormatItem),
        XX(T, TabFormatItem),
        XX(F, FiberIdFormatItem),
        XX(N, ThreadNameFormatItem),
#undef XX
    };

    for (auto& i : vec) {
        if (std::get<2>(i) == 0) {
            items_.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        } else {
            auto it = s_format_items.find(std::get<0>(i));
            if (it == s_format_items.end()) {
                items_.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                error_ = true;
            } else {
                items_.push_back(it->second(std::get<1>(i)));
            }
        }

    }
}


LoggerManager::LoggerManager() {
    root_.reset(new Logger);
    root_->AddAppender(LogAppender::ptr(new StdoutLogAppender));

    logger_s[root_->name_] = root_;
}

Logger::ptr LoggerManager::GetLogger(const std::string& name) {
    MutexType::Lock lock(mutex_);
    auto it = logger_s.find(name);
    if(it != logger_s.end()) {
        return it->second;
    }

    Logger::ptr logger(new Logger(name));
    logger->root_ = root_;
    logger_s[name] = logger;
    return logger;
}

struct LogAppenderDefine {
    int type = 0; //1 File, 2 Stdout
    LogLevel::Level level = LogLevel::UNKNOW;
    std::string formatter;
    std::string file;

    bool operator==(const LogAppenderDefine& oth) const {
        return type == oth.type
            && level == oth.level
            && formatter == oth.formatter
            && file == oth.file;
    }
};

struct LogDefine {
    std::string name;
    LogLevel::Level level = LogLevel::UNKNOW;
    std::string formatter;
    std::vector<LogAppenderDefine> appenders;

    bool operator==(const LogDefine& oth) const {
        return name == oth.name
            && level == oth.level
            && formatter == oth.formatter
            && appenders == appenders;
    }

    bool operator<(const LogDefine& oth) const {
        return name < oth.name;
    }

    bool isValid() const {
        return !name.empty();
    }
};

}