#ifndef LOOPER_H
#define LOOPER_H

#include <QObject>
#include <QElapsedTimer>
#include <QDebug>
#include <QThread>

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
};

#endif // LOOPER_H
