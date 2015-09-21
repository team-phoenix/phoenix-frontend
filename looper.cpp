#include "looper.h"

Looper::Looper( QObject *parent ) : QObject( parent ) {

}

void Looper::beginLoop( double interval ) {
    QElapsedTimer timer;
    int counter = 1;
    int printEvery = 60;
    double averageSleepDelta = 0;

#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
    MMRESULT ret = timeGetDevCaps( &timecaps, sizeof( TIMECAPS ) );

    switch( ret ) {
        case MMSYSERR_NOERROR:
            break;

        case MMSYSERR_ERROR:
            qWarning() << "MMSYSERR_ERROR (general error?)";
            break;

        case TIMERR_NOCANDO:
            qWarning() << "TIMERR_NOCANDO (bad parameters)";
            break;

        default:
            break;
    }

    qDebug() << "Minimum timer resolution:" << timecaps.wPeriodMin << "ms";
    qDebug() << "Maximum timer resolution:" << timecaps.wPeriodMax << "ms";

#endif

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

        // if( ( counter % printEvery ) == 0 ) {
        //     qDebug() << "Frame took" << frameTime << "ms";
        //     qDebug() << "Remaining:" << remainingFrameTime << "ms";
        // }
#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
        timeBeginPeriod( timecaps.wPeriodMin );
#endif
        QThread::usleep( remainingFrameTime * 1000.0 );
#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
        timeEndPeriod( timecaps.wPeriodMax );
#endif

        double sleepTime = ( double )timer.nsecsElapsed() / 1000.0 / 1000.0;
        double totalTime = frameTime + sleepTime;
        averageSleepDelta = averageSleepDelta * ( counter - 1 ) /
                            counter + ( totalTime - interval ) / counter;

        // if( ( counter % printEvery ) == 0 )  {
        //     qDebug() << "Slept for" << sleepTime << "ms";
        //     qDebug() << "\tTotal:" << totalTime << "ms, target:" << interval << "ms";
        //     qDebug() << "\tDifference:" << totalTime - interval << " -- " << ( ( totalTime - interval ) / interval ) * 100.0 << "%";
        //     qDebug() << "\tSleep delta:" << averageSleepDelta;
        //     qDebug() << "";
        // }

        // if( counter % 60 == 0 ) {
        //     double temp = averageSleepDelta / counter;
        //     counter = 1;
        //     averageSleepDelta = temp;
        // }
    }
}

