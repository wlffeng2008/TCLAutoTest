#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "DialogSerialportList.h"

#include <windows.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle(QString("LDM自动化调试 -- By QT") + QT_VERSION_STR);

    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint | Qt::MSWindowsFixedSizeDialogHint);

    connect(ui->pushButtonCom0,&QPushButton::clicked,this,[=]{
        DialogSerialportList comList;
        comList.exec();
    });

    connect(ui->checkBoxOntop,&QCheckBox::clicked,this,[=](bool checked){
        HWND hWnd = (HWND)this->winId();

            ::SetWindowPos(hWnd, checked?HWND_TOPMOST:HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    });
}

MainWindow::~MainWindow()
{
    delete ui;
}
