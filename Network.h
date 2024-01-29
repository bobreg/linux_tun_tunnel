#ifndef NETWORK_H_INCLUDED
#define NETWORK_H_INCLUDED

#include <QObject>
#include <QTimer>
#include <QDebug>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QHostAddress>
#include <QtEndian>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

//----------------------------------------------------------------------------------------------------------------

const int NET_LISTEN_PORT=24000;
const int NET_MAX_SOCKETS=1;
const int NET_MAX_PORTS=1;

class Network: public QObject
{
    Q_OBJECT

    int get_sender();

    QHostAddress haOur;
    QHostAddress haHost;
    int hport;
    QTcpServer *servsock;
    QTcpSocket *sock[NET_MAX_SOCKETS];
    QTimer *timer;

public:
    Network();
    ~Network();

    bool init(int port = NET_LISTEN_PORT);  // создаёт сервер
    bool init(QHostAddress host, int port = NET_LISTEN_PORT);  // создаёт клиент
    void destroy();

private slots:
    void slotNewConnection();
    void hostFound();
    void connected();
    void connectionClosed();
    void delayedCloseFinished();
    void readyRead();
    void bytesWritten(qint64 nbytes);
    void error(QAbstractSocket::SocketError err);

    void timeout();

public slots:
    void sendReply(const char *buffer, unsigned int length);
    //void sendReply(QByteArray arr);
//    void slotConnect(QString address, int port);
//    void slotConnect(int port);
//    void slotDisconnect();

signals:
    void dataArrived(const char *buffer, unsigned int length);
    void sigConnectError();
    void sigConnected();
    void sigDisconnected();
};

//----------------------------------------------------------------------------------------------------------------
/**
 * @brief The Udp class
 */
class Udp : public QObject
{
    Q_OBJECT
public:
    explicit Udp(QObject *parent = nullptr) : QObject(parent){}
    bool init(quint16 localPort = 0, QString addr = "0.0.0.0", quint16 remotePort = 0);
    ~Udp();

signals:
    void dataArrived(QByteArray, int);

public slots:
    void sendDatagramm(const char *, int len);
private:
    QUdpSocket * m_socket = nullptr;
    QTimer m_aliveTimer;
    QTimer m_timeoutConnection;
    const int timeout = 15000;
    quint16 m_localPort = 0;
    quint16 m_remotePort = 0;
    QHostAddress m_remoteAddress = QHostAddress("0.0.0.0");
    bool isServer = false;
    bool isConnected = false;
    QByteArray alivePack = "0alive_1alive_2alive_3alive_4alive<$>";
    QByteArray buffer;
private slots:
    void readDatagrams();
};
#endif
