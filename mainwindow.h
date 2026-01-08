#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "udpreceiver.h"
#include <QMainWindow>
#include <QLabel>
#include <QSqlTableModel>
#include <QTcpSocket>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
public slots:
    void onQsoLogged(const QString &call, const QString &band, const QString &mode);
    void onAddClicked();
    void onClearClicked();
private:
    Ui::MainWindow *ui;
    UdpReceiver *udp = nullptr;
    void updateStatusCounts();

    QLabel *statusCountsLabel = nullptr;
    QSqlTableModel *m_model = nullptr;
    QTcpSocket *rbnSocket = nullptr;
    QByteArray rbnBuffer;
    bool rbnLoginSent = false;
};
#endif // MAINWINDOW_H
