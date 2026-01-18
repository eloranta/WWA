#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "udpreceiver.h"

#include <QApplication>
#include <QTableView>
#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QSqlQuery>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>
#include <QSqlError>
#include <QVariant>
#include <QMessageBox>
#include <QSqlRecord>
#include <QAbstractSocket>
#include <QRegularExpression>
#include <QEvent>

// ✅ Custom delegate
class CheckboxDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override {
        int bits = index.data(Qt::DisplayRole).toInt();
        QStringList symbols = {"CW", "PH", "FT8", "FT4"};

        int boxWidth = option.rect.width() / 4;

        for (int i = 0; i < 4; ++i) {
            QRect boxRect(
                option.rect.left() + i * boxWidth,
                option.rect.top(),
                boxWidth,
                option.rect.height()
                );

            boxRect.adjust(2, 2, -2, -2);

            bool checked = bits & (1 << i);

            painter->save();
            painter->setPen(Qt::black);
            painter->setBrush(Qt::NoBrush);
            painter->drawRoundedRect(boxRect, 4, 4);
            painter->setPen(checked ? Qt::red : Qt::gray);
            painter->drawText(boxRect, Qt::AlignCenter, symbols[i]);
            painter->restore();
        }
    }

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override {
        Q_UNUSED(parent);
        Q_UNUSED(option);
        Q_UNUSED(index);
        return nullptr;
    }

    bool editorEvent(QEvent *event, QAbstractItemModel *model,
                     const QStyleOptionViewItem &option,
                     const QModelIndex &index) override {
        if (event->type() == QEvent::MouseButtonRelease) {
            auto *mouseEvent = static_cast<QMouseEvent*>(event);
            int boxWidth = option.rect.width() / 4;
            if (boxWidth <= 0) {
                return false;
            }

            int clickedIndex = (mouseEvent->pos().x() - option.rect.x()) / boxWidth;
            if (clickedIndex < 0 || clickedIndex >= 4) {
                return false;
            }

            int bits = index.data(Qt::DisplayRole).toInt();
            bits ^= (1 << clickedIndex); // toggle bit

            model->setData(index, bits, Qt::EditRole);
            QSqlTableModel *sqlModel = qobject_cast<QSqlTableModel*>(model);
            if (sqlModel) {
                sqlModel->submitAll();  // Save to DB immediately
            }
            return true;
        }
        return false;
    }
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_model = new QSqlTableModel(ui->tableView);
    m_model->setTable("modes");
    m_model->setEditStrategy(QSqlTableModel::OnFieldChange);
    const int callCol = m_model->fieldIndex("callsign");
    if (callCol >= 0) {
        m_model->setSort(callCol, Qt::AscendingOrder);
    }
    m_model->select();

    ui->tableView->setModel(m_model);
    // Disable all selection/highlight
    ui->tableView->setSelectionMode(QAbstractItemView::NoSelection);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectItems); // optional

    ui->tableView->setColumnHidden(0, true);

    CheckboxDelegate *delegate = new CheckboxDelegate(ui->tableView);
    // ✅ Use the custom delegate on the 'bands' column
    for (int i = 2; i < 10; i++)
    {
        ui->tableView->setItemDelegateForColumn(i, delegate);
        ui->tableView->setColumnWidth(i, 120);
    }

    connect(m_model, &QAbstractItemModel::dataChanged,
            this, [this](const QModelIndex &, const QModelIndex &, const QVector<int> &) {
                updateStatusCounts();
            });

    connect(ui->addButton, &QPushButton::clicked,
            this, &MainWindow::onAddClicked);
    connect(ui->clearButton, &QPushButton::clicked,
            this, &MainWindow::onClearClicked);

    udp = new UdpReceiver(this);

    connect(udp, &UdpReceiver::qsoLogged,
            this, &MainWindow::onQsoLogged);

    udp->start(2333);

    statusCountsLabel = new QLabel(this);
    statusCountsLabel->setMinimumWidth(260);
    statusCountsLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    statusInfoLabel = new QLabel(this);
    statusInfoLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->statusbar->addWidget(statusInfoLabel, 1);
    ui->statusbar->addPermanentWidget(statusCountsLabel);
    statusInfoLabel->setText("Ready");
    updateStatusCounts();
    ui->statusbar->installEventFilter(this);

    rbnSocket = new QTcpSocket(this);
    connect(rbnSocket, &QTcpSocket::readyRead, this, [this]() {
        const QByteArray data = rbnSocket->readAll();
        if (data.isEmpty()) {
            return;
        }

        rbnBuffer.append(data);
        // qDebug().noquote() << "RBN:" << data;

        static const QRegularExpression rbnLineRegex(
            R"(^DX de\s+\S+:\s+([0-9.]+)\s+([A-Za-z0-9/]+)\b(?:\s+([A-Za-z0-9/]+))?)"
        );
        auto freqToBand = [](double value) -> QString {
            // RBN spots often use kHz (e.g. 14074.0); normalize to MHz.
            double mhz = value;
            if (mhz > 1000.0) {
                mhz /= 1000.0;
            }

            if (mhz >= 3.5 && mhz < 4.0) return "80";
            if (mhz >= 7.0 && mhz < 7.3) return "40";
            if (mhz >= 10.1 && mhz < 10.15) return "30";
            if (mhz >= 14.0 && mhz < 14.35) return "20";
            if (mhz >= 18.068 && mhz < 18.168) return "17";
            if (mhz >= 21.0 && mhz < 21.45) return "15";
            if (mhz >= 24.89 && mhz < 24.99) return "12";
            if (mhz >= 28.0 && mhz < 29.7) return "10";
            return QString();
        };

        if (rbnOutputPaused) {
            if (!rbnLoginSent && rbnBuffer.contains("Please enter your call:")) {
                rbnSocket->write("OG3Z\r\n");
                rbnLoginSent = true;
                qDebug() << "RBN login sent";
            }
            rbnBuffer.clear();
            return;
        }

        while (true) {
            const int newlineIndex = rbnBuffer.indexOf('\n');
            if (newlineIndex < 0) {
                break;
            }

            const QByteArray lineBytes = rbnBuffer.left(newlineIndex);
            rbnBuffer.remove(0, newlineIndex + 1);

            const QString line = QString::fromUtf8(lineBytes).trimmed();
            if (line.isEmpty()) {
                continue;
            }

            qDebug().noquote() << line;

            const QRegularExpressionMatch match = rbnLineRegex.match(line);
            if (match.hasMatch()) {
                const QString freq = match.captured(1);
                const QString call = match.captured(2);
                const QString callUp = call.trimmed().toUpper();
                const QString mode = match.captured(3).trimmed().toUpper();
                const double freqValue = freq.toDouble();
                const QString band = freqToBand(freqValue);
                // qDebug().noquote() << "RBN spot:" << "call=" << call << "freq=" << freq;

                if (band.isEmpty()) {
                    return;
                }

                QSqlQuery q;
                const QString sql = QString(R"(SELECT "%1" FROM modes WHERE callsign = ? LIMIT 1)").arg(band);
                q.prepare(sql);
                q.addBindValue(callUp);
                if (!q.exec()) {
                    qWarning() << "RBN DB lookup failed:" << q.lastError();
                } else if (q.next()) {
                    const int mask = q.value(0).toInt();
                    //qDebug().noquote() << "RBN in DB:" << callUp;
                    if (!(mask & (1 << 0))) {
                        //qDebug().noquote() << callUp << mode << freq;
                        if (statusInfoLabel && mode == "CW") {
                            statusInfoLabel->setText(QString("%1 %2").arg(callUp, freq));
                        } else {
                            qDebug() << "RBN non-CW:" << callUp << freq << "mode" << (mode.isEmpty() ? "<none>" : mode);
                        }
                    }
                }
            }
        }

        if (!rbnLoginSent && rbnBuffer.contains("Please enter your call:")) {
            rbnSocket->write("OG3Z\r\n");
            rbnLoginSent = true;
            qDebug() << "RBN login sent";
        }
    });
    connect(rbnSocket,
            QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, [this](QAbstractSocket::SocketError) {
                qWarning() << "RBN socket error:" << rbnSocket->errorString();
            });
    connect(rbnSocket, &QTcpSocket::connected, this, [this]() {
        qDebug() << "RBN connected";
    });
    connect(rbnSocket, &QTcpSocket::disconnected, this, [this]() {
        qWarning() << "RBN disconnected";
    });
    rbnSocket->connectToHost("telnet.reversebeacon.net", 7000);
 }

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->statusbar && event->type() == QEvent::MouseButtonPress) {
        rbnOutputPaused = !rbnOutputPaused;
        if (statusInfoLabel) {
            statusInfoLabel->setStyleSheet(rbnOutputPaused ? "color: red;" : "");
        }
        return true;
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::onQsoLogged(const QString &call, const QString &band, const QString &mode)
{
    const QString callUp = call.trimmed().toUpper();
    const QString bandCol = band.trimmed();          // expects: "10","12","15","17","20","30","40","80"
    const QString modeUp = mode.trimmed().toUpper(); // "FT8" or "FT4"

    qDebug().noquote() << "MainWindow slot: QSO logged -> call=" << callUp
                       << "band=" << bandCol
                       << "mode=" << modeUp;

    // Map mode to your bitmask (CW=0, PH=1, FT8=2, FT4=3)
    int bit = -1;
    if (modeUp == "FT8") bit = 2;
    else if (modeUp == "FT4") bit = 3;
    else {
        qDebug() << "Ignoring mode (not FT8/FT4):" << modeUp;
        return;
    }

    // IMPORTANT: band is used as a column name, so we must validate it.
    static const QSet<QString> allowedBands = {"10","12","15","17","20","30","40","80"};
    if (!allowedBands.contains(bandCol)) {
        qWarning() << "Ignoring unknown band column:" << bandCol;
        return;
    }

    QSqlQuery q;

    // 1) Check if call exists and read current mask value in that band column
    const QString selectSql = QString(R"(
        SELECT id, "%1" FROM modes WHERE callsign = ?
    )").arg(bandCol);

    q.prepare(selectSql);
    q.addBindValue(callUp);

    if (!q.exec()) {
        qWarning() << "Select failed:" << q.lastError();
        return;
    }

    if (!q.next()) {
        qDebug() << "Call not found in DB, ignoring:" << callUp;
        return; // "if call exists in db" -> only update when it exists
    }

    const int id = q.value(0).toInt();
    const int currentMask = q.value(1).toInt();
    const int newMask = currentMask | (1 << bit);

    if (newMask == currentMask) {
        qDebug() << "Already set, no DB update needed for" << callUp << "band" << bandCol << "mode" << modeUp;
        return;
    }

    // 2) Update the band column
    QSqlQuery u;
    const QString updateSql = QString(R"(
        UPDATE modes SET "%1" = ? WHERE id = ?
    )").arg(bandCol);

    u.prepare(updateSql);
    u.addBindValue(newMask);
    u.addBindValue(id);

    if (!u.exec()) {
        qWarning() << "Update failed:" << u.lastError();
        return;
    }

    qDebug().noquote() << "DB updated:" << callUp
                       << "band" << bandCol
                       << "mask" << currentMask << "->" << newMask;

    auto *m = qobject_cast<QSqlTableModel*>(ui->tableView->model()); // TODO:
    if (m) m->select();

    updateStatusCounts();

    if (statusInfoLabel) {
        statusInfoLabel->setText(QString("Logged %1 on %2m %3").arg(call, band, mode));
    }
}

void MainWindow::onAddClicked()
{
    if (!m_model) {
        return;
    }

    QSqlRecord rec = m_model->record();
    rec.setValue("callsign", "");
    rec.setValue("10", 0);
    rec.setValue("12", 0);
    rec.setValue("15", 0);
    rec.setValue("17", 0);
    rec.setValue("20", 0);
    rec.setValue("30", 0);
    rec.setValue("40", 0);
    rec.setValue("80", 0);

    if (!m_model->insertRecord(-1, rec)) {
        qWarning() << "Insert failed:" << m_model->lastError();
        if (statusInfoLabel) {
            statusInfoLabel->setText("Add failed");
        }
        return;
    }

    if (!m_model->submitAll()) {
        qWarning() << "Submit failed:" << m_model->lastError();
        if (statusInfoLabel) {
            statusInfoLabel->setText("Add failed");
        }
        m_model->revertAll();
        return;
    }

    m_model->select();
    updateStatusCounts();
    if (statusInfoLabel) {
        statusInfoLabel->setText("Added empty record");
    }
}

void MainWindow::onClearClicked()
{
    const auto response = QMessageBox::question(
        this,
        "Confirm Clear",
        "Set every band value to 0 for all callsigns?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
        );

    if (response != QMessageBox::Yes) {
        return;
    }

    QSqlQuery q;
    const QString sql = R"(
        UPDATE modes SET
            "10" = 0,
            "12" = 0,
            "15" = 0,
            "17" = 0,
            "20" = 0,
            "30" = 0,
            "40" = 0,
            "80" = 0
    )";

    if (!q.exec(sql)) {
        qWarning() << "Clear failed:" << q.lastError();
        if (statusInfoLabel) {
            statusInfoLabel->setText("Clear failed");
        }
        return;
    }

    if (m_model) {
        m_model->select();
    }
    updateStatusCounts();
    if (statusInfoLabel) {
        statusInfoLabel->setText("Cleared all mode values");
    }
}

void MainWindow::updateStatusCounts()
{
    int cw = 0, ph = 0, ft8 = 0, ft4 = 0;

    static const QStringList bands = {
        "10","12","15","17","20","30","40","80"
    };

    QSqlQuery q;

    for (const QString &band : bands) {
        const QString sql = QString(R"(SELECT "%1" FROM modes)").arg(band);
        if (!q.exec(sql)) {
            qWarning() << "Count query failed:" << q.lastError();
            return;
        }

        while (q.next()) {
            const int mask = q.value(0).toInt();
            if (mask & (1 << 0)) cw++;
            if (mask & (1 << 1)) ph++;
            if (mask & (1 << 2)) ft8++;
            if (mask & (1 << 3)) ft4++;
        }
    }

    const int total = cw * 10 + ph * 5 + ft8 * 2 + ft4 * 2;

    statusCountsLabel->setText(
        QString("CW:%1  PH:%2  FT8:%3  FT4:%4  TOTAL:%5")
            .arg(cw)
            .arg(ph)
            .arg(ft8)
            .arg(ft4)
            .arg(total)
        );
}
