#include "DialogSPISetting.h"
#include "ui_DialogSPISetting.h"

#include "EasyToast.h"

#include <QFile>
#include <QTimer>
#include <QFileDialog>
#include <QStandardPaths>


#define NO_MSXML_XMLDOCUMENT
#include <windows.h>
#include "tinyxml2.h"
using namespace tinyxml2;

DialogSPISetting::DialogSPISetting(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogSPISetting)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags()|Qt::MSWindowsFixedSizeDialogHint);

    m_strModel = "LA5016";

    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    m_VisFile = appDataPath + QString("/AppData/Local/kingst/vis.config");
    m_CfgFile = QApplication::applicationDirPath() + "/vis.config" ;

    for(int i=1; i<=64; i++)
    {
        QString strItem = QString::asprintf("%2d 位单次传输字长", i);
        if(i == 8) strItem += "(标准)";
        ui->comboBox_6->addItem(strItem);
    }
    ui->comboBox_6->setCurrentIndex(7);

    connect(ui->pushButtonCancel,&QPushButton::clicked,this,[=]{
        this->hide();
    });
    connect(ui->pushButtonOK,&QPushButton::clicked,this,[=]{
        this->hide();
        saveConfig();
    });

    connect(ui->radioButton1,&QRadioButton::clicked,[=]{
        ui->comboBox_1->setCurrentIndex(1);
        ui->comboBox_2->setCurrentIndex(3);
        ui->comboBox_3->setCurrentIndex(0);
        ui->comboBox_4->setCurrentIndex(2);
    });

    connect(ui->radioButton2,&QRadioButton::clicked,[=]{
        ui->comboBox_1->setCurrentIndex(5);
        ui->comboBox_2->setCurrentIndex(7);
        ui->comboBox_3->setCurrentIndex(4);
        ui->comboBox_4->setCurrentIndex(6);
    });

    connect(ui->pushButtonLoad,&QRadioButton::clicked,[=]{
        QString strFile = QFileDialog::getOpenFileName(
            this, "选择配置文件", nullptr, tr("文配置件(*.config);;所有文件 (*.*)"));
        if (strFile.isEmpty())
            return;
        tinyxml2::XMLDocument doc;
        XMLError error = doc.LoadFile(strFile.toStdString().c_str());

        if (error == XMLError::XML_SUCCESS)
        {
            tinyxml2::XMLElement* settings = doc.RootElement();
            tinyxml2::XMLElement* global   = settings->FirstChildElement();
            tinyxml2::XMLElement* socket = global->FirstChildElement("enaSocket");
            socket->SetText("1");
            doc.SaveFile(m_CfgFile.toStdString().c_str());
            doc.SaveFile(m_VisFile.toStdString().c_str());
            loadConfig();
            EasyToast::information("分析仪配置导入成功！");
        }
        else
        {
            EasyToast::warning("分析仪配置导入失败！");
        }
    });

    connect(ui->pushButtonSaveAs,&QRadioButton::clicked,[=]{
        QString strFile = QFileDialog::getSaveFileName(
            this, "选择配置文件", nullptr, tr("文配置件(*.config);;所有文件 (*.*)"));
        if (strFile.isEmpty())
            return;
        tinyxml2::XMLDocument doc;
        XMLError error = doc.LoadFile(m_CfgFile.toStdString().c_str());
        if (error == XMLError::XML_SUCCESS)
        {
            tinyxml2::XMLElement* settings = doc.RootElement();
            tinyxml2::XMLElement* global   = settings->FirstChildElement();
            tinyxml2::XMLElement* socket = global->FirstChildElement("enaSocket");
            socket->SetText("1");
            EasyToast::information("分析仪配置导出成功！");
        }
        else
        {
            EasyToast::warning("分析仪配置导出失败！");
        }
    });

    connect(ui->comboBoxModel,&QComboBox::textActivated,this,[=](const QString & text){
        m_strModel = text;
    });

    QTimer::singleShot(300, this, [=]{
        loadConfig();
        m_bInited=true;
    });
}

void DialogSPISetting::saveConfig()
{
    if(!m_bInited) return;

    tinyxml2::XMLDocument doc;
    XMLError error = doc.LoadFile(m_CfgFile.toStdString().c_str());

    if (error == XMLError::XML_SUCCESS)
    {
        tinyxml2::XMLElement* settings = doc.RootElement();              // settings
        tinyxml2::XMLElement* global   = settings->FirstChildElement();  // global
        tinyxml2::XMLElement* devices  = settings->FirstChildElement("devices");    // device
        tinyxml2::XMLElement* analyzers = settings->FirstChildElement("analyzers"); // analyzers
        tinyxml2::XMLElement* socket = global->FirstChildElement("enaSocket");
        socket->SetText("1");

        tinyxml2::XMLElement* pModel = devices->FirstChildElement(m_strModel.toStdString().c_str());
        tinyxml2::XMLElement* L2 = pModel->FirstChildElement("chnEnable");
        QList<int> enables={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        enables[ui->comboBox_1->currentIndex()]=1;
        enables[ui->comboBox_2->currentIndex()]=1;
        enables[ui->comboBox_3->currentIndex()]=1;
        enables[ui->comboBox_4->currentIndex()]=1;
        QString strValue;
        for(int i=0; i<enables.size(); i++)
            strValue += QString("%1,").arg(enables[i]);
        strValue = strValue.chopped(1);
        L2->SetText(strValue.toStdString().c_str());
        {
            tinyxml2::XMLElement* V0 = pModel->FirstChildElement("smpDepth");
            tinyxml2::XMLElement* V1 = pModel->FirstChildElement("smpDepthIndex");
            tinyxml2::XMLElement* V2 = pModel->FirstChildElement("smpFrequ");
            tinyxml2::XMLElement* V3 = pModel->FirstChildElement("smpFrequIndex");

            int Depths[]={20,200,1000,2000,5000,10000,20000,50000,100000,200000,500000,1000000,2000000,5000000,10000000};
            int Frequs[]={500000,200000,100000,50000,20000,10000,5000,2000,1000,500,200,100,50,20};

            V0->SetText(Depths[m_nDepth]);
            V1->SetText(m_nDepth);
            V2->SetText(Frequs[m_nFrequ]);
            V3->SetText(m_nFrequ);
        }

        strValue.clear();
        strValue=QString::asprintf("SpiAnalyzer,%d,0,%d,0,%d,0,%d,0,%d,%d,%d,%d,%d,%d,",
                                     ui->comboBox_1->currentIndex(),
                                     ui->comboBox_2->currentIndex(),
                                     ui->comboBox_3->currentIndex(),
                                     ui->comboBox_4->currentIndex(),
                                     ui->comboBox_5->currentIndex(),
                                     ui->comboBox_6->currentIndex() + 1,
                                     ui->comboBox_7->currentIndex(),
                                     ui->comboBox_8->currentIndex(),
                                     ui->comboBox_9->currentIndex(),
                                     ui->checkBox->isChecked() ? 1 : 0
                                     );

        //SpiAnalyzer,5,0,7,0,4,0,6,0,0,16,0,0,0,1,
        tinyxml2::XMLElement* item0 = analyzers->FirstChildElement("item0");
        tinyxml2::XMLElement* parameters = item0->FirstChildElement("parameters");
        parameters->SetText(strValue.toStdString().c_str());
        doc.SaveFile(m_CfgFile.toStdString().c_str());
        doc.SaveFile(m_VisFile.toStdString().c_str());
    }
}

void DialogSPISetting::loadConfig()
{
    tinyxml2::XMLDocument doc;
    XMLError error = doc.LoadFile(m_CfgFile.toStdString().c_str());

    if (error == XMLError::XML_SUCCESS)
    {
        tinyxml2::XMLElement* settings = doc.RootElement();      // settings
        tinyxml2::XMLElement* global   = settings->FirstChildElement();  // global
        tinyxml2::XMLElement* devices  = settings->FirstChildElement("devices"); // device
        tinyxml2::XMLElement* analyzers = settings->FirstChildElement("analyzers"); // analyzers
        tinyxml2::XMLElement* socket = global->FirstChildElement("enaSocket");
        socket->SetText("1");

        tinyxml2::XMLElement* pModel = devices->FirstChildElement(m_strModel.toStdString().c_str());
        tinyxml2::XMLElement* V1 = pModel->FirstChildElement("smpDepthIndex");
        tinyxml2::XMLElement* V3 = pModel->FirstChildElement("smpFrequIndex");
        m_nDepth = QString(V1->GetText()).toInt();
        m_nFrequ = QString(V3->GetText()).toInt();

        tinyxml2::XMLElement* item0 = analyzers->FirstChildElement("item0");
        tinyxml2::XMLElement* parameters = item0->FirstChildElement("parameters");

        // parameters SpiAnalyzer,5,0,7,0,4,0,6,0,0,16,0,0,0,1,
        QString strParam = parameters->GetText();
        QStringList params = strParam.split(',');

        ui->comboBox_1->setCurrentIndex(params[1].toInt());
        ui->comboBox_2->setCurrentIndex(params[3].toInt());
        ui->comboBox_3->setCurrentIndex(params[5].toInt());
        ui->comboBox_4->setCurrentIndex(params[7].toInt());

        ui->comboBox_5->setCurrentIndex(params[9].toInt());
        ui->comboBox_6->setCurrentIndex(params[10].toInt()-1);
        ui->comboBox_7->setCurrentIndex(params[11].toInt());
        ui->comboBox_8->setCurrentIndex(params[12].toInt());
        ui->comboBox_9->setCurrentIndex(params[13].toInt());

        ui->checkBox->setChecked(params[14].toInt() == 1);

        doc.SaveFile(m_CfgFile.toStdString().c_str());
        doc.SaveFile(m_VisFile.toStdString().c_str());

        emit onLoadConfig(m_nDepth, m_nFrequ);
    }
}

void DialogSPISetting::setDeepth(int index)
{
    m_nDepth = index;
    saveConfig();
}

void DialogSPISetting::setFreq(int index)
{
    m_nFrequ = index;
    saveConfig();
}

DialogSPISetting::~DialogSPISetting()
{
    delete ui;
}
