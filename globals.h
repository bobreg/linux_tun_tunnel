#ifndef GLOBALS_H
#define GLOBALS_H

#include <QObject>
#include <QDir>
#include <QSettings>
#include <QtEndian>
#include <QSemaphore>


// globals variables
extern QSettings * g_settings;

#pragma pack(push, 1)
struct Ip_header {
    struct header
    {
        quint8 version_header_length;
        quint8 service_type;
        quint16_be total_length;

        quint16_be identification;
        quint16_be flags_fragment_offset;

        quint8 ttl;
        quint8 protocol;
        quint16_be checksum;

        quint32_be src;
        quint32_be dest;
    }HEADER;
    struct ports
    {
        quint16_be srcPort;
        quint16_be destPort;
    }PORTS;
};
#pragma pack(pop)

#endif // GLOBALS_H
