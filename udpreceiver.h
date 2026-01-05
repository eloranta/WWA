#ifndef UDPRECEIVER_H
#define UDPRECEIVER_H

#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>

class UdpReceiver : public QObject
{
    Q_OBJECT
public:
    explicit UdpReceiver(QObject *parent = nullptr);

    // Start listening on localhost:2237
    bool start(quint16 port = 2237);
signals:
    void qsoLogged(const QString &call, const QString &band, const QString &mode);
private slots:
    void onReadyRead();

private:
    QUdpSocket m_socket;
};

#endif // UDPRECEIVER_H
