#include <qdatetime.h>
#include <math.h>
#include "logger.h"
#include "stdarg.h"


Logger * g_log = nullptr;

//------------------------------------------------------------------------------
Logger::Logger(QString log_name, unsigned int log_level, bool out_console, QObject *parent) : QObject(parent)
{
    m_logname = log_name;
    m_loglevel = (LOG_LEVEL)log_level;
    m_logfile = NULL;
    m_logstream = NULL;
    m_console = out_console;
    if(log_level != LOG_OFF){
        logOpen();
    }
}

//------------------------------------------------------------------------------
Logger::~Logger()
{
    logClose();
}

//------------------------------------------------------------------------------
void Logger::setChanged(bool on)
{
    if(on && m_loglevel == LOG_OFF)
    {
        m_loglevel = LOG_ALL;
        logOpen();
    }
    else if(!on && m_loglevel != LOG_OFF)
    {
        logClose();
        m_loglevel = LOG_OFF;
    }
}

//------------------------------------------------------------------------------
unsigned int Logger::logLevel()
{
    return (unsigned int)m_loglevel;
}

//------------------------------------------------------------------------------
void Logger::setLogLevel(unsigned int log_level)
{
    m_loglevel = (LOG_LEVEL)log_level;
}

//------------------------------------------------------------------------------
void Logger::setOutConsole(bool out_console)
{
    m_console = out_console;
}

//------------------------------------------------------------------------------
void Logger::clearTheOldestLog(QString path, unsigned int period)
{
    QDateTime today = QDateTime::currentDateTime();
    QDir logPath(path);
    QStringList logFiles = logPath.entryList(QDir::Files);
    for(auto i : logFiles){
        QFile file(path + '/' + i);
        QFileInfo infoFile(file);

        if(infoFile.birthTime().daysTo(today) > period){
            file.remove();
        }
    }
}

//------------------------------------------------------------------------------
void Logger::logOpen()
{
    m_logfile = NULL;
    m_logstream = NULL;
    if(m_loglevel != LOG_OFF)
    {
        m_logfile = new QFile(m_logname);
        if(m_logfile)
        {
            if(m_logfile->open(QIODevice::WriteOnly | QIODevice::Append))
            {
                m_logstream = new QTextStream(m_logfile);
                if(m_logstream)
                {
                    logIt("--------------- log open --------------- " + m_logname);
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
void Logger::logClose()
{
    if(m_logfile)
    {
        if(m_logstream)
        {
            logIt("--------------- log close -------------- " + m_logname);
            *m_logstream << flush;
            delete m_logstream;
            m_logstream = NULL;
        }
        m_logfile->flush();
        m_logfile->close();
        delete m_logfile;
        m_logfile = NULL;
    }
}

//------------------------------------------------------------------------------
void Logger::logIt(const QString & message, unsigned int log_level)
{
    if(m_loglevel & log_level)
    {
        if(m_console)
        {
            qDebug() << message;
            const unsigned int maxbuf = 4096;
            unsigned int divider = message.length() / maxbuf;
            unsigned int quotient = message.length() % maxbuf;

            if(quotient > 0)
                divider++;

            for(unsigned int i = 0; i < divider; i++)
            {
                if(divider > 1)
                    qDebug("(Part %d of %d)", i + 1, divider);
                qDebug("%s", (const char*)(message.mid(i * maxbuf, maxbuf).constData()));
            }
        }

        if(m_logstream)
        {
            QString strdate = QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss.zzz");
            *m_logstream << strdate << "\t" << message << endl;
            *m_logstream << flush;
        }
    }
}

//------------------------------------------------------------------------------
void Logger::logIt(const QString & head, const char* data, int nbyte, unsigned int log_level)
{
    if((m_loglevel & log_level) && m_logstream && data && nbyte > 0)
    {
        QString sT(head);
        QString sTT;

        for(int kk = 0; kk < nbyte; kk++)
        {
            sTT.sprintf("%02X ",(unsigned char)data[kk]);
            sT += sTT;
        }

        logIt(sT, log_level);
    }
}

//------------------------------------------------------------------------------
void Logger::logIt(const QString & head, const short* data, int nword, unsigned int log_level)
{
    if((m_loglevel & log_level) && m_logstream && data && nword > 0)
    {
        QString sT(head);
        QString sTT;

        for(int kk = 0; kk < nword; kk++)
        {
            sTT.sprintf("%04X ",(unsigned short)data[kk]);
            sT += sTT;
        }

        logIt(sT, log_level);
    }
}

//------------------------------------------------------------------------------
Logger & Logger::logIt(unsigned int log_level)
{
    Q_UNUSED(log_level);
    return *this;
}

//------------------------------------------------------------------------------
Logger & Logger::operator<<(const QString & data)
{
    Q_UNUSED(data);
    return *this;
}

//------------------------------------------------------------------------------
Logger & Logger::endLogLine(Logger & logger)
{
    Q_UNUSED(logger);
    return *this;
}

//------------------------------------------------------------------------------

void Logger::logItSlot(QString str){
    logIt(str, LOG_MESSAGES);
    //qDebug() << "Logger::logItslot" << str;
}
