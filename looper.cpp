#include "looper.h"

Looper::Looper( QObject *parent ) : QObject( parent ) {

}

void Looper::beginLoop( double interval ) {
    QElapsedTimer timer;
    int counter = 1;
    int printEvery = 1;
    double averageSleepDelta = 0;

    while( 1 ) {
        timer.start();
        counter++;
        emit signalFrame();

        double frameTime = ( double )timer.nsecsElapsed() / 1000.0 / 1000.0;

        timer.start();

        double remainingFrameTime = interval - frameTime - 2.0 * averageSleepDelta;

        if( remainingFrameTime < 0 ) {
            remainingFrameTime = 0;
        }

        if( ( counter % printEvery ) == 0 ) {
            qDebug() << "Frame took" << frameTime << "ms";
            qDebug() << "Remaining:" << remainingFrameTime << "ms";
        }

        QThread::usleep( remainingFrameTime * 1000.0 );

        double sleepTime = ( double )timer.nsecsElapsed() / 1000.0 / 1000.0;
        double totalTime = frameTime + sleepTime;
        averageSleepDelta = averageSleepDelta * ( counter - 1 ) /
                            counter + ( totalTime - interval ) / counter;

        if( ( counter % printEvery ) == 0 )  {
            qDebug() << "Slept for" << sleepTime << "ms";
            qDebug() << "\tTotal:" << totalTime << "ms, target:" << interval << "ms";
            qDebug() << "\tDifference:" << totalTime - interval << " -- " << ( ( totalTime - interval ) / interval ) * 100.0 << "%";
            qDebug() << "\tSleep delta:" << averageSleepDelta;
            qDebug() << "";
        }

        // if( counter % 60 == 0 ) {
        //     double temp = averageSleepDelta / counter;
        //     counter = 1;
        //     averageSleepDelta = temp;
        // }
    }
}

