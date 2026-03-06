#include "DialogTestFlow.h"
#include "ui_DialogTestFlow.h"

DialogTestFlow::DialogTestFlow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogTestFlow)
{
    ui->setupUi(this);
}

DialogTestFlow::~DialogTestFlow()
{
    delete ui;
}
