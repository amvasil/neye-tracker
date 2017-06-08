/*
 * * Alexander Vasiliev, 2017
 * */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <qwt_plot_curve.h>
#include <qwt_plot_zoomer.h>
#include <QThread>
#include <QList>

#include "fftworker.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void error(QString str);
private slots:
    void on_pushButton_clicked();
    void slot_read_timer();
    void slot_replot_timer();
    void on_recordButton_clicked();

private:
    Ui::MainWindow *ui;
    int connectionId; //connection to mindwave through virtual uart
    QVector<double> data_x,data_y;
    QTimer readTimer,replotTimer;
    char errstr[200];
    QwtPlotCurve *curve,*curve_fft;
    QThread fft_thread;
    FftWorker *fft_worker;
    bool is_computing_fft;
    double qpc_freq;
    QwtPlotZoomer *zoom;
    QList<double> saved_data;
    bool is_saving;

    double max_line,min_line;
    QwtPlotCurve *max_line_curve,*min_line_curve;
    QVector<double> max_line_data,min_line_data,lim_x_data;
    bool to_compute_limits;
    QVector<double> fft_accum,fft_accum_count;

    double getTimestamp();
    double getTimeIntervalMs();
    double prev_ts;
    void update_limits(QVector<double> &data);
    void start_fft(QVector<double>& data);
signals:
    void compute_fft();
public slots:
    void on_compute_fft_complete();

};

#endif // MAINWINDOW_H
