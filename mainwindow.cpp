#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QFile>
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QMouseEvent>

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

        return QStyledItemDelegate::editorEvent(event, model,option,index);
    }
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    openDatabase();

    QSqlTableModel *model = new QSqlTableModel(ui->tableView);
    model->setTable("modes");
    model->setEditStrategy(QSqlTableModel::OnFieldChange);
    model->select();

    ui->tableView->setModel(model);
    ui->tableView->setColumnHidden(0, true);
    ui->tableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->tableView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // ✅ Use the custom delegate on the 'bands' column
    CheckboxDelegate *delegate = new CheckboxDelegate(ui->tableView);
    for (int i = 2; i < 10; i++)
    {
        ui->tableView->setItemDelegateForColumn(i, delegate);
        ui->tableView->setColumnWidth(i, 120);
    }

    connect(ui->clearButton, &QPushButton::clicked, this, [this, model]() {
        const QMessageBox::StandardButton choice =
            QMessageBox::warning(
                this,
                "Clear all values",
                "Clear all band values?",
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No
            );
        if (choice != QMessageBox::Yes) {
            return;
        }
        QSqlQuery query;
        const char *clearSql =
            "UPDATE modes SET "
            "\"10\"=0, "
            "\"12\"=0, "
            "\"15\"=0, "
            "\"17\"=0, "
            "\"20\"=0, "
            "\"30\"=0, "
            "\"40\"=0, "
            "\"80\"=0";
        if (!query.exec(clearSql)) {
            statusBar()->showMessage("Clear failed: " + query.lastError().text());
            return;
        }
        model->select();
        statusBar()->showMessage("All values cleared");
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::openDatabase()
{
    const QString dbPath =
        QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../WWA.db");
    if (!QFile::exists(dbPath)) {
        QFile file(dbPath);
        if (!file.open(QIODevice::WriteOnly)) {
            statusBar()->showMessage("Failed to create WWA.db");
            return false;
        }
        file.close();
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);
    if (!db.open()) {
        statusBar()->showMessage("DB open failed: " + db.lastError().text());
        return false;
    }

    QSqlQuery query(db);
    const char *createSql =
        "CREATE TABLE IF NOT EXISTS modes ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "callsign TEXT,"
        "\"10\" INTEGER,"
        "\"12\" INTEGER,"
        "\"15\" INTEGER,"
        "\"17\" INTEGER,"
        "\"20\" INTEGER,"
        "\"30\" INTEGER,"
        "\"40\" INTEGER,"
        "\"80\" INTEGER"
        ");";
    if (!query.exec(createSql)) {
        statusBar()->showMessage("Table create failed: " + query.lastError().text());
        return false;
    }

    if (!query.exec("SELECT COUNT(*) FROM modes")) {
        statusBar()->showMessage("Count query failed: " + query.lastError().text());
        return false;
    }

    if (query.next() && query.value(0).toInt() == 0) {
        QStringList calls = {
            "3B8WWA","3Z6I","4M5A","4M5DX","4U1A","8A1A","9M2WWA","9M8WWA","A43WWA","AT2WWA","AT3WWA","AT4WWA","AT6WWA","AT7WWA","BA3RA","BA7CK","BG0DXC","BH9CA","BI4SSB","BY1RX",
            "BY2WL","BY5HB","BY6SX","BY8MA","CQ7WWA","CR2WWA","CR5WWA","CR6WWA","D4W","DA0WWA","DL0WWA","DU0WWA","E2WWA","E7W","EG1WWA","EG2WWA","EG3WWA","EG4WWA","EG5WWA","EG6WWA",
            "EG7WWA","EG9WWA","EM0WWA","GB0WWA","GB1WWA","GB2WWA","GB4WWA","GB5WWA","GB6WWA","GB8WWA","GB9WWA","HB9WWA","HI3WWA","HI6WWA","HI7WWA","HI8WWA","HZ1WWA","II0WWA","II1WWA",
            "II2WWA","II3WWA","II4WWA","II5WWA","II6WWA","II7WWA","II8WWA","II9WWA","IR0WWA","IR1WWA","LR1WWA","N0W","N1W","N4W","N6W","N8W","N9W","OL6WWA","OP0WWA","PA26WWA","PC26WWA",
            "PD26WWA","PE26WWA","PF26WWA","RW1F","S53WWA","SB9WWA","SC9WWA","SD9WWA","SN0WWA","SN1WWA","SN2WWA","SN3WWA","SN4WWA","SN6WWA","SO3WWA","SX0W","TK4TH","TM18WWA","TM1WWA",
            "TM29WWA","TM7WWA","TM9WWA","UP7WWA","VB2WWA","VC1WWA","VE9WWA","W4I","YI1RN","YL73R","YO0WWA","YU45MJA","Z30WWA"
        };
        for (const QString &call : calls) {
            query.prepare(
                "INSERT INTO modes (callsign, \"10\", \"12\", \"15\", \"17\", \"20\", \"30\", \"40\", \"80\") "
                "VALUES (?, 0, 0, 0, 0, 0, 0, 0, 0)"
            );
            query.addBindValue(call);
            if (!query.exec()) {
                qWarning() << "Insert failed for call:" << call;
            }
        }

        qDebug() << "Inserted" << calls.size() << "calls";
    } else {
        qDebug() << "Data already exists, skipping insert.";
    }

    statusBar()->showMessage("Opened WWA.db");
    return true;
}
