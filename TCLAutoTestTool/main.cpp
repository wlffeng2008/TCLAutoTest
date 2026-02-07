#include "MainWindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QLibraryInfo>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "TCLAutoTestTool_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }


    a.setStyleSheet( R"(

        QTableView {font-size: 12px; font-weight: bold; border: 1px solid gray;  background-color: rgb(220, 250, 255); gridline-color: gray;}
        QTableView::Item { padding-left:2px;  border-top: 0px solid gray; border-bottom: 1px solid transparent;border-right: 0px solid gray;}
        QTableView::Item::selected { background-color: #a0bb9e; color:white; }
        QTableView QTableCornerButton::section { background-color: skyblue ; min-width: 32px; border-top: 0px solid gray; border-bottom: 1px solid gray; border-left: 0px solid gray; border-right: 1px solid gray; }

        QTableView QHeaderView::section  {background-color: skyblue; }
        QTableView QHeaderView { background-color: skyblue; }
        QHeaderView::section:horizontal {  padding-left: 2px; border-top: 0px solid gray; border-bottom: 1px solid gray; border-right: 1px solid gray; font-weight: bold;}
        QHeaderView::section:vertical {  padding-left: 2px; min-width: 36px; border-top: 0px solid gray; border-bottom: 1px solid gray; border-left: 0px solid gray; border-right: 1px solid gray;}
        QTableView::indicator { width: 18px; height: 18px; }
        QTableView::indicator:checked { image: url(:/images/BoxChecked.png); }
        QTableView::indicator:unchecked { image: url(:/images/BoxUncheck.png); }
        QHeaderView::section:vertical{ text-align: right;}

        QLineEdit {border: 1px solid gray; border-radius: 4px; }
        QLineEdit:focus { border: 1px solid #50b7c1; border-radius: 4px;  background-color: rgb(230, 240, 255);}

        QTextEdit {border: 1px solid gray; border-radius: 0px; }
        QTextEdit:focus { border: 1px solid #50b7c1; border-radius: 4px; background-color: rgb(230, 240, 255);}

        QSlider::groove:horizontal { height: 8px; background: #DCDCDC; border-radius: 4px;}
        QSlider::sub-page:horizontal { background: #3EA8FF;  border-radius: 4px;}
        QSlider::handle:horizontal {
            width: 16px;
            height: 16px;
            margin: -6px 0;
            background: white;
            border: 2px solid #3EA8FF;
            border-radius: 9px; }

        QSlider::groove:vertical { width: 8px; background: #3EA8FF;  border-radius: 4px; }
        QSlider::sub-page:vertical { background: #DCDCDC;  border-radius: 4px; }
        QSlider::handle:vertical {
            width: 16px;
            height: 16px;
            margin: 0 -6px;
            background: white;
            border: 2px solid #3EA8FF;
            border-radius: 9px; }

        QSlider::handle:hover { background: #F0F0F0;}
        QSlider::handle:pressed { background: #E0E0E0; border-color: #2D7FDD; }

        QPushButton {
                background-color: #2D7FDD;
                border-radius: 8px;
                color: white;
                border: 1px solid #6C9F50;
                padding: 2px 2px;
                min-width: 60px;
                min-height: 16px; }

        QPushButton:hover { background-color: #87ceeb; }
        QPushButton:pressed { background-color: #1e90af;}
        QPushButton:checked { background-color: #1e90ff;}
        QPushButton:disabled { background-color: gray; color: #cccccc;}

        QMessageBox { min-width: 500px; min-height: 250px;}
        QMessageBox QLabel#qt_msgbox_label { min-width: 450px; min-height: 120px; max-width: 450px; max-height: 520px; qproperty-alignment: AlignLeft; white-space: pre-wrap;font: bold 12px 微软雅黑;}
        QMessageBox QLabel#qt_msgboxex_icon_label { min-width: 32px; min-height: 32px; max-width: 32px; max-height: 32px;qproperty-alignment: AlignTop;}
        QMessageBox QPushButton {  min-width: 80px;min-height: 24px;}

        QRadioButton { spacing: 5px; color: #333333; }
        QRadioButton::indicator {
            width: 14px;
            height: 14px;
            border: 2px solid #999999;
            border-radius: 8px;
        }

        QRadioButton::indicator:unchecked { background: #ffffff; }
        QRadioButton::indicator:unchecked:hover { border-color: #666666; background: #f0f0f0; }

        QRadioButton::indicator:checked {
            border: 2px solid #0085FF;
            background: qradialgradient(
                cx:0.5, cy:0.5, radius:0.4,
                fx:0.5, fy:0.5,
                stop:0 #0085FF, stop:1 white
            );
            color: #0085FF;
        }

        QRadioButton::indicator:checked:hover {
            border-color: #0066CC;
            background: qradialgradient(
                cx:0.5, cy:0.5, radius:0.4,
                fx:0.5, fy:0.5,
                stop:0 #0066CC, stop:1 white
            );
        }

        QRadioButton::indicator:pressed { border-color: #004499; }
        QRadioButton:disabled { color: #cccccc; }
        QRadioButton:checked {  color: #0085FF; }
        QRadioButton::indicator:disabled { border: 2px solid #dddddd; background: white; }

        QCheckBox { spacing: 5px; color: #333; }
        QCheckBox::indicator { width: 14px; height: 14px; }
        QCheckBox::indicator:unchecked { background: white; border: 2px solid #999; }
        QCheckBox::indicator:checked {
            width: 16px;
            height: 16px;
            background: transparent;
            image: url(:/images/BoxChecked.png);
        }

        QCheckBox::indicator:hover { border-color: #666; }
        QCheckBox:disabled { color: #AAA; }
        QCheckBox::indicator:disabled { background: #EEE; }

        QComboBox {
            border: 1px solid gray;
            border-radius: 4px;
            background: #FFFFFF;
            color: #333333;
            font-weight: normal;
            padding: 2px 2px;
        }

        QComboBox:hover { border-color: #ADADFD; }

        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 18px;
            border-left: 1px solid #E0E0E0;
        }

        QComboBox::down-arrow { width: 14px; height: 14px; image: url(:/images/down-arrow.png); }
        QComboBox:disabled { background: #F5F5F5; color: #9E9E9E; }
        QComboBox:disabled::down-arrow { opacity: 0.5; }

        QSpinBox {border: 1px solid gray; border-radius: 4px; }
        QSpinBox:focus{ border: 1px solid #50b7c1; border-radius: 4px;  background-color: rgb(226, 249, 255);}

        QGroupBox * { font-size: 12px; font-weight: semibold; }

    )");

    // QTranslator translator;
    // const QStringList uiLanguages = QLocale::system().uiLanguages();
    // for (const QString &locale : uiLanguages) {
    //     const QString baseName = "PX-ZBV100_" + QLocale(locale).name();
    //     if (translator.load(":/i18n/" + baseName)) {
    //         a.installTranslator(&translator);
    //         break;
    //     }
    // }

    QLocale::setDefault(QLocale(QLocale::Chinese,QLocale::China));

    QTranslator translatorA ;
    QTranslator translatorB ;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    translatorA.load("qt_zh_CN.qm", QLibraryInfo::path(QLibraryInfo::TranslationsPath));
    translatorB.load("qtbase_zh_CN.qm", QLibraryInfo::path(QLibraryInfo::TranslationsPath));
#else
    translatorA.load("qt_zh_CN.qm", QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    translatorB.load("qtbase_zh_CN.qm", QLibraryInfo::location(QLibraryInfo::TranslationsPath));
#endif

    qDebug() << "installTranslator";
    a.installTranslator(&translatorA) ;
    a.installTranslator(&translatorB) ;



    MainWindow w;
    w.show();
    return a.exec();
}
