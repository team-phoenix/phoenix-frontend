
#include "audiooutput.h"

AudioOutput::AudioOutput()
    : isCoreRunning( false ),
      audioOutInterface( nullptr ),
      audioOutIODev( nullptr ),
      resamplerState( nullptr ),
      outputDataFloat( nullptr ),
      outputDataShort( nullptr ) {

}

AudioOutput::~AudioOutput() {
    if( audioOutInterface ) {
        delete audioOutInterface;
    }

    if( outputDataFloat ) {
        delete outputDataFloat;
    }

    if( outputDataShort ) {
        delete outputDataShort;
    }
}

void AudioOutput::slotSetInputFormat( QAudioFormat newInFormat, double coreFrameRate ) {

    qCDebug( phxAudio, "setInFormat(%iHz %ibits)", newInFormat.sampleRate(), newInFormat.sampleSize() );

    QAudioDeviceInfo info( QAudioDeviceInfo::defaultOutputDevice() );

    audioFormatIn = newInFormat;

    // Try using the nearest supported format
    audioFormatOut = info.nearestFormat( newInFormat );

    // If that got us a format with a worse sample rate, use preferred format
    if( audioFormatOut.sampleRate() <= audioFormatIn.sampleRate() ) {
        audioFormatOut = info.preferredFormat();
    }

    sampleRateRatio = ( qreal )audioFormatOut.sampleRate()  / audioFormatIn.sampleRate();
    this->coreFrameRate = coreFrameRate;

    qCDebug( phxAudio ) << "audioFormatIn" << audioFormatIn;
    qCDebug( phxAudio ) << "audioFormatOut" << audioFormatOut;
    qCDebug( phxAudio ) << "sampleRateRatio" << sampleRateRatio;
    qCDebug( phxAudio, "Using nearest format supported by sound card: %iHz %ibits",
             audioFormatOut.sampleRate(), audioFormatOut.sampleSize() );

}

void AudioOutput::slotInitAudio() {

    // Re-create the output interface object
    if( audioOutInterface ) {
        audioOutInterface->stop();
        delete audioOutInterface;
    }

    audioOutInterface = new QAudioOutput( audioFormatOut, this );
    Q_CHECK_PTR( audioOutInterface );


    connect( audioOutInterface, &QAudioOutput::stateChanged, this, &AudioOutput::slotStateChanged );
    audioOutInterface->setBufferSize( audioFormatOut.sampleRate() );
    audioOutIODev = audioOutInterface->start();

    if( !isCoreRunning ) {
        audioOutInterface->suspend();
    }

    //audioOutIODev->moveToThread( &audioThread );

    // This is where the amount of time that passes between audio updates is set
    // At timer intervals this low on most OSes the jitter is quite significant
    // Try to grab data from the input buffer at least every frame
    // qint64 durationInMs = audioFormatOut.durationForBytes( audioOut->periodSize() * 1.0 ) / 1000;

    qint64 durationInMs = 16;
    qCDebug( phxAudio ) << "Timer interval set to" << durationInMs << "ms, Period size" << audioOutInterface->periodSize() << "bytes, buffer size" << audioOutInterface->bufferSize() << "bytes";


    // audioTimer.setInterval( durationInMs );

    if( resamplerState ) {
        src_delete( resamplerState );
    }

    int errorCode;
    resamplerState = src_new( SRC_SINC_BEST_QUALITY, 2, &errorCode );

    if( !resamplerState ) {
        qCWarning( phxAudio ) << "libresample could not init: " << src_strerror( errorCode ) ;
    }

    // Now that the IO devices are properly set up,
    // allocate space for buffers that'll hold up to their hardware couterpart's size in data
    auto outputDataSamples = audioOutInterface->bufferSize() * 2;
    qCDebug( phxAudio ) << "Allocated" << outputDataSamples << "for conversion.";

    if( outputDataFloat ) {
        delete outputDataFloat;
    }

    if( outputDataShort ) {
        delete outputDataShort;
    }

    outputDataFloat = new float[outputDataSamples];
    outputDataShort = new short[outputDataSamples];
}

void AudioOutput::slotThreadStarted() {
    if( !audioFormatIn.isValid() ) {
        // We don't have a valid audio format yet...
        qCDebug( phxAudio ) << "audioFormatIn is not valid";
        return;
    }

    slotInitAudio();
}

void AudioOutput::slotHandleAudioData( int16_t data ) {

    // Handle the situation where there is no device to output to

    if( !audioOutIODev ) {
        qCDebug( phxAudio ) << "Audio device was not found, attempting reset...";
        slotInitAudio();
        return;
    }

    // Handle the situation where there is an error opening the audio device
    if( audioOutInterface->error() == QAudio::OpenError ) {
        qWarning( phxAudio ) << "QAudio::OpenError, attempting reset...";
        slotInitAudio();
    }

    if( !audioOutInterface->bytesFree() ) {
        return;
    }

    int chunks = audioOutInterface->bytesFree() / audioOutInterface->periodSize();
    QVarLengthArray<char, 44100> tmpbuf( audioOutInterface->bytesFree() );

    //qDebug() << audioOut->bytesFree();

    while( chunks ) {
        const qint64 len = 0;// audioBuf->read( tmpbuf.data(), audioOutInterface->periodSize() );

        if( len ) {
            audioOutIODev->write( tmpbuf.data(), len );
        }

        if( len != audioOutInterface->periodSize() ) {
            break;
        }

        --chunks;
    }

}

void AudioOutput::slotRunChanged( bool _isCoreRunning ) {
    isCoreRunning = _isCoreRunning;

    if( !audioOutInterface ) {
        return;
    }

    if( !isCoreRunning ) {
        if( audioOutInterface->state() != QAudio::SuspendedState ) {
            qCDebug( phxAudio ) << "Paused";
            audioOutInterface->suspend();
            //emit signalStopTimer();
        }
    } else {
        if( audioOutInterface->state() != QAudio::ActiveState ) {
            qCDebug( phxAudio ) << "Started";
            audioOutInterface->resume();
            //emit signalStartTimer();
        }
    }
}

void AudioOutput::slotStateChanged( QAudio::State s ) {
    if( s == QAudio::IdleState && audioOutInterface->error() == QAudio::UnderrunError ) {
        qWarning( phxAudio ) << "audioOut underrun";
        audioOutIODev = audioOutInterface->start();
    }

    if( s != QAudio::IdleState && s != QAudio::ActiveState ) {
        qCDebug( phxAudio ) << "State changed:" << s;
    }
}

void AudioOutput::slotSetVolume( qreal level ) {
    if( audioOutInterface ) {
        audioOutInterface->setVolume( level );
    }
}


