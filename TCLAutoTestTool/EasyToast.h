#ifndef EASYTOAST_H
#define EASYTOAST_H

#include <QDialog>
#include <QTimer>

namespace Ui {
class DialogToast;
}

#include <QPixmap>

class EasyToast : public QDialog
{
    Q_OBJECT

public:
    explicit EasyToast(QWidget *parent = nullptr);
    ~EasyToast();

    void active(const QString&text,int type=0,int durtion=1200) ;

    static void information(const QString&text,int durtion=1200) ;
    static void warning(const QString&text,int durtion=1200) ;
    static void critical(const QString&text,int durtion=1200) ;
    static void question(const QString&text,int durtion=1200) ;

private:
    Ui::DialogToast *ui;

    QPixmap pmInformation;
    QPixmap pmQuestion;
    QPixmap pmWarning;
    QPixmap pmCritical;

    qreal m_opacity = 1.0 ;
    QTimer m_TMShow ;
    QTimer m_TMHide ;
};

EasyToast *toast() ;
void easyToast(const QString&text,int type=0,quint32 durtaion=1200) ;

#endif // EASYTOAST_H
