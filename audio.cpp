
#include "audio.h"

Audio::Audio( QObject *parent )
    : QObject( parent ),
      isCoreRunning( false ),
      audioOut( nullptr ),
      audioOutputBuffer(),
      audioInputBuffer( new AudioBuffer ) {

    Q_CHECK_PTR( audioInputBuffer );

    resamplerState = nullptr;

    // We need to send this signal to ourselves
    connect( this, &Audio::signalFormatChanged, this, &Audio::slotHandleFormatChanged );

    outputDataFloat = nullptr;
    outputDataShort = nullptr;
    audioTimer = nullptr;

    outputBufferPos = 0;

    audioOutputBuffer.open( QBuffer::ReadWrite );
}

Audio::~Audio() {
    if( audioOut ) {
        delete audioOut;
    }

    //if( audioOutIODev ) {
    //    delete audioOutIODev;
    //}

    if( outputDataFloat ) {
        delete outputDataFloat;
    }

    if( outputDataShort ) {
        delete outputDataShort;
    }
}

AudioBuffer *Audio::getAudioBuf() const {
    return audioInputBuffer.get();
}

void Audio::setInFormat( QAudioFormat newInFormat, double videoFPS ) {

    qCDebug( phxAudio, "setInFormat(%iHz %ibits)", newInFormat.sampleRate(), newInFormat.sampleSize() );

    QAudioDeviceInfo info( QAudioDeviceInfo::defaultOutputDevice() );

    audioFormatIn = newInFormat;
    audioFormatOut = info.nearestFormat( newInFormat ); // try using the nearest supported format

    if( audioFormatOut.sampleRate() <= audioFormatIn.sampleRate() ) {
        // If that got us a format with a worse sample rate, use preferred format
        audioFormatOut = info.preferredFormat();
    }

    sampleRateRatio = ( double )audioFormatOut.sampleRate()  / audioFormatIn.sampleRate();

    // Hard-coded to 16-bit audio
    audioFormatOut.setSampleSize( 16 );

    qCDebug( phxAudio ) << "audioFormatIn" << audioFormatIn << audioFormatIn.isValid();
    qCDebug( phxAudio ) << "audioFormatOut" << audioFormatOut << audioFormatOut.isValid();
    qCDebug( phxAudio ) << "sampleRateRatio" << sampleRateRatio;
    qCDebug( phxAudio, "Using nearest format supported by sound card: %iHz %ibits",
             audioFormatOut.sampleRate(), audioFormatOut.sampleSize() );

    this->videoFPS = videoFPS;

    emit signalFormatChanged();

}

void Audio::slotHandleNotify() {
    qCDebug( phxAudio ) << "\t50ms consumed";
    outputBufferPos -= audioFormatOut.bytesForDuration( 50 * 1000 );
}

void Audio::slotHandleFormatChanged() {
    if( audioOut ) {
        audioOut->stop();
        delete audioOut;
    }

    audioOut = new QAudioOutput( audioFormatOut );
    Q_CHECK_PTR( audioOut );

    connect( audioOut, &QAudioOutput::stateChanged, this, &Audio::slotStateChanged );
    audioOut->start( &audioOutputBuffer );

    if( !isCoreRunning ) {
        audioOut->suspend();
    }

    // Set the timer for one period
    Q_CHECK_PTR( audioTimer );
    audioTimer->setInterval( ( ( double )audioOut->periodSize() / audioOut->bufferSize() ) * ( audioFormatOut.durationForBytes( audioOut->bufferSize() ) / 1000 ) );
    audioOut->setNotifyInterval( 50 );

    qCDebug( phxAudio ) << "Audio timer set to" << audioTimer->interval()
                        << "ms, Period size" << audioOut->periodSize()
                        << "bytes, buffer size" << audioOut->bufferSize()
                        << "bytes, notifyInterval" << audioOut->notifyInterval()
                        << "ms";

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

    connect( audioOut, &QAudioOutput::notify, this, &Audio::slotHandleNotify );
    outputBufferPos = 0;
}

void Audio::slotThreadStarted() {
    if( !audioFormatIn.isValid() ) {
        // We don't have a valid audio format yet...
        qCDebug( phxAudio ) << "audioFormatIn is not valid";
        return;
    }

    slotHandleFormatChanged();
}

void Audio::slotHandlePeriodTimer() {

    // Handle the situation where there is an error opening the audio device
    if( audioOut->error() == QAudio::OpenError ) {
        qWarning( phxAudio ) << "QAudio::OpenError, attempting reset...";
        emit signalFormatChanged();
    }

    auto samplesPerFrame = 2;

    // Max number of bytes/frames we can write to the output
    int outputBytesFree = audioOutputBuffer.size() - outputBufferPos;
    int outputFramesFree = audioFormatOut.framesForBytes( outputBytesFree );
    int outputSamplesFree = outputFramesFree * samplesPerFrame;
    Q_UNUSED( outputSamplesFree );

    //
    // TODO: Make these configurable
    //

    // Last term is the absolute bare minimum buffer target, add something to it to ensure no underruns
    int outputBufferTargetMs = ( double )2 * ( 1000.0 / videoFPS );// + audioFormatOut.durationForBytes( audioOut->periodSize() ) / 1000;

    // Max amount of stretching performed to compensate for output buffer position being off target
    double maxDeviation = 0.005;

    //
    //
    //

    // Read one frame of data from input
    //int outputPeriodLengthUs = ( ( double )audioOut->periodSize() / audioOut->bufferSize() ) * ( audioFormatOut.durationForBytes( audioOut->bufferSize() ) );
    int videoFramePeriodUs = ( ( double )1000000.0 / videoFPS );
    int inputBytesToRead = audioFormatIn.bytesForDuration( videoFramePeriodUs );

    // Read the input data
    int inputBytesRead = audioInputBuffer->read( inputDataChar, inputBytesToRead );
    int inputFramesRead = audioFormatIn.framesForBytes( inputBytesRead );
    int inputSamplesRead = inputFramesRead * samplesPerFrame;

    // Calculate how much the read data should be stretched to fit the ideal size
    // Does not use max deviation, only ever needed when there's not even one video frame of audio in the input buffer
    int outputBytesPerVideoFrame = audioFormatOut.bytesForDuration( ( ( double )1000000.0 / videoFPS ) );
    double outputAudioRoomToStretch = audioFormatOut.bytesForDuration( audioFormatIn.durationForBytes( inputBytesToRead - inputBytesRead ) );
    double stretchRatio = ( double )outputAudioRoomToStretch / outputBytesPerVideoFrame;

    // Calculate how much the read data should be scaled (shrunk or stretched) to keep the buffer on target
    // Assume the ideal amount was read
    int outputBufferTargetByte = audioFormatOut.bytesForDuration( outputBufferTargetMs * 1000 );
    int outputBufferVectorTargetToCurrent = outputBufferTargetByte - ( 44100 - outputBytesFree );
    double bufferTargetCorrectionRatio = ( double )outputBufferVectorTargetToCurrent / outputBufferTargetByte;

    // Calculate the final DRC ratio
    double DRCRatio = ( 1.0 + maxDeviation * bufferTargetCorrectionRatio ) * ( 1.0 + stretchRatio );
    double adjustedSampleRateRatio = sampleRateRatio * DRCRatio;

    // libsamplerate works in floats, must convert to floats for processing
    src_short_to_float_array( ( short * )inputDataChar, inputDataFloat, inputSamplesRead );

    // Set up a struct containing parameters for the resampler
    SRC_DATA srcData;
    srcData.data_in = inputDataFloat;
    srcData.data_out = outputDataFloat;
    srcData.end_of_input = 0;
    srcData.input_frames = inputFramesRead;
    srcData.output_frames = outputFramesFree; // Max size
    srcData.src_ratio = adjustedSampleRateRatio;

    // Perform resample
    src_set_ratio( resamplerState, adjustedSampleRateRatio );
    auto errorCode = src_process( resamplerState, &srcData );

    if( errorCode ) {
        qCWarning( phxAudio ) << "libresample error: " << src_strerror( errorCode ) ;
    }

    auto outputFramesConverted = srcData.output_frames_gen;
    auto outputBytesConverted = audioFormatOut.bytesForFrames( outputFramesConverted );
    auto outputSamplesConverted = outputFramesConverted * samplesPerFrame;

    // Convert float data back to shorts
    src_float_to_short_array( outputDataFloat, outputDataShort, outputSamplesConverted );

    int outputBytesWritten = audioOutputBuffer.write( ( char * ) outputDataShort, outputBytesConverted );
    outputBufferPos += outputBytesWritten;

    //audioOutIODev->waitForBytesWritten( -1 );

    qCDebug( phxAudio ) << "Input is" << ( audioInputBuffer->size() * 100 / audioFormatIn.bytesForFrames( 4096 ) )
                        << "% full, output is" << ( ( ( double )( outputBufferPos ) / 44100 ) * 100 )
                        << "% full DRC adjust =" << DRCRatio;
    qCDebug( phxAudio ) << "\toutputBufferPos =" << outputBufferPos
                        << "outputAudioRoomToStretch" << outputAudioRoomToStretch
                        << "stretchRatio =" << stretchRatio
                        << "bufferTargetCorrectionRatio =" << bufferTargetCorrectionRatio
                        << "sampleRateRatio =" << sampleRateRatio
                        << "adjustedSampleRateRatio =" << adjustedSampleRateRatio;
    qCDebug( phxAudio ) << "\tInput: needed" << inputBytesToRead << "bytes, read" << inputBytesRead << "bytes"
                        << "Output: needed" << outputBufferVectorTargetToCurrent << "bytes, wrote" << outputBytesWritten << "bytes";
    qCDebug( phxAudio ) << "\toutputBytesFree =" << outputBytesFree
                        << "outputBufferTargetByte =" << outputBufferTargetByte
                        << "outputBufferVectorTargetToCurrent =" << outputBufferVectorTargetToCurrent
                        //<< "audioOut->bufferSize() =" << audioOut->bufferSize()
                        << "audioOutputBuffer->bytesToWrite() =" << audioOutputBuffer.bytesToWrite();
    /*
    qCDebug( phxAudio ) << "\tOutput buffer is" << audioFormatOut.durationForBytes( audioOut->bufferSize() ) / 1000
                        << "ms, buffer target is" << outputBufferTargetMs
                        << "ms (" << 100.0 * ( ( double )outputBufferTargetMs / ( audioFormatOut.durationForBytes( audioOut->bufferSize() ) / 1000.0 ) )
                        << "%)";
    */

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
        //audioOutputBuffer.write( silence, 10000 );
        //audioOut->start( &audioOutputBuffer );
    }

    if( s != QAudio::IdleState ) {
        qCDebug( phxAudio ) << "State changed:" << s;
    }
}

void Audio::slotSetVolume( qreal level ) {
    if( audioOut ) {
        audioOut->setVolume( level );
    }
}


