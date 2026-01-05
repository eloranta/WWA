#include "udpreceiver.h"
#include <QDataStream>
#include <QTime>
#include <QDebug>

static QString readUtf8(QDataStream &ds)
{
    QByteArray ba;
    ds >> ba;                       // WSJT-X "utf8" is serialized as QByteArray
    return QString::fromUtf8(ba);
}

static void setStreamVersionFromSchema(QDataStream &ds, quint32 schema)
{
    // From WSJT-X NetworkMessage.hpp:
    // schema 2 -> Qt_5_2, schema 3 -> Qt_5_4 (and message is big endian).
    // (If you ever see schema 3, use Qt_5_4.)
    if (schema >= 3) ds.setVersion(QDataStream::Qt_5_4);
    else             ds.setVersion(QDataStream::Qt_5_2);
}

static bool decodeType2(QDataStream &ds, const QString &id)
{
    bool isNew = false;
    QTime time;
    qint32 snr = 0;
    double deltaTime = 0.0;     // float serialized as double
    quint32 deltaFreq = 0;
    QString mode;
    QString message;
    bool lowConfidence = false;
    bool offAir = false;

    ds >> isNew
        >> time
        >> snr
        >> deltaTime
        >> deltaFreq;

    mode = readUtf8(ds);
    message = readUtf8(ds);

    // These two bools exist in newer schemas/builds.
    // Only read them if bytes remain, otherwise default false.
    if (ds.device() && (ds.device()->bytesAvailable() >= 1)) ds >> lowConfidence;
    if (ds.device() && (ds.device()->bytesAvailable() >= 1)) ds >> offAir;

    if (ds.status() != QDataStream::Ok) {
        qWarning() << "Type2 decode failed: stream status =" << ds.status();
        return false;
    }

    qDebug().noquote()
        << "DECODE(type=2)"
        << "id=" << id
        << "new=" << isNew
        << "time=" << time.toString("HH:mm:ss")
        << "snr=" << snr
        << "dt=" << deltaTime
        << "df=" << deltaFreq
        << "mode=" << mode
        << "msg=" << message
        << "lowconf=" << lowConfidence
        << "offair=" << offAir;

    return true;
}

static void debugDecodeWsjtx(const QByteArray &datagram)
{
    QDataStream ds(datagram);
    ds.setByteOrder(QDataStream::BigEndian);   // WSJT-X network messages are big endian :contentReference[oaicite:1]{index=1}

    quint32 magic = 0, schema = 0, type = 0;
    ds >> magic >> schema >> type;

    if (magic != 0xadbccbda) {
        qDebug() << "Not WSJT-X. First bytes:" << datagram.left(16).toHex(' ');
        return;
    }

    setStreamVersionFromSchema(ds, schema);

    // After header: Id (unique key) is "utf8" (QByteArray), NOT QString :contentReference[oaicite:2]{index=2}
    const QString id = readUtf8(ds);

    qDebug().noquote() << "WSJT-X schema=" << schema << "type=" << type << "id=" << id;

    if (type == 2) {
        decodeType2(ds, id);
    } else {
        qDebug() << "Unhandled type" << type << "remaining bytes"
                 << (ds.device() ? ds.device()->bytesAvailable() : -1);
    }
}

UdpReceiver::UdpReceiver(QObject *parent)
    : QObject(parent)
{
    connect(&m_socket, &QUdpSocket::readyRead, this, &UdpReceiver::onReadyRead);
}

bool UdpReceiver::start(quint16 port)
{
    // Bind only to localhost (127.0.0.1). If you want to accept from any host,
    // use QHostAddress::AnyIPv4 instead.
    const bool ok = m_socket.bind(
        QHostAddress::LocalHost,
        port,
        QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint
        );

    if (!ok) {
        qWarning() << "UDP bind failed on 127.0.0.1:" << port << "-" << m_socket.errorString();
        return false;
    }

    qDebug() << "UDP listening on 127.0.0.1:" << port;
    return true;
}

void UdpReceiver::onReadyRead()
{
    while (m_socket.hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(int(m_socket.pendingDatagramSize()));

        QHostAddress sender;
        quint16 senderPort = 0;

        const qint64 n = m_socket.readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        if (n < 0) {
            qWarning() << "readDatagram failed:" << m_socket.errorString();
            continue;
        }

        // Try to print as UTF-8 text; also show raw bytes length.
        const QString text = QString::fromUtf8(datagram);

        // qDebug().noquote()
        //     << "UDP from" << sender.toString() << ":" << senderPort
        //     << "len=" << datagram.size()
        //     << "msg=" << text.trimmed();

        debugDecodeWsjtx(datagram);
    }
}

