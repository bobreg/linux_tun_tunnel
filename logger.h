#ifndef logger_h_included
#define logger_h_included

#include <qfile.h>
#include <qtextstream.h>
#include <qstring.h>
#include <QFile>

#include <QDebug>
#include <QObject>
#include <QDateTime>
#include <QDir>

// define для вызова лога в любой части программы
#define Log(str) g_log->logIt(str);

class Logger : public QObject
{
      Q_OBJECT
public:
    enum LOG_LEVEL
    {
        LOG_OFF = 0,
        LOG_MESSAGES = 1,
        LOG_PACKETS = 2,
        LOG_ALL = 3
    };

    Logger(QString log_name, unsigned int log_level = LOG_OFF, bool out_console = false, QObject *parent = nullptr);
    ~Logger();

    void logOpen();
    void logClose();

    void logIt(const QString & message, unsigned int log_level = LOG_MESSAGES);
    void logIt(const QString & head, const char  * packet, int length, unsigned int log_level = LOG_PACKETS);
    void logIt(const QString & head, const short * packet, int length, unsigned int log_level = LOG_PACKETS);

    // === logIt() << QString << QString << endl;
    Logger & logIt(unsigned int log_level = LOG_MESSAGES);
    Logger & operator<<(const QString & data);
    Logger & endLogLine(Logger & logger);
    // ===

    void setChanged(bool on);

    unsigned int logLevel();
    void setLogLevel(unsigned int log_level);

    void setOutConsole(bool out_console);

    void clearTheOldestLog(QString path, unsigned int period = 30);

public slots:
    void logItSlot(QString);

private:    
    LOG_LEVEL m_loglevel;
    QString m_logname;
    QFile *m_logfile;
    QTextStream *m_logstream;
    bool m_console;
};

extern Logger * g_log;

#endif
