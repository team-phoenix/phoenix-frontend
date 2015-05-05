
#include "audio.h"

Audio::Audio( QObject *parent )
    : QObject( parent ),
      isCoreRunning( false ),
      audioOut( nullptr ),
      audioOutIODev( nullptr ),
      resamplerState( nullptr ),
      outputDataFloat( nullptr ),
      outputDataShort( nullptr ) {






    //connect( &audioTimer, &QTimer::timeout, this, &Audio::slotHandlePeriodTimer );
    //connect( this, &Audio::signalStartTimer, &audioTimer, static_cast<void ( QTimer::* )( void )> ( &QTimer::start ) );
    //connect( this, &Audio::signalStopTimer, &audioTimer, &QTimer::stop );
    // We need to send this signal to ourselves
    connect( this, &Audio::signalFormatChanged, this, &Audio::slotHandleFormatChanged );

}

Audio::~Audio() {
    if( audioOut ) {
        delete audioOut;
    }

    if( outputDataFloat ) {
        delete outputDataFloat;
    }

    if( outputDataShort ) {
        delete outputDataShort;
    }
}

void Audio::setInFormat( QAudioFormat newInFormat ) {

    qCDebug( phxAudio, "setInFormat(%iHz %ibits)", newInFormat.sampleRate(), newInFormat.sampleSize() );

    QAudioDeviceInfo info( QAudioDeviceInfo::defaultOutputDevice() );

    audioFormatIn = newInFormat;
    audioFormatOut = info.nearestFormat( newInFormat ); // try using the nearest supported format

    if( audioFormatOut.sampleRate() < audioFormatIn.sampleRate() ) {
        // If that got us a format with a worse sample rate, use preferred format
        audioFormatOut = info.preferredFormat();
    }

    sampleRateRatio = ( qreal )audioFormatOut.sampleRate()  / audioFormatIn.sampleRate();

    qCDebug( phxAudio ) << "audioFormatIn" << audioFormatIn;
    qCDebug( phxAudio ) << "audioFormatOut" << audioFormatOut;
    qCDebug( phxAudio ) << "sampleRateRatio" << sampleRateRatio;
    qCDebug( phxAudio, "Using nearest format supported by sound card: %iHz %ibits",
             audioFormatOut.sampleRate(), audioFormatOut.sampleSize() );

    emit signalFormatChanged();

}

void Audio::slotHandleFormatChanged() {
    if( audioOut ) {
        audioOut->stop();
        delete audioOut;
    }

    audioOut = new QAudioOutput( audioFormatOut, this );
    Q_CHECK_PTR( audioOut );


    connect( audioOut, &QAudioOutput::stateChanged, this, &Audio::slotStateChanged );
    audioOutIODev = audioOut->start();

    if( !isCoreRunning ) {
        audioOut->suspend();
    }

    //audioOutIODev->moveToThread( &audioThread );

    // This is where the amount of time that passes between audio updates is set
    // At timer intervals this low on most OSes the jitter is quite significant
    // Try to grab data from the input buffer at least every frame
    // qint64 durationInMs = audioFormatOut.durationForBytes( audioOut->periodSize() * 1.0 ) / 1000;

    qint64 durationInMs = 16;
    qCDebug( phxAudio ) << "Timer interval set to" << durationInMs << "ms, Period size" << audioOut->periodSize() << "bytes, buffer size" << audioOut->bufferSize() << "bytes";


    audioTimer.setInterval( durationInMs );

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
    auto outputDataSamples = audioOut->bufferSize() * 2;
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

void Audio::slotThreadStarted() {
    if( !audioFormatIn.isValid() ) {
        // We don't have a valid audio format yet...
        qCDebug( phxAudio ) << "audioFormatIn is not valid";
        return;
    }

    slotHandleFormatChanged();
}

void Audio::slotHandlePeriodTimer( AudioBuffer *audioBuf, int size ) {

    // Handle the situation where there is no device to output to

    if( !audioOutIODev ) {
        qCDebug( phxAudio ) << "Audio device was not found, attempting reset...";
        emit signalFormatChanged();
        return;
    }

    // Handle the situation where there is an error opening the audio device
    if( audioOut->error() == QAudio::OpenError ) {
        qWarning( phxAudio ) << "QAudio::OpenError, attempting reset...";
        emit signalFormatChanged();
    }

    int chunks = audioOut->bytesFree() / audioOut->periodSize();
    QVarLengthArray<char, 4096 * 4> tmpbuf(audioOut->bytesFree());

    while (chunks) {
        const qint64 len = audioBuf->read( tmpbuf.data(), audioOut->periodSize() );
        if (len)
            audioOutIODev->write( tmpbuf.data(), len );
        if (len != audioOut->periodSize())
            break;
        --chunks;
    }

}

void Audio::slotRunChanged( bool _isCoreRunning ) {
    isCoreRunning = _isCoreRunning;

    if( !audioOut ) {
        return;
    }

    if( !isCoreRunning ) {
        if( audioOut->state() != QAudio::SuspendedState ) {
            qCDebug( phxAudio ) << "Paused";
            audioOut->suspend();
            emit signalStopTimer();
        }
    } else {
        if( audioOut->state() != QAudio::ActiveState ) {
            qCDebug( phxAudio ) << "Started";
            audioOut->resume();
            emit signalStartTimer();
        }
    }
}

void Audio::slotStateChanged( QAudio::State s ) {
    if( s == QAudio::IdleState && audioOut->error() == QAudio::UnderrunError ) {
        qWarning( phxAudio ) << "audioOut underrun";
        audioOutIODev = audioOut->start();
    }

    if( s != QAudio::IdleState && s != QAudio::ActiveState ) {
        qCDebug( phxAudio ) << "State changed:" << s;
    }
}

void Audio::slotSetVolume( qreal level ) {
    if( audioOut ) {
        audioOut->setVolume( level );
    }
}


