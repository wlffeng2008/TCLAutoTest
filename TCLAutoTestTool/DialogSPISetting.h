#ifndef DIALOGSPISETTING_H
#define DIALOGSPISETTING_H

#include <QDialog>

namespace Ui {
class DialogSPISetting;
}

class DialogSPISetting : public QDialog
{
    Q_OBJECT

public:
    explicit DialogSPISetting(QWidget *parent = nullptr);
    ~DialogSPISetting();

private:
    Ui::DialogSPISetting *ui;
};

#endif // DIALOGSPISETTING_H
