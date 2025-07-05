#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStyledItemDelegate>
#include <QApplication>
#include <QMouseEvent>
#include <QSqlTableModel>
#include <QPainter>

class MyModel : public QSqlTableModel
{
public:
    MyModel()
    {
        cellData = 0;
     }
    QVariant data(const QModelIndex &index, int role) const
    {
        if (role == Qt::UserRole) {
            return QVariant::fromValue(cellData);  //.at(index.row()).at(index.column()));
        }
        return QVariant();
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role)
    {
        if (role == Qt::UserRole) {
            cellData = value.toInt();
            emit dataChanged(index, index);
            qDebug() << "there";

            return true;
        }
        return false;
    }
    int cellData;
};



class CheckboxDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        // Suppose your model returns an int with 4 bits (1 = checked)
        int bits = index.data(Qt::UserRole).toInt();
        QStringList symbols = {"C", "P", "8", "4"};

        int boxWidth = option.rect.width() / 4;

        for (int i = 0; i < 4; ++i) {
            QStyleOptionButton checkBoxOption;
            checkBoxOption.rect = QRect(
                option.rect.left() + i * boxWidth,
                option.rect.top(),
                boxWidth,
                option.rect.height()
                );

            bool checked = bits & (1 << i);
            checkBoxOption.state = checked ? QStyle::State_On : QStyle::State_Off;

            // Draw the checkbox frame
            QApplication::style()->drawPrimitive(QStyle::PE_IndicatorCheckBox, &checkBoxOption, painter);

           // if (checked) {
                // Draw the letter inside the box rect
                painter->save();
                painter->setPen(Qt::black);
                painter->drawText(checkBoxOption.rect, Qt::AlignCenter, symbols[i]);
                painter->restore();
           // }
        }
    }
    bool editorEvent(QEvent *event,
                     QAbstractItemModel *model,
                     const QStyleOptionViewItem &option,
                     const QModelIndex &index) override
    {
        if (event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

            //int boxWidth = option.rect.width() / 4;
            //int clickedIndex = (mouseEvent->pos().x() - option.rect.x()) / boxWidth;

            bool bools = index.data(Qt::UserRole).value<bool>();
            bools = !bools;
            model->setData(index, QVariant::fromValue(bools), Qt::UserRole);
            qDebug() << "here" << bools;
         }

        return true;
    }
};




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

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
