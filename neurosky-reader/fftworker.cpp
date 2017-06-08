#include "fftworker.h"

#include <QDebug>
#include <assert.h>
#include <QThread>
#include <cmath>



FftWorker::FftWorker(QObject *parent) : QObject(parent)
{
    out_x = new QVector<double>();
    out_y = new QVector<double>();
    for(int i=0;i<buffer_size/2;i++){
        out_x->append((double)i/buffer_size*512.0);
        out_y->append(-1);
    }
    in = new float[buffer_size];
    out = new float[buffer_size];
    has_plan = 0;

}

FftWorker::~FftWorker()
{
    fftwf_cleanup();
    if(has_plan)
        fftwf_destroy_plan(plan);
    delete out_x;
    delete out_y;
}

void FftWorker::setData(const QVector<double> &ydata)
{
    assert(ydata.size() == buffer_size);
    mutex.lock();
    /*for(int i=0;i<buffer_size;i++){
        in[i] = (float)ydata.at(i);
        //in[i] = sin((float)i/(float)buffer_size*2*3.14*100)*100;
    }
    if(!has_plan)
    {
        plan = fftwf_plan_r2r_1d(buffer_size,in,out,FFTW_R2HC,FFTW_MEASURE);
        has_plan = 1;
        for(int i=0;i<buffer_size;i++){
            in[i] = (float)ydata.at(i);
            in[i] = sin((float)i/(float)buffer_size*2*3.14*500);
        }

    }*/

    for(int i=0;i<buffer_size;i++){
        in[i] = (float)ydata.at(i);
        //in[i] = sin((float)i/(float)buffer_size*2*3.14*100);
    }

    mutex.unlock();
}

const QVector<double> &FftWorker::getXData()
{
    return *out_x;
}

const QVector<double> &FftWorker::getYData()
{
    return *out_y;
}

void FftWorker::compute_fftw()
{
    /*if(!has_plan)
    {
        qDebug()<<"has not plan!";
        return;
    }*/

    plan = fftwf_plan_r2r_1d(buffer_size,in,out,FFTW_R2HC,FFTW_ESTIMATE);
    fftwf_execute(plan);
    fftwf_destroy_plan(plan);
    //fftwf_execute_r2r(plan,in,out);
    //convert half complex array to real magnitudes
    //(*out_y)[0] = out[0];
    float real,imag;
    for(int i=1;i<=buffer_size/2-1;i++){
        real = out[i];
        imag = out[buffer_size-i];
        (*out_y)[i-1] = sqrt(real*real+imag*imag);
    }
    (*out_y)[buffer_size/2-1] = out[buffer_size/2];

}

void FftWorker::on_compute_requested()
{
    mutex.lock();
    compute_fftw();
    mutex.unlock();
    emit compute_complete();
}

