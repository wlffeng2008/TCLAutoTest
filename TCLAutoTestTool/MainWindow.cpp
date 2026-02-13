#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "DialogSerialportList.h"

#include <windows.h>
#include <QTcpSocket>
#include <QDateTime>
#include <QStandardPaths>
#include <QTimer>

#define NO_MSXML_XMLDOCUMENT
#include <windows.h>
#include "tinyxml2.h"
using namespace tinyxml2;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle(QString("LDM自动化调试 -- By QT") + QT_VERSION_STR);
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint | Qt::MSWindowsFixedSizeDialogHint);

    m_setting = new QSettings(QCoreApplication::applicationDirPath() + "\\setting.ini",QSettings::IniFormat);

    ui->pushButtonCom0->setText(m_setting->value("port0","COM0").toString());
    ui->pushButtonCom1->setText(m_setting->value("port1","COM0").toString());
    ui->comboBoxBaud0->setCurrentIndex(m_setting->value("baud0",6).toInt());
    ui->comboBoxBaud1->setCurrentIndex(m_setting->value("baud1",6).toInt());

    QStringList tvSet = m_setting->value("TVSet","0,0,0,0,0,0,0,0,0,0,0,0,0").toStringList();

    connect(ui->pushButtonSetDefault,&QPushButton::clicked,this,[=]{
        ui->comboBoxTV0->setCurrentIndex(0);
        ui->comboBoxTV1->setCurrentIndex(0);
        ui->comboBoxTV2->setCurrentIndex(0);
        ui->comboBoxTV3->setCurrentIndex(0);
        ui->comboBoxTV4->setCurrentIndex(0);
        ui->comboBoxTV5->setCurrentIndex(0);
        ui->comboBoxTV6->setCurrentIndex(0);
        ui->comboBoxTV7->setCurrentIndex(0);
        ui->comboBoxTV8->setCurrentIndex(0);
    });

    //Windows 版：C:\Users\'用户名'\AppData\Local\kingst\vis.config

    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    qDebug() << "AppData 路径：" << appDataPath;

    connect(ui->checkBoxOntop,&QCheckBox::clicked,this,[=](bool checked){
        HWND hWnd = (HWND)this->winId();
        ::SetWindowPos(hWnd, checked?HWND_TOPMOST:HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    });

    // COM0
    connect(ui->pushButtonCom0,&QPushButton::clicked,this,[=]{
        DialogSerialportList comList;
        if(QDialog::Accepted == comList.exec())
        {
            ui->pushButtonCom0->setText(comList.selectedCOM());
            ui->checkBoxOpen0->setChecked(false);
            ui->checkBoxOpen0->click();
            m_setting->setValue("port0",comList.selectedCOM());
        }
    });
    connect(ui->checkBoxOpen0,&QCheckBox::clicked,this,[=](bool checked){
        if(m_COM0)
        {
            m_COM0->closePort();
            m_COM0->deleteLater();
            m_COM0 = nullptr;
        }

        if(checked)
        {
            m_COM0 = new GenComport(this);
            int index = ui->comboBoxBaud0->currentIndex();
            m_setting->setValue("baud0",index);
            m_COM0->setPortParam(ui->comboBoxBaud0->currentText().toInt());
            m_COM0->setPortName(ui->pushButtonCom0->text());
        }

        if(m_COM0 && m_COM0->isOpen())
        {
            ui->labelStatus0->setText("已连接");
            ui->labelStatus0->setStyleSheet("QLabel{color:blue;}");

            connect(m_COM0,&GenComport::onReceive,this,[=](const QByteArray &data){
                QString reply(data.data());
                qDebug() << reply;
                QTimer::singleShot(100,this,[=]{ sendLmCmd(); });
            });
        }
        else
        {
            ui->labelStatus0->setText("未打开");
            ui->labelStatus0->setStyleSheet("QLabel{color:red;}");
        }
    });

    connect(ui->pushButtonMeasure,&QPushButton::clicked,this,[=]{
        m_LmCmds.append("STR,1,23\r");
        m_LmCmds.append("STR,1,23\r");
        m_LmCmds.append("XYZ,1\r");
        sendLmCmd();
    });
    connect(ui->pushButtonZeroCall,&QPushButton::clicked,this,[=]{
        QDateTime tm = QDateTime::currentDateTime();
        QString strCmd = QString::asprintf("ZRC,1,%d,%d,%d,%d,%d,%d\r",tm.date().year(),tm.date().month(),tm.date().day(),
                                                              tm.time().hour(),tm.time().minute(),tm.time().second());
        if(m_COM0) m_COM0->send(strCmd,true);
    });

    // COM1
    connect(ui->pushButtonCom1,&QPushButton::clicked,this,[=]{
        DialogSerialportList comList;
        if(QDialog::Accepted == comList.exec())
        {
            ui->pushButtonCom1->setText(comList.selectedCOM());
            ui->checkBoxOpen1->setChecked(false);
            ui->checkBoxOpen1->click();
            m_setting->setValue("port1",comList.selectedCOM());
        }
    });

    connect(ui->checkBoxOpen1,&QCheckBox::clicked,this,[=](bool checked){
        if(m_COM1)
        {
            m_COM1->closePort();
            m_COM1->deleteLater();
            m_COM1 = nullptr;
        }

        if(checked)
        {
            m_COM1 = new GenComport(this);

            int index = ui->comboBoxBaud1->currentIndex();
            m_setting->setValue("baud1",index);
            m_COM1->setPortParam(ui->comboBoxBaud1->currentText().toInt());
            m_COM1->setPortName(ui->pushButtonCom1->text());
        }

        if(m_COM1 && m_COM1->isOpen())
        {
            ui->labelStatus1->setText("已连接");
            ui->labelStatus1->setStyleSheet("QLabel{color:blue;}");

            connect(m_COM1,&GenComport::onReceive,this,[=](const QByteArray &data){
                QString reply(data.data());
                //qDebug() << data.toHex(' ').toUpper();
            });
        }
        else
        {
            ui->labelStatus1->setText("未打开");
            ui->labelStatus1->setStyleSheet("QLabel{color:red;}");
        }
    });

    QStringList TVCmds = {
        "AA 06 10 01 A7 EF",
        "AA 06 11 03 B4 9C",
        "AA 06 11 02 B4 9C",
        "AA 07 9F 07 01 F1 55",
        "AA 06 30 03 A1 09",
        "AA 07 9F 3E 01 4E 58",
        "AA 07 9F 0A 01 87 09",
        "AA 06 25 01 5D 8F",
        "AA 06 31 01 92 38",
        "AA 09 93 01 02 03 04 4A 36",
        "AA 06 27 01 3B ED",
        "AA 08 28 FF FF FF D9 9A",
        "AA 08 28 00 00 00 D9 9A",
        "AA 07 7C 03 00 D4 D1",
        "AA 06 3A 01 4E C2",
        "AA 06 36 64 37 AC",
        "AA 06 32 00 D7 4A"
    };

    ui->comboBoxTVCmd->addItems(TVCmds);
    connect(ui->pushButtonTVSend,&QPushButton::clicked,this,[=]{
        if(m_COM1)
            m_COM1->send(ui->comboBoxTVCmd->currentText(),false);
    });

    connect(ui->pushButtonDotest,&QPushButton::clicked,this,[=]{
        if(!s_sock)
        {
            s_sock = new QTcpSocket(this);

            connect(s_sock,&QAbstractSocket::connected,this,[=]{
                // s_sock->write("start");
            });

            connect(s_sock,&QAbstractSocket::disconnected,this,[=]{
                s_sock->close();
                s_sock->deleteLater();
                s_sock = nullptr;
            });

            connect(s_sock,&QAbstractSocket::readyRead,this,[=]{
                qDebug() << s_sock->readAll();
            });

            s_sock->connectToHost("127.0.0.1",23367);
        }

        s_sock->write("start");
    });

    {
        tinyxml2::XMLDocument doc;
        XMLError error = doc.LoadFile("D:/vis(ch0~3).config");
        //XMLError tinyxml2::XMLDocument::Parse(const char *xml,size_t nBytes = static_cast<size_t>(-1));
        if (error != XMLError::XML_SUCCESS)
            return;
        tinyxml2::XMLElement* settings = doc.RootElement();      // settings
        qDebug() << settings->Name();
        tinyxml2::XMLElement* global = settings->FirstChildElement();  // global
        tinyxml2::XMLElement* devices = settings->FirstChildElement("devices"); // device
        tinyxml2::XMLElement* analyzers = settings->FirstChildElement("analyzers"); // analyzers
        qDebug() << global->Name();
        qDebug() << devices->Name();
        qDebug() << analyzers->Name();

        tinyxml2::XMLElement* socket = global->FirstChildElement("enaSocket");
        socket->SetText("1");

        tinyxml2::XMLElement* g1 = global->FirstChildElement("chnShowIndex");
        tinyxml2::XMLElement* g2 = global->FirstChildElement("chnShowMultip");
        qDebug() << g1->Name() << g1->GetText();
        qDebug() << g2->Name() << g2->GetText();

        tinyxml2::XMLElement* LA2016 = devices->FirstChildElement("LA2016");
        tinyxml2::XMLElement* LA5016 = devices->FirstChildElement("LA5016");
        tinyxml2::XMLElement* L1 = LA5016->FirstChildElement("chnTrig");
        tinyxml2::XMLElement* L2 = LA5016->FirstChildElement("chnEnable");
        qDebug() << L1->Name() << L1->GetText();
        qDebug() << L2->Name() << L2->GetText();

        tinyxml2::XMLElement* item0 = analyzers->FirstChildElement("item0");
        tinyxml2::XMLElement* parameters = item0->FirstChildElement("parameters");
        tinyxml2::XMLElement* format = item0->FirstChildElement("format");
        qDebug() << parameters->Name() << parameters->GetText();
        qDebug() << format->Name() << format->GetText();


        doc.SaveFile("D:\\test.xml");
    }

}

void MainWindow::sendLmCmd()
{
    int count = m_LmCmds.count();
    if(count <= 0)
        return;

    QString strCmd = m_LmCmds[0];
    m_LmCmds.pop_front();

    if(m_COM0) m_COM0->send(strCmd,true);
}

MainWindow::~MainWindow()
{
    delete ui;
}
