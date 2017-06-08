#ifndef FFTWORKER_H
#define FFTWORKER_H

#include <QObject>
#include <fftw/fftw3.h>

#include <QMutex>
#include <QVector>

/*
 * Class for computing FFT of input data in another thread
 */
class FftWorker : public QObject
{
    Q_OBJECT
public:
    explicit FftWorker(QObject *parent = 0);
    ~FftWorker();
    void setData(const QVector<double>& ydata);
    static const int buffer_size = 512;
    const QVector<double>& getXData();
    const QVector<double>& getYData();
private:
    float *in;
    float *out;
    QMutex mutex;
    void compute_fftw();
    QVector<double> *out_x;
    QVector<double> *out_y;
    fftwf_plan plan;
    bool has_plan;
signals:
    void compute_complete();
public slots:
    void on_compute_requested();

};

#endif // FFTWORKER_H
