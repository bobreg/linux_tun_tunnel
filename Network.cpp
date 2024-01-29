#include "Network.h"

//----------------------------------------------------------------------------------------------------------------
Network::Network()
{
    haOur = QHostAddress(QHostAddress::AnyIPv4);
    servsock = 0;
    timer = 0;
    hport = 0;
    for(int j=0;j<NET_MAX_SOCKETS;j++) sock[j]=0;
}

//----------------------------------------------------------------------------------------------------------------
Network::~Network()
{
    connectionClosed();
    destroy();
}

//----------------------------------------------------------------------------------------------------------------
void Network::destroy()
{
    if(timer)
    {
        timer->stop();
        delete timer;
        timer = nullptr;
    }
    if(servsock)
    {
        delete servsock;
        servsock=0;
    }
    for(int j=0;j<NET_MAX_SOCKETS;j++)
    {
        if(sock[j])
        {
            sock[j]->close();
            delete sock[j];
            sock[j]=0;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------
bool Network::init(int port)
{
    for(int i=0;i<NET_MAX_SOCKETS;i++) sock[i]=0;

    hport = port;
    servsock = new QTcpServer(this);
    connect(servsock,SIGNAL(newConnection()),this,SLOT(slotNewConnection()));

    if(!servsock->listen(haOur,port))
    {
        qDebug("listen error on port %d at address %s",port,qPrintable(haOur.toString()));
        return false;
    }
    return true;
}

//----------------------------------------------------------------------------------------------------------------
bool Network::init(QHostAddress host, int port)
{
    for(int i=0;i<NET_MAX_SOCKETS;i++) sock[i]=0;

    haHost = host;
    hport = port;

    sock[0] = new QTcpSocket(this);

    //----настрока контроля соединения----
    int enableKeepAlive = 1;
    int fd = sock[0]->socketDescriptor();
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &enableKeepAlive, sizeof(enableKeepAlive));
    int maxIdle = 10; /* seconds */
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &maxIdle, sizeof(maxIdle));

    int count = 3;  // send up to 3 keepalive packets out, then disconnect if no response
    setsockopt(fd, SOL_TCP, TCP_KEEPCNT, &count, sizeof(count));

    int interval = 2;   // send a keepalive packet out every 2 seconds (after the 5 second idle period)
    setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
    //------------------------------------


/*
    if(!sock[0]->socketDevice()->bind(haOur,0))
    {
        qDebug("bind error at address %s",(const char*)(haOur.toString()));
        return true;
    }
*/
    connect(sock[0],SIGNAL(hostFound()),this,SLOT(hostFound()));
    connect(sock[0],SIGNAL(connected()),this,SLOT(connected()));
    connect(sock[0],SIGNAL(disconnected()),this,SLOT(connectionClosed()));
    connect(sock[0],SIGNAL(readyRead()),this,SLOT(readyRead()));
    connect(sock[0],SIGNAL(bytesWritten(qint64)),this,SLOT(bytesWritten(qint64)));
    connect(sock[0],SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(error(QAbstractSocket::SocketError)));

    timer = new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(timeout()));
    timer->start(1000);

    return true;
}

//----------------------------------------------------------------------------------------------------------------
int Network::get_sender()
{
    for(int i=0;i<NET_MAX_SOCKETS;i++)
        if(sender()==sock[i]) return i;
    return -1;
}

//----------------------------------------------------------------------------------------------------------------
void Network::hostFound()
{
//    int nsock = get_sender();
//    qDebug("%d sig hostFound",nsock+1);
}

//----------------------------------------------------------------------------------------------------------------
void Network::connected()
{
//    int nsock = get_sender();
//    qDebug("%d sig connected",nsock+1);
    emit sigConnected();
}

//----------------------------------------------------------------------------------------------------------------
void Network::connectionClosed()
{
    int nsock = get_sender();
//    qDebug("%d sig connectionClosed",nsock+1);
    if(servsock)
    {
        if(sock[nsock])
        {
            QTcpSocket *skt = sock[nsock];
            sock[nsock]=0;
            skt->close();
            skt->deleteLater();
        }
    }
    emit sigDisconnected();
}

//----------------------------------------------------------------------------------------------------------------
void Network::delayedCloseFinished()
{
//    int nsock = get_sender();
//    qDebug("%d sig delayCloseFinished",nsock+1);
    emit sigDisconnected();
}

//----------------------------------------------------------------------------------------------------------------
void Network::readyRead()
{
    int nsock = get_sender();
    static char buf[65536]={0};

    int available = sock[nsock]->bytesAvailable();
//	qDebug("%d sig readyRead=%d",nsock+1,available);

    while(available > 0)
    {
        int bytesToRead = 0;
        int bytesRead = 0;

        if(available > 65536)
            bytesToRead = 65536;
        else
            bytesToRead = available;

        bytesRead = sock[nsock]->read(buf,bytesToRead);
        if(bytesRead < 0)
        {
            qDebug("%d read error",nsock+1);
            return;
        }

        emit dataArrived((const char*)buf,bytesRead);
//		available -= bytesRead;
        available = sock[nsock]->bytesAvailable();
    }
}

//----------------------------------------------------------------------------------------------------------------
void Network::bytesWritten(qint64 /*nbytes*/)
{
//    int nsock = get_sender();
//    qDebug("%d sig bytesWritten = %d",nsock+1,nbytes);
}

//----------------------------------------------------------------------------------------------------------------
void Network::error(QAbstractSocket::SocketError /*err*/)
{
//    int nsock = get_sender();
//    qDebug("%d sig error (%s)",nsock+1,qPrintable(sock[nsock]->errorString()));
}

//----------------------------------------------------------------------------------------------------------------
void Network::slotNewConnection()
{
//    QSocket *skt = new QTcpSocket(this);
    QTcpSocket *skt = servsock->nextPendingConnection();
    if(!skt)
        return;

//    qDebug("newConnection");

    connect(skt,SIGNAL(hostFound()),this,SLOT(hostFound()));
    connect(skt,SIGNAL(connected()),this,SLOT(connected()));
    connect(skt,SIGNAL(disconnected()),this,SLOT(connectionClosed()));
//    connect(skt,SIGNAL(delayedCloseFinished()),this,SLOT(delayedCloseFinished()));
    connect(skt,SIGNAL(readyRead()),this,SLOT(readyRead()));
    connect(skt,SIGNAL(bytesWritten(qint64)),this,SLOT(bytesWritten(qint64)));
    connect(skt,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(error(QAbstractSocket::SocketError)));

//    skt->setSocket(socket);

    int num_sock;
//    qDebug("peerAddress=%s",qPrintable(skt->peerAddress().toString()));

    for(num_sock=0;num_sock<NET_MAX_SOCKETS;num_sock++)
        if(!sock[num_sock])
            break;

    if(num_sock==NET_MAX_SOCKETS)
    {
        qDebug("No more sockets: Closing connection");
        skt->disconnect();
        skt->waitForDisconnected(1000);
        delete skt;
        //---было так---
        // skt->close();
        // skt->deleteLater();
        //--------------
        return;
    }

    if(sock[num_sock])
    {
        sock[num_sock]->close();
        delete sock[num_sock];
        sock[num_sock]=0;
    }

    //----настрока контроля соединения----
    int enableKeepAlive = 1;
    int fd = skt->socketDescriptor();
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &enableKeepAlive, sizeof(enableKeepAlive));
    int maxIdle = 10; /* seconds */
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &maxIdle, sizeof(maxIdle));

    int count = 3;  // send up to 3 keepalive packets out, then disconnect if no response
    setsockopt(fd, SOL_TCP, TCP_KEEPCNT, &count, sizeof(count));

    int interval = 2;   // send a keepalive packet out every 2 seconds (after the 5 second idle period)
    setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
    //------------------------------------

    sock[num_sock]=skt;
//    qDebug("accepted connection: num_sock=%d",num_sock);

    emit sigConnected();
}

void Network::sendReply(const char *buffer, unsigned int length)
{
    //QByteArray arr(buffer, length);
    //qDebug() << "Network: " << arr;
    //qDebug() << "Network: " << arr.toHex(':');
    for(int i=0;i<NET_MAX_SOCKETS;i++)
    {
        if(sock[i])
        {
            if(sock[i]->state() == QAbstractSocket::ConnectedState)
            {
                if(sock[i]) sock[i]->write(buffer,length);
                if(sock[i]) sock[i]->flush();
            }
        }
    }
}

/*//----------------------------------------------------------------------------------------------------------------
void Network::slotConnect(QString address, int port)
{
    QHostAddress ha;
    if(!ha.setAddress(address) || init(ha,port))
        emit sigConnectError();
}

//----------------------------------------------------------------------------------------------------------------
void Network::slotConnect(int port)
{
    if(init(port))
        emit sigConnectError();
}

//----------------------------------------------------------------------------------------------------------------
void Network::slotDisconnect()
{
    destroy();
    emit sigDisconnected();
}*/

//----------------------------------------------------------------------------------------------------------------
void Network::timeout()
{
    static quint32 connectCounter[NET_MAX_SOCKETS] = {0};
    for(int i=0;i<NET_MAX_SOCKETS;i++)
    {
        if(sock[i])
        {
            switch(sock[i]->state())
            {
            case QAbstractSocket::UnconnectedState:
            //qDebug("connectToHost: %s:%d",qPrintable(haHost.toString()),hport);
                sock[i]->close();
                sock[i]->connectToHost(haHost.toString(),hport);
                connectCounter[i]=0;
                break;
            case QAbstractSocket::HostLookupState:
                break;
            case QAbstractSocket::ConnectingState:
                connectCounter[i]++;
                if(connectCounter[i]>100)
                {
                  connectCounter[i]=0;
            //qDebug("connectToHost: %s:%d",qPrintable(haHost.toString()),hport);
                  sock[i]->close();
                  sock[i]->waitForDisconnected(10);
                  sock[i]->connectToHost(haHost.toString(),hport);
                }
                break;
            case QAbstractSocket::ConnectedState:
                break;
            case QAbstractSocket::ClosingState:
                break;
            case QAbstractSocket::BoundState:
                break;
            case QAbstractSocket::ListeningState:
                break;
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------


bool Udp::init(quint16 localPort, QString addr, quint16 remotePort)
{
    bool isBind;
    m_socket = new QUdpSocket(this);
    m_remoteAddress = QHostAddress(addr);
    m_localPort = localPort;
    m_remotePort = remotePort;
    isServer = (addr == "0.0.0.0") ? true : false;

    if(isServer){
        isBind = m_socket->bind(QHostAddress::Any, m_localPort);
    }else{
        isBind = m_socket->bind(m_localPort);
    }

    if(isBind){
        connect(m_socket, &QUdpSocket::readyRead, this, &Udp::readDatagrams);
        connect(&m_aliveTimer, &QTimer::timeout, this, [=](){sendDatagramm(alivePack, alivePack.length());});
        connect(&m_timeoutConnection, &QTimer::timeout, this, [=](){qDebug() << "Connection lost!";
                                                                    isConnected = false;
                                                                    m_timeoutConnection.stop();});

        if(!isServer){
            sendDatagramm(alivePack, alivePack.length());
            m_aliveTimer.start(10000);
            //m_timeoutConnection.start(timeout);
            qDebug() << "Start udp alive requests on:" << m_remoteAddress << m_remotePort;
        }
    }else{
        delete m_socket;
        m_socket = nullptr;
    }
    return isBind;
}

Udp::~Udp(){
    if(m_socket != nullptr) delete m_socket;
    //qDebug() << "Udp: delete";
}


void Udp::sendDatagramm(const char * arr, int len){
    QByteArray m = "[{<$%";
    m.append(QByteArray::fromRawData(arr, len));
    m.append("%$>}]");
    qint64 nwrite = m_socket->writeDatagram(m, m_remoteAddress, m_remotePort);
    if(nwrite < 0){
        qWarning() << "Udp socket nwrite: " << nwrite;
        qDebug() << m_remoteAddress << m_remotePort;
    }
}

void Udp::readDatagrams(){
    QByteArray arr;
    QByteArray data;
    arr.resize(qint32(m_socket->pendingDatagramSize()));
    if(arr.length() != 0){
        QHostAddress address;
        quint16 port;
        m_socket->readDatagram(arr.data(), arr.size(), &address, &port);

        buffer.append(arr);
        //qDebug() << arr;
        int start = buffer.indexOf("[{<$%");
        int stop = buffer.indexOf("%$>}]");
        if(start != -1 && stop != -1 && start < stop){
            data = buffer.mid(start + 5, stop - start - 5);
            buffer.remove(0, stop - start + 5);
            //qDebug() << buffer << data;
        }else{
            return;
        }
        if(data == alivePack){
            m_timeoutConnection.stop();
            if(!isConnected){
                qDebug() << "Connection established with: " << address << port;
                isConnected = true;
                if(isServer){
                    m_remoteAddress = address;
                    m_remotePort = port;
                    sendDatagramm(alivePack, alivePack.length());
                }
            }else{
                if(isServer){
                    sendDatagramm(alivePack, alivePack.length());
                }
            }
            m_timeoutConnection.start(timeout);
        }else{
            emit dataArrived(data, data.length());
        }

        //-----if buffer is oversized-------
        if(buffer.length() > 100000){
            qWarning() << "Udp buffer's oversized!";
            buffer.resize(0);
        }
    }
}
