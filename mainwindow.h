#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "udpreceiver.h"
#include <QMainWindow>
#include <QLabel>
#include <QSqlTableModel>
#include <QTcpSocket>
#include <array>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class CheckboxDelegate;

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
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
private:
    Ui::MainWindow *ui;
    UdpReceiver *udp = nullptr;
    void updateStatusCounts();
    void updateModeVisibility();

    QLabel *statusInfoLabel = nullptr;
    QLabel *statusCountsLabel = nullptr;
    QSqlTableModel *m_model = nullptr;
    class CheckboxDelegate *checkboxDelegate = nullptr;
    std::array<bool, 4> modeVisible{{true, true, true, true}};
    QTcpSocket *rbnSocket = nullptr;
    QByteArray rbnBuffer;
    bool rbnLoginSent = false;
    bool rbnOutputPaused = false;
};
#endif // MAINWINDOW_H
