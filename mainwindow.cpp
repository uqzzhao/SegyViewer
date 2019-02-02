#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "util.h"
#include <QString>
#include <QFileDialog>
#include <QTextBrowser>
#include <QPushButton>
#include <QByteArray>
#include <QDir>
#include <QMessageBox>
#include <QFileInfo>
#include <QFileInfoList>
#include <QAbstractItemModel>
#include <QStringListModel>
#include <QComboBox>
#include <QMenu>
#include <QPalette>
#include <qmap.h>
#include <QPen>
#include <QDebug>
#include <qwt_plot_grid.h>
#include <QColor>
#include <qwt_plot_canvas.h>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
  , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //隐藏前2个Tab页，显示Help信息
    m_pTabHeader = ui->tabWidget->widget(0);
    m_pTabData = ui->tabWidget->widget(1);
    ui->tabWidget->removeTab(0);
    ui->tabWidget->removeTab(0);

    //-------------------------------------------系统托盘图标设置
    //设置提示文字
    m_sysTray.setToolTip("DeepListen SEGY Viewer");

    //设置托盘图标
    m_sysTray.setIcon(QIcon(":/resources/logo"));

    //设置托盘菜单
    QMenu* menu = new QMenu();
    menu->addAction(ui->actionOpen);
    menu->addSeparator();

    menu->addAction(ui->actionExit);
    m_sysTray.setContextMenu(menu);

    //显示托盘菜单
    m_sysTray.show();

    connect(ui->actionExit, SIGNAL(triggered(bool)), this, SLOT(OnExit()));
    connect(&m_sysTray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this,SLOT(OnSystemTrayClicked(QSystemTrayIcon::ActivationReason)));
    //---------------------------------------------------------------系统托盘图标设置



    ui->cbxDataType->setUpdatesEnabled(true);//一定要加上这句话，否则无法显示列表
    ui->cbxDataByte->setUpdatesEnabled(true);//一定要加上这句话，否则无法显示列表
    ui->cbxEndianType->setUpdatesEnabled(true);//一定要加上这句话，否则无法显示列表

    //利用QComboBox初始化数据类型
    ui->cbxDataType->addItem("IBM Floating Point","IBM");//currentIndex值从0开始
    ui->cbxDataType->addItem("Fixed Point","Fixed");
    ui->cbxDataType->addItem("IEEE Floating Point","IEEE");


    //利用QComboBox初始化数据字节
    ui->cbxDataByte->addItem("1 Byte","1");//currentIndex值从0开始
    ui->cbxDataByte->addItem("2 Bytes","2");
    ui->cbxDataByte->addItem("4 Bytes","4");


    //利用QComboBox初始化大端/小端
    ui->cbxEndianType->addItem("Big Endian","B");//currentIndex值从0开始
    ui->cbxEndianType->addItem("Little Endian","L");
    connect(ui->cbxEndianType, SIGNAL(currentIndexChanged(int)),
            this, SLOT(OnCurrentEdianTypeChanged(int)));




    //打开一个segy文件
    connect(ui->btnBrowse, SIGNAL(clicked(bool)), this, SLOT(OnBtnBrowse()));
    connect(ui->actionOpen, SIGNAL(triggered(bool)), this, SLOT(OnBtnBrowse()));

    //重新选择列表控件中的SEGY文件
    connect(ui->listSegyFiles, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(OnSelectNewFile(QListWidgetItem*,QListWidgetItem*)));

    //QTabWidget选项卡切换至Waveform页面时，响应
    ui->tabWidget->setCurrentIndex(0);


    //利用QCheckBox控件判断是否是EBCDIC/ASCII格式，如果乱码，则为ASCII格式,默认显示ASCII格式
    ui->chbxEbcdicHead->setCheckState(Qt::Checked);
    connect(ui->chbxEbcdicHead, SIGNAL(stateChanged(int)), this, SLOT(updateEbcdicInfo()));

    //单选框radiobutton改变
    connect(ui->rbtn4Byte, SIGNAL(clicked(bool)), this, SLOT(On4ByteValueChecked()));
    connect(ui->rbtn2Byte, SIGNAL(clicked(bool)), this, SLOT(On2ByteValueChecked()));

    //SpinBox道号改变
    connect(ui->spinCurrentTraceNo, SIGNAL(valueChanged(int)),this, SLOT(OnCurrentTraceNumChanged(int)));

    //微地震波形显示模式及分量选择
    ui->rbtTrace->setChecked(true);
    ui->rbtGeophone->setChecked(false);
    ui->cbxX->setEnabled(false);
    ui->cbxY->setEnabled(false);
    ui->cbxZ->setEnabled(false);

    connect(ui->rbtGeophone, SIGNAL(clicked(bool)), this, SLOT(OnGeophoneChecked()));
    connect(ui->rbtTrace, SIGNAL(clicked(bool)), this, SLOT(OnTracesChecked()));
    connect(ui->cbxX,  SIGNAL(toggled(bool)), this, SLOT(OnXSelected()));
    connect(ui->cbxY,  SIGNAL(toggled(bool)), this, SLOT(OnYSelected()));
    connect(ui->cbxZ,  SIGNAL(toggled(bool)), this, SLOT(OnZSelected()));



    //最小化到托盘
    connect(ui->btnToTray, SIGNAL(clicked(bool)), this, SLOT(OnBtnToTray()));



    //-------------------------微震三分量数据绘图

    ui->Plot3C->enableAxis(QwtPlot::yLeft);
    ui->Plot3C->enableAxis(QwtPlot::yRight, false);
    ui->Plot3C->enableAxis(QwtPlot::xTop, false);
    ui->Plot3C->enableAxis(QwtPlot::xBottom);

    ui->Plot3C->setAxisTitle(ui->Plot3C->yLeft, tr("Geophones"));
    ui->Plot3C->setAxisScale(ui->Plot3C->yLeft, 0,13);
    ui->Plot3C->setAxisTitle(ui->Plot3C->xBottom, tr("Samples"));
    ui->Plot3C->setAxisScale(ui->Plot3C->xBottom, 0,1000);

    ui->Plot3C->setLineWidth(1);
    ui->Plot3C->setFrameStyle(QFrame::Box| QFrame::Plain);


    m_nCurrentEndianType = 0;
    m_nCurrentTraceNum = 1;
    m_nGeophones =12; //检波器个数
    m_nTotalTraceNum=100;
    m_nTraceLen=100;
    absMax[100]={0};


    m_nTraceHeaderByteValue = 4;
    m_nComponent=14;
    m_nScale=1.0;
    m_nDisMode =0;



}




MainWindow::~MainWindow()
{

    delete ui;
    m_segy.closeFile();


}

void MainWindow::closeEvent(QCloseEvent *event)
{
    this->hide();
    event->ignore();
}


void MainWindow::OnBtnBrowse()
{

    this->show();

    m_strCurrentFile = QFileDialog::getOpenFileName(
                this,
                tr("Select one SEG-Y file"),
                tr(""),
                tr("SEG-Y Files(*.sgy *.segy)")
                );

    if ( m_strCurrentFile.length()>0 )
    {

        ui->Plot3C->detachItems();
        ui->Plot3C->replot();

        clearData();

        ui->listSegyFiles->blockSignals(true);
        ui->listSegyFiles->clear(); //重新打开SEGY文件时要先清空，否则多次重复打开同一目录，文件重复显示
        ui->listSegyFiles->blockSignals(false);

        //clearListWidget();

        //插入2个Tab页，并切换到Header页面
        ui->tabWidget->insertTab(0, m_pTabHeader, tr("Header"));
        ui->tabWidget->insertTab(1, m_pTabData, tr("Waveform"));
        ui->tabWidget->setCurrentIndex(0); //切换至Header页面

        ui->edtFilePath->setText(m_strCurrentFile);



        QDir dir = QFileInfo(m_strCurrentFile).absoluteDir();//获得当前目录
        m_strCurrentPath = dir.absolutePath();    //获得当前目录绝对路径

        QDirIterator dirIterator(dir);
        while (dirIterator.hasNext())
        {
            dirIterator.next();
            QFileInfo info = dirIterator.fileInfo();
            if (info.isFile())
            {
                if (info.suffix() == tr("segy") || info.suffix() == tr("sgy"))
                {
                    QListWidgetItem *item = new QListWidgetItem;
                    item->setText(info.fileName());
                    ui->listSegyFiles->addItem(item);

                    //加上下面这两行代码，将指定文件（不一定是第一个）设为当前项，否则显示列表中第一个SEGY文件，
                    if (info.absoluteFilePath()==m_strCurrentFile)
                        ui->listSegyFiles->setCurrentItem(item);


                }
            }


        }

        char szFilePath[2000];
        memset(szFilePath,0,2000);
        QStringData *data = m_strCurrentFile.data_ptr();
        ushort *s = data->data();

        for (int i = 0; i<data->size; i++ )
        {
            szFilePath[i] = s[i];
        }

        ui->rbtn4Byte->setChecked(true);
        ui->spinCurrentTraceNo->setValue(1);



        updateDataInfo(szFilePath);

    }


}




void MainWindow::updateDataInfo( const char *szFileName)
{

    if(!m_segy.OpenFile(szFileName))
    {
        QMessageBox::information(this, tr("Open SEGY File"), tr("SEGY file open failed!"));
        return;
    }


    updateEbcdicInfo();
    updateVolumeInfo();
    updateTraceHeadInfo();


    setParameters();

    loadData();
    updateMicroseis();



}

void MainWindow::setParameters()
{

    m_nTotalTraceNum=m_segy.getTotalTraceNumber();
    m_nTraceLen = m_segy.getSamplesNumber();
    m_nGeophones = m_nTotalTraceNum/3;


}

int MainWindow::getCheck()
{
    int x, y, z, sum;
    if (ui->cbxX->checkState()== Qt::Checked)
        x = 13;
    else
        x = 0;
    if (ui->cbxY->checkState()== Qt::Checked)
        y = 14;
    else
        y = 0;
    if (ui->cbxZ->checkState()== Qt::Checked)
        z = 12;
    else
        z = 0;

    sum = x+y+z;
    return sum;
}

void MainWindow::updateEbcdicInfo()
{

    //在QTextEdit控件中显示3200字节文本卷头信息
    QString ebcdicHeader;
    char sz[3202]= "\0";
    const unsigned char* ebcdicText =  m_segy.GetEbcdic();

    if (ui->chbxEbcdicHead->checkState() == Qt::Checked)
    {
        ui->chbxEbcdicHead->setText("EBCDIC Header");
        for (int i = 0; i<3200; i++)
        {
            sz[i] = EbcdicToAscii(ebcdicText[i]);
            ebcdicHeader.append(sz);
        }

        ebcdicHeader.sprintf("%s", sz);

    }

    else
    {

        ui->chbxEbcdicHead->setText("ASCII Header");
        ebcdicHeader.sprintf("%s", ebcdicText);

    }

    ui->txtTextHeader->setText(ebcdicHeader);



}

void MainWindow::updateVolumeInfo()
{
    //利用LineEdit控件显示卷头重要信息
    ui->edtSamples->setText(QString("%1").arg(m_segy.getSamplesNumber())); //显示采样总数；

    float sampleRate = float(m_segy.getSamplesInterval())/1000;
    QString s;
    s.sprintf("%.2f", sampleRate);
    ui->edtSampleRate->setText(s); //显示采样率

    ui->edtFormatCode->setText(QString("%1").arg(m_segy.getFormat())); //显示数据类型
    ui->edtTraces->setText(QString("%1").arg(m_segy.getTotalTraceNumber())); //显示总道数
    ui->edtSegyRev->setText(QString("%1").arg(m_segy.getSegyRev())); //显示SEGY版本号


    //利用ComboBox显示数据格式+字节信息

    switch(m_segy.getFormat())
    {
    case 1:
    {
        ui->cbxDataType->setCurrentIndex(0); //QComboBox的默认Index是从0开始的，不是从-1开始！！！
        ui->cbxDataByte->setCurrentIndex(2);
        break;
    }

    case 2:
    {
        ui->cbxDataType->setCurrentIndex(1);
        ui->cbxDataByte->setCurrentIndex(2);
        break;
    }
    case 3:
    {
        ui->cbxDataType->setCurrentIndex(1);
        ui->cbxDataByte->setCurrentIndex(1);
        break;
    }
    case 5:
    {
        ui->cbxDataType->setCurrentIndex(2);
        ui->cbxDataByte->setCurrentIndex(2);
        break;
    }
    case 8:
    {
        ui->cbxDataType->setCurrentIndex(1);
        ui->cbxDataByte->setCurrentIndex(0);
        break;
    }


    }

}



void MainWindow::On4ByteValueChecked()
{
    m_nTraceHeaderByteValue = 4;
    updateTraceHeadInfo();

}

void MainWindow::On2ByteValueChecked()
{

    m_nTraceHeaderByteValue = 2;
    updateTraceHeadInfo();


}

void MainWindow::OnCurrentEdianTypeChanged(int curNum)
{
    //m_nCurrentEndianm_nComponent = curNum;
}

void MainWindow::OnCurrentTraceNumChanged(int curNum)
{
    if (curNum > m_segy.getTotalTraceNumber())
    {
        QMessageBox::information(this, tr("Error"),
                                 QString("Total trace number is:\n    %1").arg(m_segy.getTotalTraceNumber()));
    }
    m_nCurrentTraceNum = curNum;



    updateTraceHeadInfo();




}





void MainWindow::OnGeophoneChecked()
{


    m_nDisMode = 1;

    ui->rbtTrace->setChecked(false);
    ui->cbxX->setEnabled(true);
    ui->cbxY->setEnabled(true);
    ui->cbxZ->setEnabled(true);
    ui->cbxZ->setChecked(true);
    updateMicroseis();

}

void MainWindow::OnTracesChecked()
{
    m_nDisMode = 0;

    ui->rbtGeophone->setChecked(false);
    ui->cbxX->setEnabled(false);
    ui->cbxY->setEnabled(false);
    ui->cbxZ->setEnabled(false);
    updateMicroseis();
}

void MainWindow::OnXSelected()
{

    if (m_nTraceId == 12)
    {

        m_nComponentZ =12;
        m_nComponentX =13;
        m_nComponentY =14;
    }
    else
    {
        m_nComponentX =12;
        m_nComponentY =13;
        m_nComponentZ =14;
    }




    updateMicroseis();


}

void MainWindow::OnYSelected()
{
    if (m_nTraceId == 12)
    {

        m_nComponentZ =12;
        m_nComponentX =13;
        m_nComponentY =14;
    }
    else
    {
        m_nComponentX =12;
        m_nComponentY =13;
        m_nComponentZ =14;
    }



    updateMicroseis();

}


void MainWindow::OnZSelected()
{
    if (m_nTraceId == 12)
    {

        m_nComponentZ =12;
        m_nComponentX =13;
        m_nComponentY =14;
    }
    else
    {
        m_nComponentX =12;
        m_nComponentY =13;
        m_nComponentZ =14;
    }


    updateMicroseis();

}



void MainWindow::updateTraceHeadInfo()
{

    while (ui->tableTraceHeader->rowCount())
    {
        ui->tableTraceHeader->removeRow(0);
    }


    TRACEHEADER theader;
    theader = m_segy.ReadTraceHeader(m_nCurrentTraceNum);
    m_nTraceId = theader.nTraId;
    ui->edtTraceID->setText(QString("%1").arg(m_nTraceId));
    const char* szHead = m_segy.GetTraceHeadAsBytes();


    int numRow = 240 / m_nTraceHeaderByteValue;

    for (int i=0;i<numRow;i++)
    {
        ui->tableTraceHeader->insertRow(i);
        QString str;
        str.sprintf("%d-%d",i*m_nTraceHeaderByteValue+1,i*m_nTraceHeaderByteValue+m_nTraceHeaderByteValue);
        ui->tableTraceHeader->setItem(i,0,new QTableWidgetItem(str));

        int itemValue = 0;
        memcpy((char*)&itemValue,szHead+i*m_nTraceHeaderByteValue,m_nTraceHeaderByteValue);
        if (ui->cbxEndianType->currentIndex() == 0) //big endian
            itemValue = swap(itemValue,m_nTraceHeaderByteValue);
        str.sprintf("%d",itemValue);
        ui->tableTraceHeader->setItem(i,1,new QTableWidgetItem(str));
        ui->tableTraceHeader->setItem(i,2,new QTableWidgetItem(str));
    }


}

void MainWindow::updateMicroseis()
{

    ui->Plot3C->detachItems();

    m_Curve.clear();

    setGeophones(m_nGeophones);

    m_nComponent = getCheck();

    wavePlot(m_nDisMode, m_nComponent);




}



void MainWindow::wavePlot(int mode, int component)
{

    int rectY; //放大缩小矩形框纵坐标大小
    int i,k;
    double j=0.0;
    double m_TempValue;
    QVector<double> m_TempData;
    QVector<double> vectorX3c;

    //for(i=0;i<m_Curve.count();i++)
    //m_Curve[i]->setVisible(false);


    vectorX3c.clear();

    for (int i=0; i<m_nTraceLen; i++)
    {
        vectorX3c<<j;
        j+=1.0;

    }

    switch (mode) {
    case 0:
        ui->Plot3C->setAxisTitle(ui->Plot3C->yLeft, tr("Traces"));
        rectY = m_nTotalTraceNum + 1;
        ui->Plot3C->setAxisScale(ui->Plot3C->yLeft, 0,rectY);
        ui->Plot3C->setTitle("Waveform of All Traces");


        for(i=0;i<m_nGeophones*3;i++)
        {
            m_TempData.clear();

            for(k=0;k<m_Traces[i]->count();k++)
            {
                m_TempValue=(*m_Traces[i])[k];
                m_TempValue=m_nScale*0.5*m_TempValue;  //显示波形的幅度
                m_TempValue=1+i+m_TempValue;
                m_TempData<<m_TempValue;
            }
            m_Curve[i]->setSamples(vectorX3c,m_TempData);
            m_Curve[i]->setVisible(true);

        }

        break;

    case 1:
        ui->Plot3C->setAxisTitle(ui->Plot3C->yLeft, tr("Geophones"));
        rectY = m_nGeophones+1;
        ui->Plot3C->setAxisScale(ui->Plot3C->yLeft, 0,rectY);


        if (component == 12)
        {
            ui->Plot3C->setTitle("Z-Component Waveform");
            for(i=0;i<m_nGeophones*3;i++)
            {
                m_TempData.clear();
                if(i%3==m_nComponentZ-12 && (m_nComponentZ-12)!=3)
                {
                    for(k=0;k<m_Traces[i]->count();k++)
                    {
                        m_TempValue=(*m_Traces[i])[k];
                        m_TempValue=m_nScale*0.5*m_TempValue;  //显示波形的幅度
                        m_TempValue=1+i/3+m_TempValue;
                        m_TempData<<m_TempValue;
                    }

                    m_Curve[i]->setSamples(vectorX3c,m_TempData);
                    m_Curve[i]->setVisible(true);
                }
            }

        }
        else if (component == 13)
        {
            ui->Plot3C->setTitle("X-Component Waveform");
            for(i=0;i<m_nGeophones*3;i++)
            {
                m_TempData.clear();
                if(i%3==m_nComponentX-12 && (m_nComponentX-12)!=3)
                {
                    for(k=0;k<m_Traces[i]->count();k++)
                    {
                        m_TempValue=(*m_Traces[i])[k];
                        m_TempValue=m_nScale*0.5*m_TempValue;  //显示波形的幅度
                        m_TempValue=1+i/3+m_TempValue;
                        m_TempData<<m_TempValue;
                    }

                    m_Curve[i]->setSamples(vectorX3c,m_TempData);
                    m_Curve[i]->setVisible(true);
                }
            }
        }
        else if (component == 14)
        {
            ui->Plot3C->setTitle("Y-Component Waveform");
            for(i=0;i<m_nGeophones*3;i++)
            {
                m_TempData.clear();
                if(i%3==m_nComponentY-12 && (m_nComponentY-12)!=3)
                {
                    for(k=0;k<m_Traces[i]->count();k++)
                    {
                        m_TempValue=(*m_Traces[i])[k];
                        m_TempValue=m_nScale*0.5*m_TempValue;  //显示波形的幅度
                        m_TempValue=1+i/3+m_TempValue;
                        m_TempData<<m_TempValue;
                    }

                    m_Curve[i]->setSamples(vectorX3c,m_TempData);
                    m_Curve[i]->setVisible(true);
                }
            }
        }
        else if (component == 25)
        {
            ui->Plot3C->setTitle("X & Z Components Waveform");
            for(i=0;i<m_nGeophones*3;i++)
            {
                m_TempData.clear();
                if(i%3==m_nComponentX-12 && (m_nComponentX-12)!=3)
                {
                    for(k=0;k<m_Traces[i]->count();k++)
                    {
                        m_TempValue=(*m_Traces[i])[k];
                        m_TempValue=m_nScale*0.5*m_TempValue;  //显示波形的幅度
                        m_TempValue=1+i/3+m_TempValue;
                        m_TempData<<m_TempValue;
                    }

                    m_Curve[i]->setSamples(vectorX3c,m_TempData);
                    m_Curve[i]->setVisible(true);
                }
            }

            for(i=0;i<m_nGeophones*3;i++)
            {
                m_TempData.clear();
                if(i%3==m_nComponentZ-12 && (m_nComponentZ-12)!=3)
                {
                    for(k=0;k<m_Traces[i]->count();k++)
                    {
                        m_TempValue=(*m_Traces[i])[k];
                        m_TempValue=m_nScale*0.5*m_TempValue;  //显示波形的幅度
                        m_TempValue=1+i/3+m_TempValue;
                        m_TempData<<m_TempValue;
                    }

                    m_Curve[i]->setSamples(vectorX3c,m_TempData);
                    m_Curve[i]->setVisible(true);
                }
            }
        }

        else if (component == 26)
        {
            ui->Plot3C->setTitle("Y & Z Components Waveform");
            for(i=0;i<m_nGeophones*3;i++)
            {
                m_TempData.clear();
                if(i%3==m_nComponentY-12 && (m_nComponentY-12)!=3)
                {
                    for(k=0;k<m_Traces[i]->count();k++)
                    {
                        m_TempValue=(*m_Traces[i])[k];
                        m_TempValue=m_nScale*0.5*m_TempValue;  //显示波形的幅度
                        m_TempValue=1+i/3+m_TempValue;
                        m_TempData<<m_TempValue;
                    }

                    m_Curve[i]->setSamples(vectorX3c,m_TempData);
                    m_Curve[i]->setVisible(true);
                }
            }
            for(i=0;i<m_nGeophones*3;i++)
            {
                m_TempData.clear();
                if(i%3==m_nComponentZ-12 && (m_nComponentZ-12)!=3)
                {
                    for(k=0;k<m_Traces[i]->count();k++)
                    {
                        m_TempValue=(*m_Traces[i])[k];
                        m_TempValue=m_nScale*0.5*m_TempValue;  //显示波形的幅度
                        m_TempValue=1+i/3+m_TempValue;
                        m_TempData<<m_TempValue;
                    }

                    m_Curve[i]->setSamples(vectorX3c,m_TempData);
                    m_Curve[i]->setVisible(true);
                }
            }
        }

        else if (component == 27)
        {
            ui->Plot3C->setTitle("X & Y Components Waveform");
            for(i=0;i<m_nGeophones*3;i++)
            {
                m_TempData.clear();
                if(i%3==m_nComponentX-12 && (m_nComponentX-12)!=3)
                {
                    for(k=0;k<m_Traces[i]->count();k++)
                    {
                        m_TempValue=(*m_Traces[i])[k];
                        m_TempValue=m_nScale*0.5*m_TempValue;  //显示波形的幅度
                        m_TempValue=1+i/3+m_TempValue;
                        m_TempData<<m_TempValue;
                    }

                    m_Curve[i]->setSamples(vectorX3c,m_TempData);
                    m_Curve[i]->setVisible(true);
                }
            }
            for(i=0;i<m_nGeophones*3;i++)
            {
                m_TempData.clear();
                if(i%3==m_nComponentY-12 && (m_nComponentY-12)!=3)
                {
                    for(k=0;k<m_Traces[i]->count();k++)
                    {
                        m_TempValue=(*m_Traces[i])[k];
                        m_TempValue=m_nScale*0.5*m_TempValue;  //显示波形的幅度
                        m_TempValue=1+i/3+m_TempValue;
                        m_TempData<<m_TempValue;
                    }

                    m_Curve[i]->setSamples(vectorX3c,m_TempData);
                    m_Curve[i]->setVisible(true);
                }
            }
        }
        else if (component == 39)
        {
            ui->Plot3C->setTitle("3 Components Waveform");
            for(i=0;i<m_nGeophones*3;i++)
            {
                m_TempData.clear();
                for(k=0;k<m_Traces[i]->count();k++)
                {
                    m_TempValue=(*m_Traces[i])[k];
                    m_TempValue=m_nScale*0.5*m_TempValue;  //显示波形的幅度
                    m_TempValue=1+i/3+m_TempValue;
                    m_TempData<<m_TempValue;
                }
                m_Curve[i]->setSamples(vectorX3c,m_TempData);
                m_Curve[i]->setVisible(true);
            }
        }
        else
        {
            ui->Plot3C->setTitle("None Component Selected");
            for(i=0;i<m_nGeophones*3;i++)
            {
                m_TempData.clear();
                for(k=0;k<m_Traces[i]->count();k++)
                {
                    m_TempValue=(*m_Traces[i])[k];
                    m_TempValue=m_nScale*0.5*m_TempValue;  //显示波形的幅度
                    m_TempValue=1+i/3+m_TempValue;
                    m_TempData<<m_TempValue;
                }
                m_Curve[i]->setSamples(vectorX3c,m_TempData);
                m_Curve[i]->setVisible(false);
            }
        }
        break;

    default:
        break;
    }

    ui->Plot3C->setAxisTitle(ui->Plot3C->xBottom, tr("Samples"));
    ui->Plot3C->setAxisScale(ui->Plot3C->xBottom, 0,m_nTraceLen);



    //加入网格
    QwtPlotGrid *grid=new QwtPlotGrid();
    grid->enableXMin(true);
    grid->setMajorPen(QPen(Qt::lightGray,0,Qt::DotLine));
    grid->setMinorPen(QPen(Qt::lightGray,0,Qt::DotLine));
    grid->attach(ui->Plot3C);




    //加入放大缩小所选择矩形区域
    QRectF rect(0,0,m_nTraceLen,rectY);

    //设置放大缩小zoomer功能
    QwtPlotCanvas* traceCanvas = new QwtPlotCanvas();
    ui->Plot3C->setCanvas(traceCanvas);
    m_pZoomer = new Zoomer(traceCanvas,rect);

    m_pZoomer->setRect(rect);
    m_pZoomer->setZoomBase(rect);
    m_pZoomer->setMousePattern(QwtEventPattern::MouseSelect2,
                               Qt::RightButton,Qt::ControlModifier);
    m_pZoomer->setMousePattern(QwtEventPattern::MouseSelect3,
                               Qt::RightButton);
    m_pZoomer->setRubberBandPen(QPen(Qt::red, 2, Qt::DotLine));
    m_pZoomer->setTrackerMode(QwtPicker::AlwaysOff);
    m_pZoomer->setEnabled(true);



    ui->Plot3C->replot();
}

void MainWindow::setGeophones(int geoNum)
{
    QwtPlotCurve* NewCurve;
    int i;

    while(!m_Curve.isEmpty())
        delete m_Curve.takeFirst();


    ui->Plot3C->setAxisScale(ui->Plot3C->yLeft,0,(double)(geoNum+1));
    ui->Plot3C->setAxisScale(ui->Plot3C->xBottom, 0, (double)m_nTraceLen);

    QPen greenPen;
    //greenPen.setColor(QColor(0,100,0)); //深绿色
    greenPen.setColor(QColor(34,139,34));  //森林绿


    for(i=0;i<geoNum*3;i++)
    {
        NewCurve=new QwtPlotCurve;
        NewCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        if(i%3==0)
            NewCurve->setPen(QPen(Qt::red));
        else if(i%3==1)
            NewCurve->setPen(QPen(Qt::blue));
        else if(i%3==2)
            NewCurve->setPen(greenPen);


        NewCurve->attach(ui->Plot3C);
        NewCurve->setVisible(false);
        m_Curve<<NewCurve;
    }

    ui->Plot3C->replot();
}

void MainWindow::loadData()
{

    QVector<double>* m_ChangeData;
    double sampleValue= 0.0;
    QVector <double> tempVector;
    float* pTrace[m_nTotalTraceNum];

    for (int i=0; i<m_nTotalTraceNum; i++)
    {

        m_ChangeData=new QVector<double>;
        m_ChangeData->clear();
        pTrace[i] = m_segy.GetTraceData(i+1);


        tempVector.clear();

        for (int j=0; j<m_nTraceLen; j++)
        {
            m_ChangeData->append(sampleValue);

        }


        for (int k=0; k<m_nTraceLen; k++)
        {
            tempVector<<(double)(pTrace[i])[k];

        }
        //地震道单道归一化，每个采样点振幅值除以该道最大振幅值的绝对值
        double absMin=0;
        GetVectorMax(tempVector, absMax[i]);

        for (int k=0; k<m_nTraceLen; k++)
        {

            tempVector[k] = ( tempVector[k]-absMin)/(absMax[i]-absMin);

            m_ChangeData->replace (k, tempVector[k]);

        }

        m_Traces<<m_ChangeData;

    }


}

void MainWindow::clearData()
{
    int i,j;
    if (!m_Traces.isEmpty())
    {
        i=m_Traces.count();
        for(j=i-1;j>=0;j--)
            m_Traces[j]->clear();
        m_Traces.clear();
    }




}

void MainWindow::clearListWidget()
{

    // clear listwidget items
    while(ui->listSegyFiles->count()>0)
    {
      ui->listSegyFiles->takeItem(0);//handle the item if you don't
                              //have a pointer to it elsewhere
    }

//    int counter =ui->listSegyFiles->count();
//    for(int index=0;index<counter;index++)
//    {
//        QListWidgetItem *item = ui->listSegyFiles->takeItem(0);
//        delete item;
//    }


}



void MainWindow::OnSelectNewFile(QListWidgetItem *current, QListWidgetItem *previous)
{
    char szFilePath[200];
    memset(szFilePath,0,200);

    QString strFilePath = m_strCurrentPath + "/"+ current->text();
    QStringData *data = strFilePath.data_ptr();
    ushort *s = data->data();
    for (int i = 0; i<data->size; i++ )
    {
        szFilePath[i] = s[i];
    }

    ui->edtFilePath->setText(strFilePath);
    ui->Plot3C->detachItems();
    ui->Plot3C->replot();

    clearData();

    updateDataInfo(szFilePath);


}







int MainWindow::OnExit()
{
    QApplication::exit(0);
    return 0;
}

int MainWindow::OnBtnToTray()
{
    this->hide();

}

int MainWindow::OnSystemTrayClicked(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger
            || reason == QSystemTrayIcon::DoubleClick)
    {
        this->showNormal();
    }
    return 0;
}
