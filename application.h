#ifndef APPLICATION_H
#define APPLICATION_H

#include <QCoreApplication>
#include <QSettings>
#include <QDebug>
#include <QFile>
#include <QDateTime>
#include <QThread>
#include <QtEndian>
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QProcess>
#include <QTimer>
#include "globals.h"
#include "Network.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <netinet/in.h>

#define IFNAMSIZ 16

class TunThread : public QThread
{
    Q_OBJECT
public:

    int tun_fd = 0;
    struct ifreq ifr;
    bool flag = false;
    char buffer[1500];
    QTimer timer;
    unsigned long allpackage = 0;
    unsigned long brockenp = 0;
    unsigned long lostp = 0;
    unsigned long successp = 0;
    unsigned long alloutp = 0;

    TunThread(){
        tun_fd = open("/dev/net/tun", O_RDWR);
        qDebug() << "tun_fd = " << tun_fd;
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
        strncpy(ifr.ifr_name, g_settings->value("settings/tun_name", "tun69").toByteArray(), IFNAMSIZ);
        ioctl(tun_fd, TUNSETIFF, (void *)&ifr);
        flag = true;

        connect(&timer, &QTimer::timeout, this, &TunThread::showStatistics);
        timer.start(10000);
    }


    void run() override{
        while (flag) {
            int nread = read(tun_fd, buffer, sizeof(buffer));
            emit sendReply((const char*)buffer, nread);
            alloutp++;
        }
    }

    ~TunThread(){
        flag = false;
        close(tun_fd);
        tun_fd = 0;
    }

public slots:
    void setData(const char* ch, int len){
        int nwrite = write(tun_fd, ch, len);
        if (nwrite < 0) {
            //qWarning() << "nwrite:" << nwrite;
            lostp++;
        }else if(nwrite != len){
            brockenp++;
        }else if(nwrite == len){
            successp++;
        }
        allpackage++;
    }

    void showStatistics(){
        QString str = "Incoming packages: all: " + QString::number(allpackage) +
                        " success: " + QString::number(successp) +
                        " lost: " + QString::number(lostp + brockenp) +
                        "\r\n" + " Outcoming packages: " + QString::number(alloutp);

        emit updateStatistics(str);
    }

signals:
    void newData(QByteArray);
    void sendReply(const char*, unsigned int);
    void updateStatistics(QString);
};

class Application : public QCoreApplication
{
    Q_OBJECT
public:
    explicit Application(int & argc, char** argv);
    ~Application();

signals:
    void writeData(const char*, unsigned int);
    void sendReply(const char*, unsigned int); // отправить кодограмму по сети;

public slots:

private slots:

    void updateStatistics(QString);

    void getSettings();
    void setDefaultSettings();
    void setSettingParameter(QString field, QString key, QVariant value);

private:

    unsigned short calculateChecksum(const unsigned short* data, int length);

    TunThread * tun;
    Network * m_network;
    Udp * m_udp;

    QFile statFile;
    QTextStream stream;


    QString m_PathToExecutable;
    QString m_pathToConfig;
    QString m_logPath;
    int m_logExpiration;
    QSettings * m_settings;
};

#endif // APPLICATION_H
