#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include "csegyread.h"
#include "Zoomer.h"
#include <QString>
#include <QFileInfo>
#include <QModelIndex>
#include <QStringListModel>
#include <QFileSystemModel>
#include <QSystemTrayIcon>
#include <QtGui>
#include <QListWidgetItem>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_magnifier.h>
#include <qwt_plot_panner.h>
#include <QVector>


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


private:
    void closeEvent(QCloseEvent* event); //关闭时右下角隐藏图标


    void updateVolumeInfo();
    void updateTraceHeadInfo();
    void updateDataInfo( const char* szFileName);


    void setParameters();


    int  getCheck();

    void wavePlot(int mode, int component);
    void setGeophones(int geoNum);
    void loadData();
    void clearData();
    void clearListWidget();







private:
    Ui::MainWindow *ui;

    CSegyRead m_segy;


    QString m_strCurrentPath;
    QString m_strCurrentFile;

    QWidget *m_pTabHeader, *m_pTabData;

    int m_nTraceHeaderByteValue;
    int m_nCurrentEndianType;


    int m_nGeophones; //检波器个数
    int m_nTotalTraceNum;
    int m_nTraceLen;
    int m_nCurrentTraceNum;

    int m_nComponent;  // X Y Z分量选择的组合，如果选中X和Y, 则为13+14=27
    int m_nComponentX; // X分量=13
    int m_nComponentY; // Y分量=14
    int m_nComponentZ; // Z分量=12
    int m_nTraceId;   //Y分量=14, X分量=13， Z分量=12
    int m_nDisMode; //显示所有道为0， 以检波器显示为1


    float m_nScale;

    double absMax[100];








    QFileInfo m_fileinfo;
    QFileSystemModel *fileModel ;


    QVector<QVector<double>*> m_Traces; //一个SEGY文件中所有道的数据





private:
    QSystemTrayIcon m_sysTray; //系统托盘图标

    QwtPlotCurve m_pTrace;

    QList<QwtPlotCurve *> m_Curve; //图像曲线

    Zoomer *m_pZoomer;






private slots:

    void OnBtnBrowse();
    void OnSelectNewFile(QListWidgetItem *current, QListWidgetItem *previous);
    void updateEbcdicInfo ();
    void On4ByteValueChecked();
    void On2ByteValueChecked();
    void OnCurrentEdianTypeChanged(int curNum);
    void OnCurrentTraceNumChanged(int curNum);

    void OnGeophoneChecked();
    void OnTracesChecked();
    void OnXSelected();
    void OnYSelected();
    void OnZSelected();
    void updateMicroseis();




    int OnExit();
    int OnBtnToTray();
    int OnSystemTrayClicked(QSystemTrayIcon::ActivationReason reason);

};

#endif // MAINWINDOW_H
