#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "DialogSerialportList.h"

#include "DialogTVCmd.h"
#include "DialogTestFlow.h"
#include "DialogSPISetting.h"

#include <windows.h>
#include <QTcpSocket>
#include <QDateTime>
#include <QStandardPaths>
#include <QTimer>
#include <QMessageBox>
#include <QFileDialog>

#define NO_MSXML_XMLDOCUMENT
#include <windows.h>
#include "tinyxml2.h"
using namespace tinyxml2;

static QStringList TVCmds = {
    "AA 06 10 01 A7 EF",    // 进入工程菜单
    "AA 06 11 03 B4 9C",    // 退出售后服务菜单
    "AA 06 11 02 A4 BD",
    "AA 07 9F 07 01 F1 55", // Local dimming开关
    "AA 06 30 03 A1 09",    // 图效选择(图像状态)
    "AA 07 9F 3E 01 4E 58", // 峰值亮度档位设置
    "AA 07 9F 0A 01 87 09", // 环境光感应开关设置
    "AA 06 25 01 5D 8F",    // 信源选择
    "AA 06 31 01 92 38",    // 色温设置
    "AA 09 93 01 02 03 04 4A 36", // 自然光参数设置

    "AA 06 27 01 3B ED",
    "AA 06 27 00 3B ED",
    "AA 08 28 FF FF FF 0B F6",
    "AA 08 28 FF 00 FF 08 09",
    "AA 08 28 FF 00 00 16 F9",
    "AA 08 28 00 FF 00 DA 65",
    "AA 08 28 00 00 FF C7 6A",
    "AA 08 28 00 00 00 D9 9A",
    "AA 07 7C 03 00 D4 D1",
    "AA 06 3A 01 4E C2",
    "AA 06 36 64 37 AC",
    "AA 06 32 00 D7 4A"
};

uint16_t crc = 0x0000;
const uint16_t polynomial = 0xA001 ;// 0x1021;//
WORD Get_CRC16_Sum(BYTE const* CRC_Buf, WORD nLen)
{
    WORD wCrc = 0xFFFF;
    for (WORD i = 0; i < nLen; i++)
    {
        wCrc ^= CRC_Buf[i]; //异或
        for (BYTE j = 0; j < 8; j++)
        {
            if (wCrc & 0x0001)
                wCrc = (wCrc >> 1) ^ polynomial;
            else
                wCrc = wCrc >> 1;
        }
    }
    return wCrc;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle(QString("LDM自动化调试 -- By QT") + QT_VERSION_STR);
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint | Qt::MSWindowsFixedSizeDialogHint);

    m_bLoading = true;

    m_pTVCmd = new DialogTVCmd(this);
    m_pTest  = new DialogTestFlow(this);
    m_pSPI   = new DialogSPISetting(this);

    connect(ui->pushButtonTVSendX,&QPushButton::clicked,this,[=]{
        m_pTVCmd->show();
    });
    connect(ui->pushButtonDotest,&QPushButton::clicked,this,[=]{
        m_pTest->show();
    });
    connect(ui->pushButtonSPISet,&QPushButton::clicked,this,[=]{
        m_pSPI->show();
    });


    connect(m_pTVCmd,&DialogTVCmd::onSendCmd,this,[=](const QString&strCmd){
        if(m_COM1) m_COM1->send(strCmd,false);
    });

    m_setting = new QSettings(QCoreApplication::applicationDirPath() + "\\setting.ini", QSettings::IniFormat);

    qDebug() << QString::asprintf("%04X", Get_CRC16_Sum( (BYTE *)QByteArray::fromHex(QString("AA 08 28 FF FF FF").toLatin1()).data(),6));

    ui->pushButtonCom0->setText(m_setting->value("port0","COM0").toString());
    ui->pushButtonCom1->setText(m_setting->value("port1","COM1").toString());
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

    connect(ui->lineEditInfoFile,&QLineEdit::textChanged,this,[=](const QString&text){
        ReloadInfo();
    });
    connect(ui->pushButtonReload,&QPushButton::clicked,this,[=]{
        ReloadInfo();
    });

    connect(ui->comboModel,&QComboBox::currentIndexChanged,this,[=](int index){
        QString strFile = ui->lineEditInfoFile->text().trimmed();
        Document xlsx(strFile);
        if (!xlsx.load())
        {
            QMessageBox::critical(this, "提示", "文件无法加载！\n" + strFile);
        }
        else
        {
            //QStringList sheets = xlsx.sheetNames();
            //foreach (const QString &strSheet, sheets)
            {
                //if (!xlsx.selectSheet(strSheet))
                //    continue;

                QStringList readCols = {"B","C","D","E","F","G","H","I","J","K","L","M"};

                for (int col = 0; col < readCols.count(); col++)
                {
                    QString strCell = QString::asprintf("%s%d", readCols[col].toStdString().c_str(), index+2);
                    QString strValue = xlsx.read(strCell).toString().trimmed();
                    if (strValue.isEmpty() && col == 0) {
                        continue;
                    }

                    switch (col) {
                    case 0: break;
                    case 1: ui->lineEditBase1->setText(strValue);  break;
                    case 2: ui->lineEditBase2->setText(strValue);  break;
                    case 3: ui->lineEditBase3->setText(strValue);  break;
                    case 4: ui->lineEditBase4->setText(strValue);  break;
                    case 5: ui->lineEditBase5->setText(strValue);  break;
                    case 6: ui->lineEditBase6->setText(strValue);  break;
                    case 7: ui->lineEditBase7->setText(strValue);  break;
                    case 8: ui->lineEditBase8->setText(strValue);  break;
                    case 9: ui->lineEditBase9->setText(strValue);  break;
                    case 10:ui->lineEditBase10->setText(strValue); break;
                    case 11:ui->lineEditBase11->setText(strValue); break;
                    default:
                        break;
                    }
                }

               // break;
            }
        }
    });

    QString strExcel=m_setting->value("Excel","./自动化测试导入数据.xlsx").toString();
    ui->lineEditInfoFile->setText(strExcel);
    connect(ui->pushButtonLoadInfo,&QPushButton::clicked,this,[=]{
        QString strFile = QFileDialog::getOpenFileName(
            this, "选择配置文件", nullptr, tr("XLSX文件(*.xlsx);;所有文件 (*.*)"));
        if (strFile.isEmpty())
            return;
        ui->lineEditInfoFile->setText(strFile);
    });

    connect(ui->checkBoxOntop,&QCheckBox::clicked,this,[=](bool checked){
        HWND hWnd = (HWND)this->winId();
        ::SetWindowPos(hWnd, checked ? HWND_TOPMOST:HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
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

                // OK00,P1 0.0992179;0.1051576;0.1289090;+0.09\r

                qDebug() << reply;
                if(reply.startsWith("OK") && reply.contains(",P1"))
                {
                    reply = reply.mid(8);
                    QStringList values = reply.split(';');

                    qreal X = values[0].toFloat();
                    qreal Y = values[1].toFloat();
                    qreal Z = values[2].toFloat();

                    qreal x = X / (X+Y+Z);
                    qreal y = Y / (X+Y+Z);

                    switch(m_nLMRead)
                    {
                    case 0:
                        ui->lineEditOut00->setText(QString::asprintf("%f",Y));
                        ui->lineEditOut01->setText(QString::asprintf("%f",x));
                        ui->lineEditOut02->setText(QString::asprintf("%f",y));
                        break;

                    case 1:
                        ui->lineEditOut03->setText(QString::asprintf("%f",Y));
                        break;
                    case 2:
                        ui->lineEditOut04->setText(QString::asprintf("%f",Y));
                        break;
                    }
                    m_nLMRead++;
                }

                QTimer::singleShot(20,this,[=]{ sendLmCmd(); });
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
        m_nLMRead = 0;

        ui->lineEditOut00->setText("--");
        ui->lineEditOut01->setText("--");
        ui->lineEditOut02->setText("--");
        ui->lineEditOut03->setText("--");
        ui->lineEditOut04->setText("--");

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

    connect(ui->comboBoxTVCmd,&QComboBox::activated,this,[=](int index){
        if(!m_bLoading)
            ui->pushButtonTVSend->click();
    });

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

        QTimer::singleShot(20,this,[=]{
            if(s_sock->state() == QAbstractSocket::ConnectedState)
                s_sock->write("start");
        });
    });

    {
        //Windows 版：C:\Users\'用户名'\AppData\Local\kingst\vis.config

        QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        QString strFile = appDataPath + QString("/AppData/Local/kingst/vis.config");
        tinyxml2::XMLDocument doc;
        XMLError error = doc.LoadFile(strFile.toStdString().c_str());
        qDebug() << "AppData 路径：" << strFile << error;
        if (error == XMLError::XML_SUCCESS)
        {
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

            doc.SaveFile(strFile.toStdString().c_str());
        }
    }

    QTimer::singleShot(100,this,[=]{
        InitTest();
        m_bLoading=false;
    });
}

void MainWindow::ReloadInfo()
{
    QString strFile = ui->lineEditInfoFile->text().trimmed();
    Document xlsx(strFile);
    if (!xlsx.load())
    {
        QMessageBox::critical(this, "提示", "文件无法加载！\n" + strFile);
    }
    else
    {
        ui->comboModel->clear();
        QStringList sheets = xlsx.sheetNames();
        foreach (const QString &strSheet, sheets)
        {
            if (!xlsx.selectSheet(strSheet))
                continue;
            for (int i = 2; i < 200; i++)
            {
                QString strCell = QString::asprintf("B%d", i);
                QString strValue = xlsx.read(strCell).toString().trimmed();
                if(strValue.isEmpty()) continue;
                ui->comboModel->addItem(strValue);
            }
            if(ui->comboModel->count())
            {
                ui->comboModel->setCurrentIndex(0);
                m_setting->setValue("Excel",strFile);
            }
            break;
        }
    }
}

void MainWindow::InitTest()
{
    ui->lineEditOut00->setText("--");
    ui->lineEditOut01->setText("--");
    ui->lineEditOut02->setText("--");
    ui->lineEditOut03->setText("--");
    ui->lineEditOut04->setText("--");

    ui->lineEditOut10->setText("--");
    ui->lineEditOut11->setText("--");
    ui->lineEditOut12->setText("--");
    ui->lineEditOut13->setText("--");
    ui->lineEditOut14->setText("--");
    ui->lineEditOut15->setText("--");

    ui->lineEditOut20->setText("--");
    ui->lineEditOut21->setText("--");
    ui->lineEditOut22->setText("--");
    ui->lineEditOut23->setText("--");
    ui->lineEditOut24->setText("--");
    ui->lineEditOut25->setText("--");
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

void MainWindow::sendTvCmd()
{
    int count = m_TvCmds.count();
    if(count <= 0)
        return;

    QString strCmd = m_TvCmds[0];
    m_TvCmds.pop_front();

    if(m_COM1) m_COM1->send(strCmd,true);
}

MainWindow::~MainWindow()
{
    delete ui;
}
