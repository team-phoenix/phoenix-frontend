#include "looper.h"

Looper::Looper( QObject *parent ) : QObject( parent ) {

}

void Looper::beginLoop( double interval ) {

    QElapsedTimer timer;
    int counter = 1;
    int printEvery = 1;

    double timeElapsed = 0.0;

    forever {

        if( timeElapsed > interval ) {
            timer.start();

            counter++;
            if( counter % printEvery == 0 )  {
                qDebug() << "timeElapsed =" << timeElapsed << "ms | interval =" << interval << "ms";
                qDebug() << "Difference:" << timeElapsed - interval << " -- " << ( ( timeElapsed - interval ) / interval ) * 100.0 << "%";
            }

            emit signalFrame();

            // Reset the frame timer
            timeElapsed = ( double )timer.nsecsElapsed() / 1000.0 / 1000.0;
        }

        timer.start();

        // Running this just once means massive overhead from calling timer.start() so many times so quickly
        for( int i = 0; i < 100; i++ ) {
            QThread::yieldCurrentThread();
        }

        timeElapsed += ( double )timer.nsecsElapsed() / 1000.0 / 1000.0;

    }
}

