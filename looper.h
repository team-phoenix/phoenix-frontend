#ifndef LOOPER_H
#define LOOPER_H

#include <QObject>
#include <QElapsedTimer>
#include <QDebug>
#include <QThread>

#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
#include <Windows.h>
#endif

class Looper : public QObject {
        Q_OBJECT
    public:
        explicit Looper( QObject *parent = 0 );

    signals:
        void signalFrame();

    public slots:
        void beginLoop( double interval );

    private:
        // Rate the signal should be emitted at
        double interval;

#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
        TIMECAPS timecaps;
#endif
};

#endif // LOOPER_H
