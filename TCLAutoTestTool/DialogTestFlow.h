#ifndef DIALOGTESTFLOW_H
#define DIALOGTESTFLOW_H

#include <QTimer>
#include <QSettings>
#include <QPainter>
#include <QDialog>
#include <QStandardItem>
#include <QHeaderView>
#include <QStandardItemModel>

namespace Ui {
class DialogTestFlow;
}


class ZeroBasedHeader : public QHeaderView {
    Q_OBJECT
public:
    explicit ZeroBasedHeader(QWidget *parent = nullptr)
        : QHeaderView(Qt::Vertical, parent) {}

protected:
    void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const override {
        painter->save();
        painter->fillRect(rect,painter->background());
        painter->setFont(parentWidget()->font());
        painter->drawText(rect.adjusted(0,0,-5,0),QString::number(logicalIndex),QTextOption(Qt::AlignVCenter|Qt::AlignRight));

        QPalette palette = this->palette();
        painter->setPen(palette.color(QPalette::Mid));
        painter->drawLine(rect.bottomLeft(),rect.bottomRight());
        painter->drawLine(rect.topRight(),rect.bottomRight());

        //QHeaderView::paintSection(painter, rect, logicalIndex);

        painter->restore();
    }
};

class DialogTestFlow : public QDialog
{
    Q_OBJECT

public:
    explicit DialogTestFlow(QWidget *parent = nullptr);
    ~DialogTestFlow();
    void addTestItem(const QString&text,const QString&time,bool option);
    void startTest();
    void toTheEnd() ;
    void toCancel() ;

    QSettings *m_pSet = nullptr;
signals:
    void onTestIndex(int item);
    void onTestToEnd();
private:
    Ui::DialogTestFlow *ui;
    QStandardItemModel *m_model = nullptr;
    int m_nTestIndex = -1;
    QTimer m_TMTest;

    int m_TCount = 0;
    QTimer m_TMCount;

};



void addTestItem(const QString&text,bool option=false) ;

#endif // DIALOGTESTFLOW_H
