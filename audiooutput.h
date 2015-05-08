
#ifndef AUDIO_H
#define AUDIO_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QAudioOutput>
#include <QDebug>

#include <memory>
#include <atomic>

#include "audiobuffer.h"
#include "logging.h"

#include "samplerate.h"

/* The Audio class writes audio data to connected audio device.
 * All of the audio functionality lives in side of this class.
 * Any errors starting with "[phoenix.audio]" correspond to this class.
 *
 * The audio class is instantiated inside of the videoitem.cpp class.
 * The Audio class uses the AudioBuffer class, which lives in the audiobuffer.cpp class, as a temporary audio buffer
 * that can be written has a whole chunk to the audio output.
 */

class AudioOutput : public QObject {
        Q_OBJECT

    signals:

        void signalFormatChanged();

    public slots:

        // Tell Audio what sample rate to expect from Core
        void slotSetInputFormat( QAudioFormat newInFormat, double coreFrameRate );

        // Completely init/re-init audio output
        void slotInitAudio();

        // Output incoming video frame of audio data to the audio output
        void slotHandleAudioData( int16_t data );

        void slotStateChanged( QAudio::State state );
        void slotRunChanged( bool _isCoreRunning );
        void slotSetVolume( qreal level );
        void slotThreadStarted();

    public:

        AudioOutput();
        ~AudioOutput();

    private:

        // Opaque pointer for libsamplerate
        SRC_STATE *resamplerState;

        double sampleRateRatio;
        double coreFrameRate;

        int audioInBytesNeeded;
        float inputDataFloat[4096 * 2];
        char inputDataChar[4096 * 4];
        float *outputDataFloat;
        short *outputDataShort;


        bool isCoreRunning;

        QAudioFormat audioFormatOut;
        QAudioFormat audioFormatIn;

        QAudioOutput *audioOutInterface;

        QIODevice *audioOutIODev;

};

#endif
