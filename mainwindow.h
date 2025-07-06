#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStyledItemDelegate>
#include <QApplication>
#include <QMouseEvent>
#include <QSqlTableModel>
#include <QPainter>
#include <QStandardItemModel>


class MyModel : public QStandardItemModel
{
public:
    MyModel()
    {
    }
    // QVariant data(const QModelIndex &index, int role) const
    // {
    //     if (role == Qt::UserRole) {
    //         return QVariant::fromValue(cellData);  //.at(index.row()).at(index.column()));
    //     }
    //     return QVariant();
    // }

     bool setData(const QModelIndex &index, const QVariant &value, int role)
     {
         if (role == Qt::UserRole) {
             QStandardItemModel::setData(index, value, role);

             emit dataChanged(index, index, {role});  // <- 🔑 this tells the view to repaint!
             qDebug() << index << value;

             return true;
         }
         return false;
     }
};

class CheckboxDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        int bits = index.data(Qt::UserRole).toInt();
        qDebug() << index;
        QStringList symbols = {"CW", "PH", "FT8", "FT4"};

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

            QRect boxRect(
                option.rect.left() + i * boxWidth,
                option.rect.top(),
                boxWidth,
                option.rect.height()
                );

            boxRect.adjust(1, 1, -1, -1);

            // Draw custom letter if checked
            if (checked) {
                painter->setPen(Qt::red);
                painter->save();
                painter->drawRoundedRect(boxRect, 4, 4);
                painter->drawText(checkBoxOption.rect, Qt::AlignCenter, symbols[i]);
                painter->restore();
            } else {
                painter->save();
                painter->setPen(Qt::gray);
                painter->drawRoundedRect(boxRect, 4, 4);
                painter->drawText(checkBoxOption.rect, Qt::AlignCenter, symbols[i]);
                painter->restore();
            }
        }
    }
    bool editorEvent(QEvent *event,
                     QAbstractItemModel *model,
                     const QStyleOptionViewItem &option,
                     const QModelIndex &index) override
    {
         if (event->type() == QEvent::MouseButtonRelease) {
            auto *mouseEvent = static_cast<QMouseEvent*>(event);

            int boxWidth = option.rect.width() / 4;
            int clickedIndex = (mouseEvent->pos().x() - option.rect.x()) / boxWidth;

            int bits = index.data(Qt::UserRole).toInt();
            bits ^= (1 << clickedIndex); // Toggle the clicked bit

            model->setData(index, bits, Qt::UserRole);
            return true;
        }

        return false;
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
