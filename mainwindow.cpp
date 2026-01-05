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

    QSqlTableModel *model = new QSqlTableModel(ui->tableView);
    model->setTable("modes");
    model->setEditStrategy(QSqlTableModel::OnFieldChange);
    model->select();

    ui->tableView->setModel(model);
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

    udp = new UdpReceiver(this);

    connect(udp, &UdpReceiver::qsoLogged,
            this, &MainWindow::onQsoLogged);

    udp->start(2333);
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
}
