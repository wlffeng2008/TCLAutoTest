#include "DialogSPISetting.h"
#include "ui_DialogSPISetting.h"

DialogSPISetting::DialogSPISetting(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogSPISetting)
{
    ui->setupUi(this);
}

DialogSPISetting::~DialogSPISetting()
{
    delete ui;
}
