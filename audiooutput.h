
#ifndef AUDIO_H
#define AUDIO_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QAudioOutput>
#include <QDebug>
#include <QBuffer>

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

    public slots:

        // Tell Audio what sample rate to expect from Core
        void slotAudioFormat( int sampleRate, double coreFPS, double hostFPS );

        // Output incoming video frame of audio data to the audio output
        void slotAudioData( int16_t *inputData, int inputBytes );

        // Respond to the core running or not by keeping audio output active or not
        // AKA Pause if core is paused
        void slotSetAudioActive( bool coreIsRunning );

        // Set volume level [0.0...1.0]
        void slotSetVolume( qreal level );

    private slots:

        void slotAudioOutputStateChanged( QAudio::State state );

    public:

        AudioOutput();
        ~AudioOutput();

    private:

        // Completely init/re-init audio output
        void resetAudio();

        // Opaque pointer for libsamplerate
        SRC_STATE *resamplerState;

        // Audio and video timing provided by Core via the controller
        int sampleRate;
        double coreFPS;
        double hostFPS;
        double sampleRateRatio;

        int audioInBytesNeeded;
        float *inputDataFloat;
        char *inputDataChar;
        float *outputDataFloat;
        short *outputDataShort;


        bool coreIsRunning;

        // Input and output audio formats being used
        QAudioFormat outputAudioFormat;
        QAudioFormat inputAudioFormat;

        QAudioOutput *outputAudioInterface;

        int outputCurrentByte;

        AudioBuffer outputBuffer;

        //
        // TODO: Make these configurable
        //

        int outputLengthMs;

        // Make this large enough to ensure no underruns
        int outputTargetMs;

        // Max amount of stretching performed to compensate for output buffer position being off target
        double maxDeviation;

        //
        // ---
        //


};

#endif
