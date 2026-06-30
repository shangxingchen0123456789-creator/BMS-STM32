/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.14.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QHBoxLayout *mainHLayout;
    QTabWidget *tabWidget;
    QWidget *tabData;
    QVBoxLayout *verticalLayout_7;
    QHBoxLayout *horizontalLayout_14;
    QGroupBox *groupBox_5;
    QHBoxLayout *horizontalLayout_11;
    QProgressBar *progressSoc;
    QVBoxLayout *verticalLayout_4;
    QHBoxLayout *horizontalLayout_12;
    QLabel *label_41;
    QLineEdit *labelChgState;
    QHBoxLayout *horizontalLayout_13;
    QLabel *label_42;
    QLineEdit *labelDsgState;
    QGroupBox *groupBox_4;
    QVBoxLayout *packInfoVLayout;
    QHBoxLayout *horizontalLayout_3;
    QSplitter *splitter;
    QLabel *label_3;
    QLineEdit *cellEdit1;
    QLabel *label_4;
    QSplitter *splitter_6;
    QLabel *label_15;
    QLineEdit *cellEdit6;
    QLabel *label_16;
    QSplitter *splitter_7;
    QLabel *label_17;
    QLineEdit *cellEdit11;
    QLabel *label_18;
    QHBoxLayout *horizontalLayout_6;
    QSplitter *splitter_2;
    QLabel *label_5;
    QLineEdit *cellEdit2;
    QLabel *label_6;
    QSplitter *splitter_8;
    QLabel *label_19;
    QLineEdit *cellEdit7;
    QLabel *label_20;
    QSplitter *splitter_9;
    QLabel *label_21;
    QLineEdit *cellEdit12;
    QLabel *label_22;
    QHBoxLayout *horizontalLayout_7;
    QSplitter *splitter_3;
    QLabel *label_7;
    QLineEdit *cellEdit3;
    QLabel *label_8;
    QSplitter *splitter_10;
    QLabel *label_23;
    QLineEdit *cellEdit8;
    QLabel *label_24;
    QSplitter *splitter_11;
    QLabel *label_25;
    QLineEdit *cellEdit13;
    QLabel *label_26;
    QHBoxLayout *horizontalLayout_8;
    QSplitter *splitter_4;
    QLabel *label_9;
    QLineEdit *cellEdit4;
    QLabel *label_10;
    QSplitter *splitter_12;
    QLabel *label_27;
    QLineEdit *cellEdit9;
    QLabel *label_28;
    QSplitter *splitter_13;
    QLabel *label_29;
    QLineEdit *cellEdit14;
    QLabel *label_30;
    QHBoxLayout *horizontalLayout_9;
    QSplitter *splitter_5;
    QLabel *label_11;
    QLineEdit *cellEdit5;
    QLabel *label_12;
    QSplitter *splitter_14;
    QLabel *label_31;
    QLineEdit *cellEdit10;
    QLabel *label_32;
    QSplitter *splitter_15;
    QLabel *label_33;
    QLineEdit *cellEdit15;
    QLabel *label_34;
    QHBoxLayout *horizontalLayout_10;
    QSplitter *splitter_16;
    QLabel *label_35;
    QLineEdit *linePackV;
    QLabel *label_36;
    QSplitter *splitter_17;
    QLabel *label_37;
    QLineEdit *lineCurrent;
    QLabel *label_38;
    QSplitter *splitter_18;
    QLabel *label_39;
    QLineEdit *lineTemp;
    QLabel *label_40;
    QGroupBox *groupBox_2;
    QVBoxLayout *verticalLayout_2;
    QTextEdit *plainTextEdit;
    QWidget *tabChart;
    QVBoxLayout *chartTabLayout;
    QWidget *chartWidget;
    QWidget *tabBar;
    QVBoxLayout *barTabLayout;
    QWidget *barChartWidget;
    QWidget *tabSettings;
    QVBoxLayout *settingsTabLayout;
    QWidget *settingsWidget;
    QVBoxLayout *verticalLayout_8;
    QGroupBox *groupBox;
    QVBoxLayout *verticalLayout_6;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QComboBox *comboPort;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_2;
    QComboBox *comboBaud;
    QHBoxLayout *horizontalLayout_17;
    QPushButton *btnConnect;
    QGroupBox *groupBox_3;
    QVBoxLayout *verticalLayout_5;
    QHBoxLayout *horizontalLayout_15;
    QPushButton *btnStart;
    QPushButton *btnStop;
    QPushButton *btnChgOn;
    QPushButton *btnChgOff;
    QPushButton *btnDsgOn;
    QPushButton *btnDsgOff;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(800, 600);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        mainHLayout = new QHBoxLayout(centralwidget);
        mainHLayout->setObjectName(QString::fromUtf8("mainHLayout"));
        tabWidget = new QTabWidget(centralwidget);
        tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(3);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(tabWidget->sizePolicy().hasHeightForWidth());
        tabWidget->setSizePolicy(sizePolicy);
        tabData = new QWidget();
        tabData->setObjectName(QString::fromUtf8("tabData"));
        verticalLayout_7 = new QVBoxLayout(tabData);
        verticalLayout_7->setObjectName(QString::fromUtf8("verticalLayout_7"));
        horizontalLayout_14 = new QHBoxLayout();
        horizontalLayout_14->setObjectName(QString::fromUtf8("horizontalLayout_14"));
        groupBox_5 = new QGroupBox(tabData);
        groupBox_5->setObjectName(QString::fromUtf8("groupBox_5"));
        horizontalLayout_11 = new QHBoxLayout(groupBox_5);
        horizontalLayout_11->setObjectName(QString::fromUtf8("horizontalLayout_11"));
        progressSoc = new QProgressBar(groupBox_5);
        progressSoc->setObjectName(QString::fromUtf8("progressSoc"));
        progressSoc->setEnabled(true);
        progressSoc->setValue(0);
        progressSoc->setTextVisible(true);

        horizontalLayout_11->addWidget(progressSoc);


        horizontalLayout_14->addWidget(groupBox_5);

        verticalLayout_4 = new QVBoxLayout();
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        horizontalLayout_12 = new QHBoxLayout();
        horizontalLayout_12->setObjectName(QString::fromUtf8("horizontalLayout_12"));
        label_41 = new QLabel(tabData);
        label_41->setObjectName(QString::fromUtf8("label_41"));
        QFont font;
        font.setPointSize(10);
        label_41->setFont(font);

        horizontalLayout_12->addWidget(label_41);

        labelChgState = new QLineEdit(tabData);
        labelChgState->setObjectName(QString::fromUtf8("labelChgState"));

        horizontalLayout_12->addWidget(labelChgState);


        verticalLayout_4->addLayout(horizontalLayout_12);

        horizontalLayout_13 = new QHBoxLayout();
        horizontalLayout_13->setObjectName(QString::fromUtf8("horizontalLayout_13"));
        label_42 = new QLabel(tabData);
        label_42->setObjectName(QString::fromUtf8("label_42"));
        label_42->setFont(font);

        horizontalLayout_13->addWidget(label_42);

        labelDsgState = new QLineEdit(tabData);
        labelDsgState->setObjectName(QString::fromUtf8("labelDsgState"));

        horizontalLayout_13->addWidget(labelDsgState);


        verticalLayout_4->addLayout(horizontalLayout_13);


        horizontalLayout_14->addLayout(verticalLayout_4);


        verticalLayout_7->addLayout(horizontalLayout_14);

        groupBox_4 = new QGroupBox(tabData);
        groupBox_4->setObjectName(QString::fromUtf8("groupBox_4"));
        groupBox_4->setMinimumSize(QSize(471, 221));
        packInfoVLayout = new QVBoxLayout(groupBox_4);
        packInfoVLayout->setSpacing(4);
        packInfoVLayout->setObjectName(QString::fromUtf8("packInfoVLayout"));
        packInfoVLayout->setContentsMargins(4, 4, 4, 4);
        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        splitter = new QSplitter(groupBox_4);
        splitter->setObjectName(QString::fromUtf8("splitter"));
        splitter->setOrientation(Qt::Horizontal);
        label_3 = new QLabel(splitter);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        QFont font1;
        font1.setPointSize(11);
        label_3->setFont(font1);
        splitter->addWidget(label_3);
        cellEdit1 = new QLineEdit(splitter);
        cellEdit1->setObjectName(QString::fromUtf8("cellEdit1"));
        splitter->addWidget(cellEdit1);
        label_4 = new QLabel(splitter);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setFont(font);
        splitter->addWidget(label_4);

        horizontalLayout_3->addWidget(splitter);

        splitter_6 = new QSplitter(groupBox_4);
        splitter_6->setObjectName(QString::fromUtf8("splitter_6"));
        splitter_6->setOrientation(Qt::Horizontal);
        label_15 = new QLabel(splitter_6);
        label_15->setObjectName(QString::fromUtf8("label_15"));
        label_15->setFont(font1);
        splitter_6->addWidget(label_15);
        cellEdit6 = new QLineEdit(splitter_6);
        cellEdit6->setObjectName(QString::fromUtf8("cellEdit6"));
        splitter_6->addWidget(cellEdit6);
        label_16 = new QLabel(splitter_6);
        label_16->setObjectName(QString::fromUtf8("label_16"));
        label_16->setFont(font);
        splitter_6->addWidget(label_16);

        horizontalLayout_3->addWidget(splitter_6);

        splitter_7 = new QSplitter(groupBox_4);
        splitter_7->setObjectName(QString::fromUtf8("splitter_7"));
        splitter_7->setOrientation(Qt::Horizontal);
        label_17 = new QLabel(splitter_7);
        label_17->setObjectName(QString::fromUtf8("label_17"));
        label_17->setFont(font1);
        splitter_7->addWidget(label_17);
        cellEdit11 = new QLineEdit(splitter_7);
        cellEdit11->setObjectName(QString::fromUtf8("cellEdit11"));
        cellEdit11->setEnabled(true);
        splitter_7->addWidget(cellEdit11);
        label_18 = new QLabel(splitter_7);
        label_18->setObjectName(QString::fromUtf8("label_18"));
        label_18->setFont(font);
        splitter_7->addWidget(label_18);

        horizontalLayout_3->addWidget(splitter_7);


        packInfoVLayout->addLayout(horizontalLayout_3);

        horizontalLayout_6 = new QHBoxLayout();
        horizontalLayout_6->setObjectName(QString::fromUtf8("horizontalLayout_6"));
        splitter_2 = new QSplitter(groupBox_4);
        splitter_2->setObjectName(QString::fromUtf8("splitter_2"));
        splitter_2->setOrientation(Qt::Horizontal);
        label_5 = new QLabel(splitter_2);
        label_5->setObjectName(QString::fromUtf8("label_5"));
        label_5->setFont(font1);
        splitter_2->addWidget(label_5);
        cellEdit2 = new QLineEdit(splitter_2);
        cellEdit2->setObjectName(QString::fromUtf8("cellEdit2"));
        splitter_2->addWidget(cellEdit2);
        label_6 = new QLabel(splitter_2);
        label_6->setObjectName(QString::fromUtf8("label_6"));
        label_6->setFont(font);
        splitter_2->addWidget(label_6);

        horizontalLayout_6->addWidget(splitter_2);

        splitter_8 = new QSplitter(groupBox_4);
        splitter_8->setObjectName(QString::fromUtf8("splitter_8"));
        splitter_8->setOrientation(Qt::Horizontal);
        label_19 = new QLabel(splitter_8);
        label_19->setObjectName(QString::fromUtf8("label_19"));
        label_19->setFont(font1);
        splitter_8->addWidget(label_19);
        cellEdit7 = new QLineEdit(splitter_8);
        cellEdit7->setObjectName(QString::fromUtf8("cellEdit7"));
        splitter_8->addWidget(cellEdit7);
        label_20 = new QLabel(splitter_8);
        label_20->setObjectName(QString::fromUtf8("label_20"));
        label_20->setFont(font);
        splitter_8->addWidget(label_20);

        horizontalLayout_6->addWidget(splitter_8);

        splitter_9 = new QSplitter(groupBox_4);
        splitter_9->setObjectName(QString::fromUtf8("splitter_9"));
        splitter_9->setOrientation(Qt::Horizontal);
        label_21 = new QLabel(splitter_9);
        label_21->setObjectName(QString::fromUtf8("label_21"));
        label_21->setFont(font1);
        splitter_9->addWidget(label_21);
        cellEdit12 = new QLineEdit(splitter_9);
        cellEdit12->setObjectName(QString::fromUtf8("cellEdit12"));
        splitter_9->addWidget(cellEdit12);
        label_22 = new QLabel(splitter_9);
        label_22->setObjectName(QString::fromUtf8("label_22"));
        label_22->setFont(font);
        splitter_9->addWidget(label_22);

        horizontalLayout_6->addWidget(splitter_9);


        packInfoVLayout->addLayout(horizontalLayout_6);

        horizontalLayout_7 = new QHBoxLayout();
        horizontalLayout_7->setObjectName(QString::fromUtf8("horizontalLayout_7"));
        splitter_3 = new QSplitter(groupBox_4);
        splitter_3->setObjectName(QString::fromUtf8("splitter_3"));
        splitter_3->setOrientation(Qt::Horizontal);
        label_7 = new QLabel(splitter_3);
        label_7->setObjectName(QString::fromUtf8("label_7"));
        label_7->setFont(font1);
        splitter_3->addWidget(label_7);
        cellEdit3 = new QLineEdit(splitter_3);
        cellEdit3->setObjectName(QString::fromUtf8("cellEdit3"));
        splitter_3->addWidget(cellEdit3);
        label_8 = new QLabel(splitter_3);
        label_8->setObjectName(QString::fromUtf8("label_8"));
        label_8->setFont(font);
        splitter_3->addWidget(label_8);

        horizontalLayout_7->addWidget(splitter_3);

        splitter_10 = new QSplitter(groupBox_4);
        splitter_10->setObjectName(QString::fromUtf8("splitter_10"));
        splitter_10->setOrientation(Qt::Horizontal);
        label_23 = new QLabel(splitter_10);
        label_23->setObjectName(QString::fromUtf8("label_23"));
        label_23->setFont(font1);
        splitter_10->addWidget(label_23);
        cellEdit8 = new QLineEdit(splitter_10);
        cellEdit8->setObjectName(QString::fromUtf8("cellEdit8"));
        splitter_10->addWidget(cellEdit8);
        label_24 = new QLabel(splitter_10);
        label_24->setObjectName(QString::fromUtf8("label_24"));
        label_24->setFont(font);
        splitter_10->addWidget(label_24);

        horizontalLayout_7->addWidget(splitter_10);

        splitter_11 = new QSplitter(groupBox_4);
        splitter_11->setObjectName(QString::fromUtf8("splitter_11"));
        splitter_11->setOrientation(Qt::Horizontal);
        label_25 = new QLabel(splitter_11);
        label_25->setObjectName(QString::fromUtf8("label_25"));
        label_25->setFont(font1);
        splitter_11->addWidget(label_25);
        cellEdit13 = new QLineEdit(splitter_11);
        cellEdit13->setObjectName(QString::fromUtf8("cellEdit13"));
        splitter_11->addWidget(cellEdit13);
        label_26 = new QLabel(splitter_11);
        label_26->setObjectName(QString::fromUtf8("label_26"));
        label_26->setFont(font);
        splitter_11->addWidget(label_26);

        horizontalLayout_7->addWidget(splitter_11);


        packInfoVLayout->addLayout(horizontalLayout_7);

        horizontalLayout_8 = new QHBoxLayout();
        horizontalLayout_8->setObjectName(QString::fromUtf8("horizontalLayout_8"));
        splitter_4 = new QSplitter(groupBox_4);
        splitter_4->setObjectName(QString::fromUtf8("splitter_4"));
        splitter_4->setOrientation(Qt::Horizontal);
        label_9 = new QLabel(splitter_4);
        label_9->setObjectName(QString::fromUtf8("label_9"));
        label_9->setFont(font1);
        splitter_4->addWidget(label_9);
        cellEdit4 = new QLineEdit(splitter_4);
        cellEdit4->setObjectName(QString::fromUtf8("cellEdit4"));
        splitter_4->addWidget(cellEdit4);
        label_10 = new QLabel(splitter_4);
        label_10->setObjectName(QString::fromUtf8("label_10"));
        label_10->setFont(font);
        splitter_4->addWidget(label_10);

        horizontalLayout_8->addWidget(splitter_4);

        splitter_12 = new QSplitter(groupBox_4);
        splitter_12->setObjectName(QString::fromUtf8("splitter_12"));
        splitter_12->setOrientation(Qt::Horizontal);
        label_27 = new QLabel(splitter_12);
        label_27->setObjectName(QString::fromUtf8("label_27"));
        label_27->setFont(font1);
        splitter_12->addWidget(label_27);
        cellEdit9 = new QLineEdit(splitter_12);
        cellEdit9->setObjectName(QString::fromUtf8("cellEdit9"));
        splitter_12->addWidget(cellEdit9);
        label_28 = new QLabel(splitter_12);
        label_28->setObjectName(QString::fromUtf8("label_28"));
        label_28->setFont(font);
        splitter_12->addWidget(label_28);

        horizontalLayout_8->addWidget(splitter_12);

        splitter_13 = new QSplitter(groupBox_4);
        splitter_13->setObjectName(QString::fromUtf8("splitter_13"));
        splitter_13->setOrientation(Qt::Horizontal);
        label_29 = new QLabel(splitter_13);
        label_29->setObjectName(QString::fromUtf8("label_29"));
        label_29->setFont(font1);
        splitter_13->addWidget(label_29);
        cellEdit14 = new QLineEdit(splitter_13);
        cellEdit14->setObjectName(QString::fromUtf8("cellEdit14"));
        splitter_13->addWidget(cellEdit14);
        label_30 = new QLabel(splitter_13);
        label_30->setObjectName(QString::fromUtf8("label_30"));
        label_30->setFont(font);
        splitter_13->addWidget(label_30);

        horizontalLayout_8->addWidget(splitter_13);


        packInfoVLayout->addLayout(horizontalLayout_8);

        horizontalLayout_9 = new QHBoxLayout();
        horizontalLayout_9->setObjectName(QString::fromUtf8("horizontalLayout_9"));
        splitter_5 = new QSplitter(groupBox_4);
        splitter_5->setObjectName(QString::fromUtf8("splitter_5"));
        splitter_5->setOrientation(Qt::Horizontal);
        label_11 = new QLabel(splitter_5);
        label_11->setObjectName(QString::fromUtf8("label_11"));
        label_11->setFont(font1);
        splitter_5->addWidget(label_11);
        cellEdit5 = new QLineEdit(splitter_5);
        cellEdit5->setObjectName(QString::fromUtf8("cellEdit5"));
        splitter_5->addWidget(cellEdit5);
        label_12 = new QLabel(splitter_5);
        label_12->setObjectName(QString::fromUtf8("label_12"));
        label_12->setFont(font);
        splitter_5->addWidget(label_12);

        horizontalLayout_9->addWidget(splitter_5);

        splitter_14 = new QSplitter(groupBox_4);
        splitter_14->setObjectName(QString::fromUtf8("splitter_14"));
        splitter_14->setOrientation(Qt::Horizontal);
        label_31 = new QLabel(splitter_14);
        label_31->setObjectName(QString::fromUtf8("label_31"));
        label_31->setFont(font1);
        splitter_14->addWidget(label_31);
        cellEdit10 = new QLineEdit(splitter_14);
        cellEdit10->setObjectName(QString::fromUtf8("cellEdit10"));
        splitter_14->addWidget(cellEdit10);
        label_32 = new QLabel(splitter_14);
        label_32->setObjectName(QString::fromUtf8("label_32"));
        label_32->setFont(font);
        splitter_14->addWidget(label_32);

        horizontalLayout_9->addWidget(splitter_14);

        splitter_15 = new QSplitter(groupBox_4);
        splitter_15->setObjectName(QString::fromUtf8("splitter_15"));
        splitter_15->setOrientation(Qt::Horizontal);
        label_33 = new QLabel(splitter_15);
        label_33->setObjectName(QString::fromUtf8("label_33"));
        label_33->setFont(font1);
        splitter_15->addWidget(label_33);
        cellEdit15 = new QLineEdit(splitter_15);
        cellEdit15->setObjectName(QString::fromUtf8("cellEdit15"));
        splitter_15->addWidget(cellEdit15);
        label_34 = new QLabel(splitter_15);
        label_34->setObjectName(QString::fromUtf8("label_34"));
        label_34->setFont(font);
        splitter_15->addWidget(label_34);

        horizontalLayout_9->addWidget(splitter_15);


        packInfoVLayout->addLayout(horizontalLayout_9);

        horizontalLayout_10 = new QHBoxLayout();
        horizontalLayout_10->setObjectName(QString::fromUtf8("horizontalLayout_10"));
        splitter_16 = new QSplitter(groupBox_4);
        splitter_16->setObjectName(QString::fromUtf8("splitter_16"));
        splitter_16->setOrientation(Qt::Horizontal);
        label_35 = new QLabel(splitter_16);
        label_35->setObjectName(QString::fromUtf8("label_35"));
        label_35->setFont(font1);
        splitter_16->addWidget(label_35);
        linePackV = new QLineEdit(splitter_16);
        linePackV->setObjectName(QString::fromUtf8("linePackV"));
        splitter_16->addWidget(linePackV);
        label_36 = new QLabel(splitter_16);
        label_36->setObjectName(QString::fromUtf8("label_36"));
        label_36->setFont(font1);
        splitter_16->addWidget(label_36);

        horizontalLayout_10->addWidget(splitter_16);

        splitter_17 = new QSplitter(groupBox_4);
        splitter_17->setObjectName(QString::fromUtf8("splitter_17"));
        splitter_17->setOrientation(Qt::Horizontal);
        label_37 = new QLabel(splitter_17);
        label_37->setObjectName(QString::fromUtf8("label_37"));
        label_37->setFont(font1);
        splitter_17->addWidget(label_37);
        lineCurrent = new QLineEdit(splitter_17);
        lineCurrent->setObjectName(QString::fromUtf8("lineCurrent"));
        splitter_17->addWidget(lineCurrent);
        label_38 = new QLabel(splitter_17);
        label_38->setObjectName(QString::fromUtf8("label_38"));
        label_38->setFont(font1);
        splitter_17->addWidget(label_38);

        horizontalLayout_10->addWidget(splitter_17);

        splitter_18 = new QSplitter(groupBox_4);
        splitter_18->setObjectName(QString::fromUtf8("splitter_18"));
        splitter_18->setOrientation(Qt::Horizontal);
        label_39 = new QLabel(splitter_18);
        label_39->setObjectName(QString::fromUtf8("label_39"));
        label_39->setFont(font1);
        splitter_18->addWidget(label_39);
        lineTemp = new QLineEdit(splitter_18);
        lineTemp->setObjectName(QString::fromUtf8("lineTemp"));
        splitter_18->addWidget(lineTemp);
        label_40 = new QLabel(splitter_18);
        label_40->setObjectName(QString::fromUtf8("label_40"));
        label_40->setFont(font1);
        splitter_18->addWidget(label_40);

        horizontalLayout_10->addWidget(splitter_18);


        packInfoVLayout->addLayout(horizontalLayout_10);


        verticalLayout_7->addWidget(groupBox_4);

        groupBox_2 = new QGroupBox(tabData);
        groupBox_2->setObjectName(QString::fromUtf8("groupBox_2"));
        verticalLayout_2 = new QVBoxLayout(groupBox_2);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        plainTextEdit = new QTextEdit(groupBox_2);
        plainTextEdit->setObjectName(QString::fromUtf8("plainTextEdit"));

        verticalLayout_2->addWidget(plainTextEdit);


        verticalLayout_7->addWidget(groupBox_2);

        tabWidget->addTab(tabData, QString());
        tabChart = new QWidget();
        tabChart->setObjectName(QString::fromUtf8("tabChart"));
        chartTabLayout = new QVBoxLayout(tabChart);
        chartTabLayout->setObjectName(QString::fromUtf8("chartTabLayout"));
        chartWidget = new QWidget(tabChart);
        chartWidget->setObjectName(QString::fromUtf8("chartWidget"));
        chartWidget->setMinimumSize(QSize(400, 300));

        chartTabLayout->addWidget(chartWidget);

        tabWidget->addTab(tabChart, QString());
        tabBar = new QWidget();
        tabBar->setObjectName(QString::fromUtf8("tabBar"));
        barTabLayout = new QVBoxLayout(tabBar);
        barTabLayout->setObjectName(QString::fromUtf8("barTabLayout"));
        barChartWidget = new QWidget(tabBar);
        barChartWidget->setObjectName(QString::fromUtf8("barChartWidget"));
        barChartWidget->setMinimumSize(QSize(400, 300));

        barTabLayout->addWidget(barChartWidget);

        tabWidget->addTab(tabBar, QString());
        tabSettings = new QWidget();
        tabSettings->setObjectName(QString::fromUtf8("tabSettings"));
        settingsTabLayout = new QVBoxLayout(tabSettings);
        settingsTabLayout->setObjectName(QString::fromUtf8("settingsTabLayout"));
        settingsWidget = new QWidget(tabSettings);
        settingsWidget->setObjectName(QString::fromUtf8("settingsWidget"));

        settingsTabLayout->addWidget(settingsWidget);

        tabWidget->addTab(tabSettings, QString());

        mainHLayout->addWidget(tabWidget);

        verticalLayout_8 = new QVBoxLayout();
        verticalLayout_8->setObjectName(QString::fromUtf8("verticalLayout_8"));
        groupBox = new QGroupBox(centralwidget);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        verticalLayout_6 = new QVBoxLayout(groupBox);
        verticalLayout_6->setObjectName(QString::fromUtf8("verticalLayout_6"));
        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        label = new QLabel(groupBox);
        label->setObjectName(QString::fromUtf8("label"));
        label->setFont(font1);

        horizontalLayout->addWidget(label);

        comboPort = new QComboBox(groupBox);
        comboPort->setObjectName(QString::fromUtf8("comboPort"));
        QFont font2;
        font2.setPointSize(13);
        comboPort->setFont(font2);

        horizontalLayout->addWidget(comboPort);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        label_2 = new QLabel(groupBox);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setFont(font1);

        horizontalLayout_2->addWidget(label_2);

        comboBaud = new QComboBox(groupBox);
        comboBaud->addItem(QString());
        comboBaud->setObjectName(QString::fromUtf8("comboBaud"));
        comboBaud->setFont(font2);

        horizontalLayout_2->addWidget(comboBaud);


        verticalLayout->addLayout(horizontalLayout_2);


        verticalLayout_6->addLayout(verticalLayout);

        horizontalLayout_17 = new QHBoxLayout();
        horizontalLayout_17->setObjectName(QString::fromUtf8("horizontalLayout_17"));
        btnConnect = new QPushButton(groupBox);
        btnConnect->setObjectName(QString::fromUtf8("btnConnect"));
        btnConnect->setMinimumSize(QSize(0, 30));
        btnConnect->setFont(font1);
        btnConnect->setIconSize(QSize(20, 25));

        horizontalLayout_17->addWidget(btnConnect);


        verticalLayout_6->addLayout(horizontalLayout_17);


        verticalLayout_8->addWidget(groupBox);

        groupBox_3 = new QGroupBox(centralwidget);
        groupBox_3->setObjectName(QString::fromUtf8("groupBox_3"));
        verticalLayout_5 = new QVBoxLayout(groupBox_3);
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        horizontalLayout_15 = new QHBoxLayout();
        horizontalLayout_15->setObjectName(QString::fromUtf8("horizontalLayout_15"));
        btnStart = new QPushButton(groupBox_3);
        btnStart->setObjectName(QString::fromUtf8("btnStart"));
        btnStart->setMinimumSize(QSize(0, 30));
        btnStart->setFont(font);
        btnStart->setIconSize(QSize(20, 25));

        horizontalLayout_15->addWidget(btnStart);

        btnStop = new QPushButton(groupBox_3);
        btnStop->setObjectName(QString::fromUtf8("btnStop"));
        btnStop->setMinimumSize(QSize(0, 30));
        btnStop->setFont(font);
        btnStop->setIconSize(QSize(20, 25));

        horizontalLayout_15->addWidget(btnStop);


        verticalLayout_5->addLayout(horizontalLayout_15);

        btnChgOn = new QPushButton(groupBox_3);
        btnChgOn->setObjectName(QString::fromUtf8("btnChgOn"));
        btnChgOn->setMinimumSize(QSize(110, 30));
        btnChgOn->setFont(font);
        btnChgOn->setIconSize(QSize(20, 25));

        verticalLayout_5->addWidget(btnChgOn);

        btnChgOff = new QPushButton(groupBox_3);
        btnChgOff->setObjectName(QString::fromUtf8("btnChgOff"));
        btnChgOff->setMinimumSize(QSize(110, 30));
        btnChgOff->setFont(font);
        btnChgOff->setIconSize(QSize(20, 25));

        verticalLayout_5->addWidget(btnChgOff);

        btnDsgOn = new QPushButton(groupBox_3);
        btnDsgOn->setObjectName(QString::fromUtf8("btnDsgOn"));
        btnDsgOn->setMinimumSize(QSize(110, 30));
        btnDsgOn->setFont(font);
        btnDsgOn->setIconSize(QSize(20, 25));

        verticalLayout_5->addWidget(btnDsgOn);

        btnDsgOff = new QPushButton(groupBox_3);
        btnDsgOff->setObjectName(QString::fromUtf8("btnDsgOff"));
        btnDsgOff->setMinimumSize(QSize(110, 30));
        btnDsgOff->setFont(font);
        btnDsgOff->setIconSize(QSize(20, 25));

        verticalLayout_5->addWidget(btnDsgOff);


        verticalLayout_8->addWidget(groupBox_3);


        mainHLayout->addLayout(verticalLayout_8);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 800, 21));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "BMS \347\224\265\346\261\240\347\256\241\347\220\206\347\263\273\347\273\237", nullptr));
        groupBox_5->setTitle(QCoreApplication::translate("MainWindow", "\345\211\251\344\275\231\347\224\265\351\207\217SOC", nullptr));
        label_41->setText(QCoreApplication::translate("MainWindow", "\345\205\205\347\224\265MOS\347\212\266\346\200\201", nullptr));
        label_42->setText(QCoreApplication::translate("MainWindow", "\346\224\276\347\224\265MOS\347\212\266\346\200\201", nullptr));
        groupBox_4->setTitle(QCoreApplication::translate("MainWindow", "\347\224\265\346\261\240\345\214\205\344\277\241\346\201\257", nullptr));
        label_3->setText(QCoreApplication::translate("MainWindow", "1\357\274\232", nullptr));
        label_4->setText(QCoreApplication::translate("MainWindow", "mV", nullptr));
        label_15->setText(QCoreApplication::translate("MainWindow", "6\357\274\232", nullptr));
        label_16->setText(QCoreApplication::translate("MainWindow", "mV", nullptr));
        label_17->setText(QCoreApplication::translate("MainWindow", "11\357\274\232", nullptr));
        label_18->setText(QCoreApplication::translate("MainWindow", "mV", nullptr));
        label_5->setText(QCoreApplication::translate("MainWindow", "2\357\274\232", nullptr));
        label_6->setText(QCoreApplication::translate("MainWindow", "mV", nullptr));
        label_19->setText(QCoreApplication::translate("MainWindow", "7\357\274\232", nullptr));
        label_20->setText(QCoreApplication::translate("MainWindow", "mV", nullptr));
        label_21->setText(QCoreApplication::translate("MainWindow", "12\357\274\232", nullptr));
        label_22->setText(QCoreApplication::translate("MainWindow", "mV", nullptr));
        label_7->setText(QCoreApplication::translate("MainWindow", "3\357\274\232", nullptr));
        label_8->setText(QCoreApplication::translate("MainWindow", "mV", nullptr));
        label_23->setText(QCoreApplication::translate("MainWindow", "8\357\274\232", nullptr));
        label_24->setText(QCoreApplication::translate("MainWindow", "mV", nullptr));
        label_25->setText(QCoreApplication::translate("MainWindow", "13\357\274\232", nullptr));
        label_26->setText(QCoreApplication::translate("MainWindow", "mV", nullptr));
        label_9->setText(QCoreApplication::translate("MainWindow", "4\357\274\232", nullptr));
        label_10->setText(QCoreApplication::translate("MainWindow", "mV", nullptr));
        label_27->setText(QCoreApplication::translate("MainWindow", "9\357\274\232", nullptr));
        label_28->setText(QCoreApplication::translate("MainWindow", "mV", nullptr));
        label_29->setText(QCoreApplication::translate("MainWindow", "14\357\274\232", nullptr));
        label_30->setText(QCoreApplication::translate("MainWindow", "mV", nullptr));
        label_11->setText(QCoreApplication::translate("MainWindow", "5\357\274\232", nullptr));
        label_12->setText(QCoreApplication::translate("MainWindow", "mV", nullptr));
        label_31->setText(QCoreApplication::translate("MainWindow", "10\357\274\232", nullptr));
        label_32->setText(QCoreApplication::translate("MainWindow", "mV", nullptr));
        label_33->setText(QCoreApplication::translate("MainWindow", "15\357\274\232", nullptr));
        label_34->setText(QCoreApplication::translate("MainWindow", "mV", nullptr));
        label_35->setText(QCoreApplication::translate("MainWindow", "\346\200\273\347\224\265\345\216\213\357\274\232", nullptr));
        label_36->setText(QCoreApplication::translate("MainWindow", "mV", nullptr));
        label_37->setText(QCoreApplication::translate("MainWindow", "\347\224\265\346\265\201\357\274\232", nullptr));
        label_38->setText(QCoreApplication::translate("MainWindow", "mA", nullptr));
        label_39->setText(QCoreApplication::translate("MainWindow", "\346\270\251\345\272\246\357\274\232", nullptr));
        label_40->setText(QCoreApplication::translate("MainWindow", "\302\260C", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("MainWindow", "\350\260\203\350\257\225\344\277\241\346\201\257", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tabData), QCoreApplication::translate("MainWindow", "\346\225\260\346\215\256\347\233\221\346\216\247", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tabChart), QCoreApplication::translate("MainWindow", "\347\224\265\345\216\213\346\233\262\347\272\277", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tabBar), QCoreApplication::translate("MainWindow", "\347\224\265\350\212\257\345\235\207\350\241\241", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tabSettings), QCoreApplication::translate("MainWindow", "\347\263\273\347\273\237\350\256\276\347\275\256", nullptr));
        groupBox->setTitle(QCoreApplication::translate("MainWindow", "BMS\350\277\236\346\216\245", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "\347\253\257\345\217\243\357\274\232", nullptr));
        label_2->setText(QCoreApplication::translate("MainWindow", "\346\263\242\347\211\271\347\216\207\357\274\232", nullptr));
        comboBaud->setItemText(0, QCoreApplication::translate("MainWindow", "115200", nullptr));

        btnConnect->setText(QCoreApplication::translate("MainWindow", "\350\277\236\346\216\245BMS", nullptr));
        groupBox_3->setTitle(QCoreApplication::translate("MainWindow", "\346\214\207\344\273\244\344\270\213\345\217\221", nullptr));
        btnStart->setText(QCoreApplication::translate("MainWindow", "\345\274\200\345\247\213\351\207\207\351\233\206", nullptr));
        btnStop->setText(QCoreApplication::translate("MainWindow", "\345\201\234\346\255\242\351\207\207\351\233\206", nullptr));
        btnChgOn->setText(QCoreApplication::translate("MainWindow", "\346\211\223\345\274\200\345\205\205\347\224\265MOS", nullptr));
        btnChgOff->setText(QCoreApplication::translate("MainWindow", "\345\205\263\351\227\255\345\205\205\347\224\265MOS", nullptr));
        btnDsgOn->setText(QCoreApplication::translate("MainWindow", "\346\211\223\345\274\200\346\224\276\347\224\265MOS", nullptr));
        btnDsgOff->setText(QCoreApplication::translate("MainWindow", "\345\205\263\351\227\255\346\224\276\347\224\265MOS", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
