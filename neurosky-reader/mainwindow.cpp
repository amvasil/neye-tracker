#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <qwt_plot.h>
#include <qwt_plot_grid.h>

#include "thinkgear.h"
#include <QMessageBox>
#include <QDebug>

#include <windows.h>

#include <qwt_plot_zoomer.h>

#include "fftw/fftw3.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //init plot
    ui->plot->setCanvasBackground(QBrush(Qt::white));
    QwtPlotGrid *grid = new QwtPlotGrid();
    grid->setMajorPen(QPen(QColor(230,230,230)));
    grid->setMinorPen(QPen(QColor(240,240,240)));
    grid->attach(ui->plot);
    grid->enableXMin(1);
    grid->enableYMin(1);

    QwtText title("EEG Plot");
    title.setFont(QFont("Segoe UI",10,0.5));
    ui->plot->setAxisTitle(2,title);

    ui->fft_plot->setCanvasBackground(QBrush(Qt::white));
    QwtPlotGrid *grid2 = new QwtPlotGrid();
    grid2->setMajorPen(QPen(QColor(230,230,230)));
    grid2->setMinorPen(QPen(QColor(240,240,240)));
    grid2->attach(ui->fft_plot);
    grid2->enableXMin(1);
    grid2->enableYMin(1);

    QwtText fft_title("EEG Spectrum");
    fft_title.setFont(QFont("Segoe UI",10,0.5));
    ui->fft_plot->setAxisTitle(2,fft_title);


    //init connection id as invalid
    connectionId = -1;
    //set polling timer period
    readTimer.setInterval(1);
    readTimer.setSingleShot(false);
    connect(&readTimer,SIGNAL(timeout()),this,SLOT(slot_read_timer()));
    //set replot timer
    replotTimer.setInterval(50);
    replotTimer.setSingleShot(false);
    connect(&replotTimer,SIGNAL(timeout()),this,SLOT(slot_replot_timer()));
    //create plot curve
    curve = new QwtPlotCurve();
    curve->attach(ui->plot);
    memset(errstr,0,200);

    curve_fft = new QwtPlotCurve();
    curve_fft->attach(ui->fft_plot);

    //set y axis limits
    ui->plot->setAxisScale(0,-2000,2000);
    ui->plot->setAxisAutoScale(0,0);
    zoom = new QwtPlotZoomer(ui->plot->canvas());

    //ui->fft_plot->setAxisScale(0,0,20000);
    ui->fft_plot->setAxisAutoScale(0,1);
    ui->fft_plot->setAxisScale(2,0,50);
    ui->fft_plot->setAxisAutoScale(2,0);


    fft_worker = new FftWorker(0);
    connect(this,SIGNAL(compute_fft()),fft_worker,SLOT(on_compute_requested()));
    connect(fft_worker,SIGNAL(compute_complete()),this,SLOT(on_compute_fft_complete()));
    is_computing_fft = 0;

    fft_worker->moveToThread(&fft_thread);
    fft_thread.start();

    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    qDebug()<<"QPC freq is"<<(quint64)(freq.QuadPart);
    qpc_freq = (double)(freq.QuadPart);
    prev_ts = getTimestamp();

    is_saving = 0;

    max_line = 65535;
    min_line = 0;
    lim_x_data.append(0);
    lim_x_data.append(0);

    max_line_curve = new QwtPlotCurve();
    max_line_data.append(max_line);
    max_line_data.append(max_line);
    max_line_curve->setSamples(lim_x_data,max_line_data);
    max_line_curve->setPen(Qt::cyan,2.0,Qt::DashLine);
    //max_line_curve->attach(ui->plot);

    min_line_curve = new QwtPlotCurve();
    min_line_data.append(min_line);
    min_line_data.append(min_line);
    min_line_curve->setSamples(lim_x_data,min_line_data);
    min_line_curve->setPen(Qt::cyan,2.0,Qt::DashLine);
    to_compute_limits = 0;
}

MainWindow::~MainWindow()
{
    if(connectionId >= 0)
        TG_FreeConnection( connectionId );
    fft_thread.terminate();
    fft_worker->deleteLater();
    delete ui;
}

void MainWindow::error(QString str)
{
    qDebug()<<str;
    QMessageBox::critical(this,"Error",str);

}

//Connect to mindwave if not connected or
//disconnect if connected
//Connection is performed via virtual COM-port over bluetooth
//Port is usually COM12
void MainWindow::on_pushButton_clicked()
{
    if(connectionId < 0)
    {
        qDebug()<<"connecting";
        ui->statusLabel->setText("Connecting");
        //connect
        const char *com_name = "\\\\.\\COM6";
        int   dllVersion = 0;
        int   errCode = 0;
        /* Print driver version number */
        dllVersion = TG_GetDriverVersion();
        printf( "ThinkGear DLL version: %d\n", dllVersion );

        /* Get a connection ID handle to ThinkGear */
        connectionId = TG_GetNewConnectionId();
        if( connectionId < 0 ) {
            sprintf( errstr, "ERROR: TG_GetNewConnectionId() returned %d.\n",
                     connectionId );
            error(errstr);
            exit( EXIT_FAILURE );
        }

        /* Set/open stream (raw bytes) log file for connection */
        errCode = TG_SetStreamLog( connectionId, "streamLog.txt" );
        if( errCode < 0 ) {
            sprintf( errstr, "ERROR: TG_SetStreamLog() returned %d.\n", errCode );
            error(errstr);
            exit( EXIT_FAILURE );
        }

        /* Set/open data (ThinkGear values) log file for connection */
        errCode = TG_SetDataLog( connectionId, "dataLog.txt" );
        if( errCode < 0 ) {
            sprintf( errstr, "ERROR: TG_SetDataLog() returned %d.\n", errCode );
            error(errstr);
            exit( EXIT_FAILURE );
        }

        /* Attempt to connect the connection ID handle to serial port "COM5" */

        errCode = TG_Connect( connectionId,
                              com_name,
                              TG_BAUD_57600,
                              TG_STREAM_PACKETS );
        if( errCode < 0 ) {
            sprintf( errstr, "ERROR: TG_Connect() returned %d.\n", errCode );
            error(errstr);
            exit( EXIT_FAILURE );
        }
        qDebug()<<"connected";
        ui->statusLabel->setText("Connected");
        readTimer.start();
        replotTimer.start();
        ui->pushButton->setText("Disconnect COM12");

    }
    else
    {
        //disconnect
        readTimer.stop();
        replotTimer.stop();
        ui->statusLabel->setText("Disconnected");
        /* Clean up */
        TG_FreeConnection( connectionId );
        connectionId = -1;
        ui->pushButton->setText("Connect COM12");
        ui->qualityBar->setValue(0);
    }
}

//This is called on timer to read data from COMport
void MainWindow::slot_read_timer()
{
    if(connectionId<0)
        return;
    /* Attempt to read a Packet of data from the connection */
    int errCode = TG_ReadPackets( connectionId, 1 );
    /* If TG_ReadPackets() was able to read a complete Packet of data... */
    if( errCode == 1 ) {
        if( TG_GetValueStatus(connectionId, TG_DATA_RAW ) != 0 ) {
            float res = TG_GetValue(connectionId, TG_DATA_RAW ) ;
            ui->periodSpin->setValue(getTimeIntervalMs());
            //printf("Raw data read, res = %f\n",res);
            if(data_x.isEmpty())
                data_x.append(0);
            else
                data_x.append(data_x.last()+1.0/512.0);
            data_y.append(res);
            if(is_saving)
                saved_data.append(res);
            //sine wave imitation
            //data_y.append(sin(data_x.last()*2*3.1415*50.0)*10);
            while(data_y.size() > ui->samplesSpin->value())
            {
                data_y.removeFirst();
                data_x.removeFirst();
            }



            if(data_x.size() > FftWorker::buffer_size && !is_computing_fft)
            {
                //update limit lines
                lim_x_data[0] = data_x.first();
                lim_x_data[1] = data_x.last();
                max_line_curve->setSamples(lim_x_data,max_line_data);
                min_line_curve->setSamples(lim_x_data,min_line_data);

                QVector<double> dy = data_y.mid(data_y.size()-FftWorker::buffer_size,-1);

                if(to_compute_limits)
                    update_limits(dy);

                auto mm = std::minmax_element(dy.begin(),dy.end());
                if(is_saving)
                {
                    if((*mm.first) >= min_line && (*mm.second) <= max_line )
                        start_fft(dy);
                }
                else
                {
                    start_fft(dy);
                }

            }

        }

        if( TG_GetValueStatus(connectionId, TG_DATA_POOR_SIGNAL ) != 0  )
        {
           // qDebug()<<"poor signal "<<TG_GetValue(connectionId, TG_DATA_POOR_SIGNAL );
            ui->qualityBar->setValue(200-TG_GetValue(connectionId, TG_DATA_POOR_SIGNAL ));
        }

        ui->errSpin->setValue(ui->errSpin->value()-1);
    }
    else{
        ui->errSpin->setValue(ui->errSpin->value()+1);
    }
}

void MainWindow::start_fft(QVector<double>& data)
{
    is_computing_fft = 1;
    fft_worker->setData(data);
    emit compute_fft();
}

void MainWindow::slot_replot_timer()
{
    if(data_x.isEmpty())
        return;
    curve->setSamples(data_x,data_y);
    int minxi = qMax(data_x.size()-ui->samplesSpin->value(),0);
    double minx = data_x.at(minxi);
    zoom->setZoomBase(QRect(minx,-2000,minx+ui->samplesSpin->value()/512.0,4000));
    ui->plot->replot();
    ui->fft_plot->replot();

}

double MainWindow::getTimestamp()
{
    LARGE_INTEGER val;
    QueryPerformanceCounter(&val);
    return val.QuadPart/qpc_freq;

}

double MainWindow::getTimeIntervalMs()
{
    double res = (getTimestamp() - prev_ts)*1000;
    prev_ts = getTimestamp();
    return res;
}

void MainWindow::update_limits(QVector<double> &data)
{
    to_compute_limits = 0;
    auto mm = std::minmax_element(data.begin(),data.end());
    double d = (*mm.second)-(*mm.first);
    min_line = *mm.first-d/2;
    max_line = *mm.second+d/2;
    max_line_data.fill(max_line);
    min_line_data.fill(min_line);
    min_line_curve->attach(ui->plot);
    max_line_curve->attach(ui->plot);
    qDebug()<<"limits updated";

    fft_accum.fill(0,FftWorker::buffer_size/2);
    fft_accum_count.fill(0,FftWorker::buffer_size/2);
}

void MainWindow::on_compute_fft_complete()
{
    is_computing_fft = 0;
    if(is_saving)
    {
        QVector<double> temp;
        QVector<double> y = fft_worker->getYData();
        for(int i=0;i<fft_accum.size();i++)
        {
            fft_accum[i] = fft_accum.at(i) + y.at(i);
            (fft_accum_count[i])++;
            temp.append(fft_accum.at(i)/(double)fft_accum_count.at(i));
        }

        curve_fft->setSamples(fft_worker->getXData(),temp);

        //ui->fft_plot->replot();
        return;
    }

    curve_fft->setSamples(fft_worker->getXData(),fft_worker->getYData());

}

void MainWindow::on_recordButton_clicked()
{
    if(!is_saving)
    {
        is_saving = 1;
        to_compute_limits = 1;
        saved_data.clear();
        ui->recordButton->setText("Write file");
    }
    else
    {
        ui->recordButton->setText("Start recording");
        is_saving = 0;

        QFile file("data.csv");
        file.open(QIODevice::WriteOnly | QIODevice::Truncate);
        for(int i=0;i<saved_data.size();i++)
        {
            file.write(QString::number(saved_data.at(i)).append("\r\n").toLatin1());
        }
        file.close();
        qDebug()<<"data written to file data.csv";
    }
}
