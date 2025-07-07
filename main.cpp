#include "mainwindow.h"

#include <QApplication>
#include <QTableView>
#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QSqlQuery>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>

void setupDatabase() {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("modes.db");

    if (!db.open()) {
        qFatal("Cannot open database!");
    }

    QSqlQuery query;

    // ✅ Create the table
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS modes (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            callsign TEXT,
            "10" INTEGER,
            "12" INTEGER,
            "15" INTEGER,
            "17" INTEGER,
            "20" INTEGER,
            "30" INTEGER,
            "40" INTEGER,
            "80" INTEGER
        )
    )");

    // ✅ Only insert if table is empty
    query.exec("SELECT COUNT(*) FROM modes");
    if (query.next() && query.value(0).toInt() == 0) {
        QStringList calls = {
            "3B8WWA","3Z6I","8A1K","9M4WWA","A43WWA","A91WWA","AT4WWA","BA7CK","BY1RX","CR2WWA",
            "CS2WWA","DA0WWA","DL0WWA","DU0WWA","E7W","EG1WWA","EG2WWA","EG4WWA","EG5WWA","EG6WWA",
            "EG7WWA","EH3WWA","EN0U","GB2WWA","GB4WWA","GB8WWA","GB9WWA","HB9WWA","HI3WWA","HI6WWA",
            "HI7WWA","HI8WWA","II0WWA","II1WWA","II2WWA","II3WWA","II4WWA","II5WWA","II6WWA","II7WWA",
            "II8WWA","II9WWA","IR0WWA","IR1WWA","J8WWA","LR1WWA","N0W","N1W","N3W","N4W","N6W","N9W",
            "OL5WWA","OR0WWA","RW1F","S53WWA","SN0WWA","SN2WWA","SN3WWA","SN4WWA","SX0W","TK4TH",
            "TM0WWA","TM2WWA","TM73WWA","TM7WWA","TO0WWA","UP7WWA","V55WWA","VE9WWA","W4I","YI0WWA"
        };

        for (const QString &call : calls) {
            query.prepare(R"(
                INSERT INTO modes (callsign, "10", "12", "15", "17", "20", "30", "40", "80")
                VALUES (?, 0, 0, 0, 0, 0, 0, 0, 0)
            )");
            query.addBindValue(call);
            if (!query.exec()) {
                qWarning() << "Insert failed for call:" << call; // << query.lastError();
            }
        }

        qDebug() << "Inserted" << calls.size() << "calls";
    } else {
        qDebug() << "Data already exists, skipping insert.";
    }
}



int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
