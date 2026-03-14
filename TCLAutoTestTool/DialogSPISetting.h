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

    void setModel(const QString&model);
    void setDeepth(int index);
    void setFreq(int index);

signals:
    void onLoadConfig(int index0,int index1);

private:
    Ui::DialogSPISetting *ui;
    QString m_strModel;
    QString m_VisFile;

    int m_nDepth=0;
    int m_nFrequ=0;

    void loadConfig();
    void saveConfig();
};

#endif // DIALOGSPISETTING_H
