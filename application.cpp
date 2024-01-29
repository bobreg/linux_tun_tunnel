#include "application.h"
#include "logger.h"

QSettings * g_settings = nullptr;
/*
 * sudo ip tuntap add dev tun0 mode tun
 * sudo ip addr add 10.0.0.1/24 dev tun0
 * sudo ip link set up dev tun0
 *
 * nc -u 10.0.0.2 2222
 * nc -luvp 2223
 * sudo tcpdump -i tun0 -v
 * sudo sysctl net.ipv4.conf.tun0.accept_local=1
 */




Application::Application(int & argc, char** argv) : QCoreApplication(argc, argv)
{
    m_PathToExecutable = qApp->applicationDirPath(); // сначала узнаем место нахождения executable.
    m_pathToConfig = m_PathToExecutable + "/settings.ini";
    //---проверим аргументы запуска---
    qDebug() << "Arguments: ";
    for(quint8 i = 0; i < argc; i++){
        qDebug() << argv[i];
        if(strcmp(argv[i], "-m") == 0){
            qDebug() << "got:" << argv[i];
            break;
        }else if(strcmp(argv[i], "-config") == 0 && i < argc){
            qDebug() << "got config path:" << argv[i] << argv[i + 1];
            if(QFile::exists(argv[i + 1])){
                m_pathToConfig = argv[i + 1];
            }else{
                qDebug() << argv[i + 1] << " is invalid!";
            }
        }
    }

    getSettings();

    //---Настройка логгирования---
    QDateTime today = QDateTime::currentDateTime();
    if(!QDir(m_logPath).exists()) QDir().mkpath(m_logPath);
    g_log = new Logger(m_logPath + "/log_" + today.toString("dd-MM-yyyy") + ".txt", Logger::LOG_MESSAGES,false, this);
    g_log->clearTheOldestLog(m_logPath, m_logExpiration);  // удаление устаревших файлов
    //--------------------------
    //---подъём tun интерфейса---

    QString tun_name = g_settings->value("settings/tun_name", "tun69").toString();
    QString tun_ip = g_settings->value("settings/tun_ip", "10.0.0.1").toString();
    QString tun_mask = g_settings->value("settings/tun_mask", "24").toString();

    QProcess p;
    p.start("sudo", QStringList() << "ifconfig");
    p.waitForFinished();
    QString res = p.readAllStandardOutput();
    if(res.indexOf(tun_name) == -1){
        p.start("sudo", QStringList() << "ip"<< "tuntap"<< "add"<< "dev"<< tun_name << "mode" << "tun");
        p.waitForFinished();
        p.start("sudo", QStringList() << "ip"<< "addr"<< "add"<< tun_ip + "/" + tun_mask << "dev" << tun_name);
        p.waitForFinished();
        p.start("sudo", QStringList() << "ip"<< "link"<< "set" << "dev" << tun_name << "mtu" << "1400");
        p.waitForFinished();
        p.start("sudo", QStringList() << "ip"<< "link"<< "set"<< "up" << "dev"<< tun_name);
        p.waitForFinished();
    }
    //--------------------------

    bool isServer = g_settings->value("settings/server", true).toBool();
    QString remoteHostIp = g_settings->value("settings/remote_server_addr", "127.0.0.1").toString();
    int remoteHostPort = g_settings->value("settings/port", 6996).toInt();
    qDebug() << "Is server:" << isServer << "Port:" << remoteHostPort << "IP address:" << remoteHostIp;

    m_network = new Network();
//    connect(m_network, &Network::sigConnected, this, [=]{qDebug() << "Connection established";});
//    connect(m_network, &Network::sigDisconnected, this, [=]{qDebug() << "Connection lost";});
//    (isServer) ? m_network->init(remoteHostPort) : m_network->init(QHostAddress(remoteHostIp), remoteHostPort);

    m_udp = new Udp();
    bool udpFlag = (isServer) ? m_udp->init(remoteHostPort) : m_udp->init(remoteHostPort, remoteHostIp, remoteHostPort);
    qDebug() << "Udp socket status:" << udpFlag;

    tun = new TunThread();
    connect(tun, &TunThread::sendReply, m_udp, &Udp::sendDatagramm, Qt::DirectConnection);
    connect(m_udp, &Udp::dataArrived, tun, &TunThread::setData, Qt::DirectConnection);
    connect(tun, &TunThread::updateStatistics, this, &Application::updateStatistics);
    tun->start();

    statFile.setFileName("statistics.txt");
    stream.setDevice(&statFile);

    qDebug() << "The program was started";
    Log("The program was started");



}

Application::~Application()
{
    delete g_settings;
    delete tun;
}

void Application::updateStatistics(QString str)
{
    if(statFile.open(QFile::Truncate | QFile::WriteOnly)){
        stream << str;
    }else{
        qWarning() << "Can't write statistics into statistics.txt";
    }
    statFile.close();
}

// Function to calculate checksum for IP header
unsigned short Application::calculateChecksum(const unsigned short* data, int length) {
    unsigned long sum = 0;
    while (length > 1) {
        sum += *data++;
        length -= 2;
    }
    // If there's a remaining byte, add it to the sum (padding)
    if (length > 0) {
        sum += *(unsigned char*)data;
    }
    // Fold the 32-bit sum to 16 bits
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return static_cast<unsigned short>(~sum);
}


//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void Application::getSettings()
{
    g_settings = new QSettings(m_pathToConfig, QSettings::IniFormat, this);
    m_logPath = g_settings->value("settings/log_path", m_PathToExecutable + "/log").toString();
    if(m_logPath.indexOf("~") != -1){
        m_logPath.replace(m_logPath.indexOf("~"), 1, QDir::homePath());
    }else{
        m_logPath = m_PathToExecutable + "/" + m_logPath;
    }
    m_logExpiration = g_settings->value("settings/log_expiration", 30).toInt();
    qDebug() << "Application::getSettings(): log path: "<< m_logPath;
}

void Application::setDefaultSettings()
{
    g_settings->setValue("settings/log_path", m_PathToExecutable + "/log");
    g_settings->setValue("settings/log_expiration", 30);
}

void Application::setSettingParameter(QString field, QString key, QVariant value)
{
    g_settings->setValue(field + "/" + key, value);
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------

//qDebug() << "++++++++++++++++++++++++++++++++++++++++";
//qDebug() << arr.toHex(':');

//QByteArray qheader = arr.left(24);
//qDebug() << qheader.toHex(':');
//qDebug() << "+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-";
//Ip_header ipHeader;
//memcpy(&ipHeader, qheader, qheader.length());
////qDebug() << QString::number(ipHeader.version_header_length, 16);
////qDebug() << QString::number(ipHeader.total_length, 16);
////qDebug() << QString::number(ipHeader.identification, 16);
////qDebug() << QString::number(ipHeader.flags_fragment_offset, 16);
////qDebug() << QString::number(ipHeader.ttl, 16);
////qDebug() << QString::number(ipHeader.protocol, 16);
////qDebug() << QString::number(ipHeader.checksum, 16);

////qDebug() << QString::number(ipHeader.src, 16);
////qDebug() << QString::number(ipHeader.dest, 16);
////qDebug() << QString::number(ipHeader.srcPort, 16);
////qDebug() << QString::number(ipHeader.PORTS.destPort, 16);
//qDebug() << "----------------------------------------";


//quint32_be temp = ipHeader.HEADER.dest;
//ipHeader.HEADER.dest = ipHeader.HEADER.src;
//ipHeader.HEADER.src = temp;
//ipHeader.PORTS.srcPort = ipHeader.PORTS.destPort;
//ipHeader.PORTS.destPort = 2223;
//ipHeader.HEADER.checksum = 0x0000;
//ipHeader.HEADER.identification = 0;

////checksum
//char buf[sizeof(ipHeader.HEADER)];
//memcpy(buf, &ipHeader.HEADER, sizeof(ipHeader.HEADER));
//QByteArray newHeaderArray = QByteArray::fromRawData(buf, sizeof(ipHeader.HEADER));
//// Calculate the length from the IHL field
//unsigned short checksum = calculateChecksum(reinterpret_cast<const unsigned short*>(newHeaderArray.constData()), sizeof(ipHeader.HEADER));
//// Update the checksum in the QByteArray
//newHeaderArray[10] = qFromBigEndian(checksum) >> 8;  // Byte 10 is the high byte of the checksum
//newHeaderArray[11] = qFromBigEndian(checksum) & 0xFF;  // Byte 11 is the low byte of the checksum

//char buf2[sizeof(ipHeader.PORTS)];
//memcpy(buf2, &ipHeader.PORTS, sizeof(ipHeader.PORTS));
//QByteArray newPortsArray = QByteArray::fromRawData(buf2, sizeof(ipHeader.PORTS));

//QByteArray outcome = arr.right(arr.length() - 24);


//newHeaderArray.append(newPortsArray);
//newHeaderArray.append(outcome);

//newHeaderArray[26] = 0x00;   // у UDP пакета можно не вычислять контрольную сумму
//newHeaderArray[27] = 0x00;


//qDebug() << newHeaderArray.toHex(':');
//emit writeData(newHeaderArray);

