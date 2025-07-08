#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QApplication>
#include <QTableView>
#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QSqlQuery>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>

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
            int clickedIndex = (mouseEvent->pos().x() - option.rect.x()) / boxWidth;

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

    QSqlTableModel *model = new QSqlTableModel;
    model->setTable("modes");
    model->setEditStrategy(QSqlTableModel::OnFieldChange);
    model->select();

    ui->tableView->setModel(model);

    // ✅ Use the custom delegate on the 'bands' column
    CheckboxDelegate *delegate = new CheckboxDelegate(ui->tableView);
    ui->tableView->setItemDelegateForColumn(2, delegate);
    ui->tableView->setItemDelegateForColumn(3, delegate);
    ui->tableView->setItemDelegateForColumn(4, delegate);
    ui->tableView->setItemDelegateForColumn(5, delegate);
    ui->tableView->setItemDelegateForColumn(6, delegate);
    ui->tableView->setItemDelegateForColumn(7, delegate);
    ui->tableView->setItemDelegateForColumn(8, delegate);
    ui->tableView->setItemDelegateForColumn(9, delegate);

    ui->tableView->setColumnHidden(0, true);

    ui->tableView->setColumnWidth(2, 120);
    ui->tableView->setColumnWidth(3, 120);
    ui->tableView->setColumnWidth(4, 120);
    ui->tableView->setColumnWidth(5, 120);
    ui->tableView->setColumnWidth(6, 120);
    ui->tableView->setColumnWidth(7, 120);
    ui->tableView->setColumnWidth(8, 120);
}

MainWindow::~MainWindow()
{
    delete ui;
}
