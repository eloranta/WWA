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
    ui->statusbar->addPermanentWidget(statusCountsLabel);
    ui->statusbar->showMessage("Ready");
    updateStatusCounts();

    rbnSocket = new QTcpSocket(this);
    connect(rbnSocket, &QTcpSocket::readyRead, this, [this]() {
        const QByteArray data = rbnSocket->readAll();
        if (data.isEmpty()) {
            return;
        }

        rbnBuffer.append(data);
        qDebug().noquote() << "RBN:" << data;

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

    ui->statusbar->showMessage(
        QString("Logged %1 on %2m %3").arg(call, band, mode),
        3000
        );
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
        ui->statusbar->showMessage("Add failed", 3000);
        return;
    }

    if (!m_model->submitAll()) {
        qWarning() << "Submit failed:" << m_model->lastError();
        ui->statusbar->showMessage("Add failed", 3000);
        m_model->revertAll();
        return;
    }

    m_model->select();
    updateStatusCounts();
    ui->statusbar->showMessage("Added empty record", 3000);
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
        ui->statusbar->showMessage("Clear failed", 3000);
        return;
    }

    if (m_model) {
        m_model->select();
    }
    updateStatusCounts();
    ui->statusbar->showMessage("Cleared all mode values", 3000);
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
