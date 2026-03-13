#include "EasyToast.h"
#include "qstyle.h"
#include "ui_EasyToast.h"
#include <QScreen>
#include <QApplication>

EasyToast *toast()
{
    static EasyToast *pDlg = new EasyToast() ;
    pDlg->close();
    return pDlg ;
}

void easyToast(const QString&text,int type,quint32 durtaion)
{
    toast()->active(text,type,durtaion) ;
}

EasyToast::EasyToast(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogToast)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::Popup | Qt::WindowStaysOnTopHint|Qt::Tool | Qt::Dialog);    // 必须设置无边框
    setAttribute(Qt::WA_TranslucentBackground); // 设置背景透明
    setStyleSheet("QFrame#frame{background-color: rgba(50, 50, 50, 150); border-radius: 10px; color:white;}");

    pmInformation=QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation).pixmap(40,40);
    pmQuestion=QApplication::style()->standardIcon(QStyle::SP_MessageBoxQuestion).pixmap(40,40);
    pmWarning=QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(40,40);
    pmCritical=QApplication::style()->standardIcon(QStyle::SP_MessageBoxCritical).pixmap(40,40);
    ui->labelIcon->setPixmap(pmInformation) ;

    connect(&m_TMShow,&QTimer::timeout,this,[=]{
        m_TMHide.stop() ;
        m_TMHide.start(30) ;
    });

    connect(&m_TMHide,&QTimer::timeout,this,[=]{
        m_opacity -= 0.1 ;
        //setWindowOpacity(m_opacity) ;
        if(m_opacity <= 0.0)
        {
            m_TMHide.stop() ;
            this->hide() ;
        }
    });
}

EasyToast::~EasyToast()
{
    delete ui;
}

void EasyToast::information(const QString&text,int durtion)
{
    easyToast(text,0,durtion);
}
void EasyToast::warning(const QString&text,int durtion)
{
    easyToast(text,1,durtion);
}
void EasyToast::critical(const QString&text,int durtion)
{
    easyToast(text,2,durtion);
}
void EasyToast::question(const QString&text,int durtion)
{
    easyToast(text,3,durtion);
}

void EasyToast::active(const QString&text, int type, int durtion)
{
    m_TMShow.stop() ;
    m_TMHide.stop() ;

    switch(type)
    {
    case 0: ui->labelIcon->setPixmap(pmInformation) ; break ;
    case 1: ui->labelIcon->setPixmap(pmQuestion) ; break ;
    case 2: ui->labelIcon->setPixmap(pmWarning) ; break ;
    case 3: ui->labelIcon->setPixmap(pmCritical) ; break ;
    }
    ui->label->setText(text) ;
    setWindowOpacity(1.0) ;
    m_opacity = 1.0 ;

    m_TMShow.start(durtion) ;
    raise() ;

    exec() ;
}
