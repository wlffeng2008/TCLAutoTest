#include "DialogSerialportList.h"
#include "ui_DialogSerialportList.h"

#include <QSerialPortInfo>
#include <QTableView>

DialogSerialportList::DialogSerialportList(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogSerialportList)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags()|Qt::WindowStaysOnTopHint|Qt::MSWindowsFixedSizeDialogHint) ;

    model = new QStandardItemModel();

    QStringList headers;
    headers << "名称" << "描述" << "序列号";
    model->setHorizontalHeaderLabels(headers);
    ui->tableView_serialportList->setModel(model);

    QHeaderView *header = ui->tableView_serialportList->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Stretch);
    header->setSectionResizeMode(0,QHeaderView::Fixed);
    header->resizeSection(0,90) ;
    header->setSectionResizeMode(1,QHeaderView::Fixed);
    header->resizeSection(1,280) ;
    on_pushButtonRefresh_clicked();
}

DialogSerialportList::~DialogSerialportList()
{
    delete ui;
}

void DialogSerialportList::on_pushButtonRefresh_clicked()
{
    while(model->rowCount())
        model->removeRow(0);

    QList<QSerialPortInfo> serialPortInfos = QSerialPortInfo::availablePorts();

    foreach (const QSerialPortInfo &serialPortInfo, serialPortInfos)
    {
        QStandardItem *item1 = new QStandardItem(serialPortInfo.portName());
        QStandardItem *item2 = new QStandardItem(serialPortInfo.description());
        QStandardItem *item3 = new QStandardItem(serialPortInfo.serialNumber());

        item1->setEditable(false);
        item2->setEditable(false);
        item3->setEditable(false);

        model->appendRow({item1,item2,item3}) ;
    }
}

