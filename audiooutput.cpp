
#include "audiooutput.h"

const auto samplesPerFrame = 2;
const auto framesPerSample = 0.5;

AudioOutput::AudioOutput()
    : resamplerState( nullptr ),
      inputDataFloat( nullptr ),
      inputDataChar( nullptr ),
      outputDataFloat( nullptr ),
      outputDataShort( nullptr ),
      coreIsRunning( false ),
      audioOutInterface( nullptr ),
      outputBufferPos( 0 ),
      outputBuffer() {

    outputBuffer.start();

}

AudioOutput::~AudioOutput() {

    if( audioOutInterface ) {
        audioOutInterface->deleteLater();
        audioOutInterface = nullptr;
    }

    if( outputDataFloat ) {
        delete outputDataFloat;
        outputDataFloat = nullptr;
    }

    if( outputDataShort ) {
        delete outputDataShort;
        outputDataShort = nullptr;
    }

}

//
// Public slots
//

void AudioOutput::slotAudioFormat( int sampleRate, double coreFPS, double hostFPS ) {

    qCDebug( phxAudioOutput, "slotAudioFormat( %i Hz, %f fps (core), %f fps (host) )", sampleRate, coreFPS, hostFPS );

    this->sampleRate = sampleRate;
    this->coreFPS = coreFPS;
    this->hostFPS = hostFPS;

    audioFormatIn.setSampleSize( 16 );
    audioFormatIn.setSampleRate( sampleRate );
    audioFormatIn.setChannelCount( 2 );
    audioFormatIn.setSampleType( QAudioFormat::SignedInt );
    audioFormatIn.setByteOrder( QAudioFormat::LittleEndian );
    audioFormatIn.setCodec( "audio/pcm" );

    // Try using the nearest supported format
    QAudioDeviceInfo info( QAudioDeviceInfo::defaultOutputDevice() );
    audioFormatOut = info.nearestFormat( audioFormatIn );

    // If that got us a format with a worse sample rate, use preferred format
    if( audioFormatOut.sampleRate() <= audioFormatIn.sampleRate() ) {
        audioFormatOut = info.preferredFormat();
    }

    sampleRateRatio = ( qreal )audioFormatOut.sampleRate()  / audioFormatIn.sampleRate();

    qCDebug( phxAudioOutput ) << "audioFormatIn" << audioFormatIn;
    qCDebug( phxAudioOutput ) << "audioFormatOut" << audioFormatOut;
    qCDebug( phxAudioOutput ) << "sampleRateRatio" << sampleRateRatio;
    qCDebug( phxAudioOutput, "Using nearest format supported by sound card: %iHz %ibits",
             audioFormatOut.sampleRate(), audioFormatOut.sampleSize() );

    resetAudio();

}

void AudioOutput::slotAudioData( int16_t *data, int inputBytes ) {

    //
    // TODO: Make these configurable
    //

    const int outputBufferLengthMs = 50;

    // Make this large enough to ensure no underruns
    const int outputBufferTargetMs = ( double )1 * ( 1000.0 / coreFPS );

    // Max amount of stretching performed to compensate for output buffer position being off target
    const double maxDeviation = 0.005;

    //
    // ---
    //

    // Handle the situation where there is an error opening the audio device
    if( audioOutInterface->error() == QAudio::OpenError ) {
        qWarning( phxAudioOutput ) << "QAudio::OpenError, attempting reset...";
        resetAudio();
    }

    int inputFrames = audioFormatIn.framesForBytes( inputBytes );
    int inputSamples = inputFrames * samplesPerFrame;

    // Max number of bytes/frames we can write to the output
    int outputBufferSize = audioFormatOut.bytesForDuration( outputBufferLengthMs * 1000 );
    int outputBytesFree = 2 << 16;
    outputBufferPos = outputBuffer.bytesAvailable();
    int outputFramesFree = audioFormatOut.framesForBytes( outputBytesFree );
    int outputSamplesFree = outputFramesFree * samplesPerFrame;
    Q_UNUSED( outputSamplesFree );

    // Calculate how much the read data should be scaled (shrunk or stretched) to keep the buffer on target
    int outputBufferTargetByte = audioFormatOut.bytesForDuration( outputBufferTargetMs * 1000 );
    int outputBufferVectorTargetToCurrent = outputBufferTargetByte - ( outputBufferPos );
    double bufferTargetCorrectionRatio = ( double )outputBufferVectorTargetToCurrent / outputBufferTargetByte;

    // Calculate the final DRC ratio
    double DRCRatio = 1.0 + maxDeviation * bufferTargetCorrectionRatio;
    double adjustedSampleRateRatio = sampleRateRatio * DRCRatio * ( hostFPS / coreFPS );

    // libsamplerate works in floats, must convert to floats for processing
    src_short_to_float_array( ( short * )data, inputDataFloat, inputSamples );

    // Set up a struct containing parameters for the resampler
    SRC_DATA srcData;
    srcData.data_in = inputDataFloat;
    srcData.data_out = outputDataFloat;
    srcData.end_of_input = 0;
    srcData.input_frames = inputFrames;
    srcData.output_frames = outputFramesFree; // Max size
    srcData.src_ratio = adjustedSampleRateRatio;

    // Perform resample
    src_set_ratio( resamplerState, adjustedSampleRateRatio );
    auto errorCode = src_process( resamplerState, &srcData );

    if( errorCode ) {
        qCWarning( phxAudioOutput ) << "libresample error: " << src_strerror( errorCode ) ;
    }

    auto outputFramesConverted = srcData.output_frames_gen;
    auto outputBytesConverted = audioFormatOut.bytesForFrames( outputFramesConverted );
    auto outputSamplesConverted = outputFramesConverted * samplesPerFrame;

    // Convert float data back to shorts
    src_float_to_short_array( outputDataFloat, outputDataShort, outputSamplesConverted );

    // Send the converted data out
    int outputBytesWritten = outputBuffer.write( ( char * ) outputDataShort, outputBytesConverted );
    Q_UNUSED( outputBytesWritten );
    outputBufferPos += outputBytesWritten;

    /*
    qCDebug( phxAudioOutput ) << "Output is" << ( ( ( double )( ( outputBufferSize - outputBytesFree ) ) / outputBufferSize ) * 100 )
                              << "% full"
                              << "(outputBufferPos: " << ( ( ( double )( outputBufferPos ) / outputBufferSize ) * 100 )
                              << "% full)"
                              << "DRCRatio =" << DRCRatio
                              << "audioOutInterface->bufferSize()" << audioOutInterface->bufferSize();
    qCDebug( phxAudioOutput ) << "\toutputBufferPos =" << outputBufferPos
                              << "Input bytes =" << inputBytes
                              // << "outputAudioRoomToStretch" << outputAudioRoomToStretch
                              // << "stretchRatio =" << stretchRatio
                              << "bufferTargetCorrectionRatio =" << bufferTargetCorrectionRatio
                              << "sampleRateRatio =" << sampleRateRatio
                              << "adjustedSampleRateRatio =" << adjustedSampleRateRatio;
    qCDebug( phxAudioOutput ) << "\tOutput: needed" << outputBufferVectorTargetToCurrent << "bytes, wrote" << outputBytesWritten << "bytes";
    qCDebug( phxAudioOutput ) << "\toutputBytesFree =" << outputBytesFree
                              << "outputBufferTargetByte =" << outputBufferTargetByte
                              << "outputBufferVectorTargetToCurrent =" << outputBufferVectorTargetToCurrent;
    qCDebug( phxAudioOutput ) << "\taudioOutInterface->bufferSize() =" << audioOutInterface->bufferSize()
                              << "audioOutInterface->bytesFree() =" << audioOutInterface->bytesFree()
                              << "audioOutputBuffer->bytesToWrite() =" << outputBuffer.bytesToWrite();
    qCDebug( phxAudioOutput ) << "\toutputBufferIODev.bytesAvailable() =" << outputBuffer.bytesAvailable()
                              << "outputBufferIODev.size() =" << outputBuffer.size()
                              << "outputBufferIODev.pos() =" << outputBuffer.pos();
    qCDebug( phxAudioOutput ) << "\tOutput buffer is" << audioFormatOut.durationForBytes( outputBufferSize ) / 1000
                              << "ms, buffer target is" << outputBufferTargetMs
                              << "ms (" << ( double )100.0 * ( ( double )outputBufferTargetMs / ( ( double )( audioFormatOut.durationForBytes( outputBufferSize ) ) / 1000.0 ) )
                              << "%)";
    qCDebug( phxAudioOutput ) << "\tState:" << audioOutInterface->state() << " error: " << audioOutInterface->error();
    */

}

void AudioOutput::slotSetAudioActive( bool coreIsRunning ) {

    this->coreIsRunning = coreIsRunning;

    if( !audioOutInterface ) {
        return;
    }

    if( !coreIsRunning ) {
        if( audioOutInterface->state() != QAudio::SuspendedState ) {
            qCDebug( phxAudioOutput ) << "Paused";
            audioOutInterface->suspend();
        }
    } else {
        if( audioOutInterface->state() != QAudio::ActiveState ) {
            qCDebug( phxAudioOutput ) << "Started";
            audioOutInterface->resume();
        }
    }

}

void AudioOutput::slotSetVolume( qreal level ) {
    if( audioOutInterface ) {
        audioOutInterface->setVolume( level );
    }
}

//
// Private slots
//

void AudioOutput::slotHandleNotify() {

    if( audioOutInterface->state() != QAudio::IdleState && audioOutInterface->error() != QAudio::UnderrunError ) {
        // qCDebug( phxAudioOutput ) << "\t16ms consumed";
        outputBufferPos -= audioFormatOut.bytesForDuration( 16 * 1000 );
    }

}

void AudioOutput::slotAudioOutputStateChanged( QAudio::State s ) {

    if( s == QAudio::IdleState && audioOutInterface->error() == QAudio::UnderrunError ) {
        qWarning( phxAudioOutput ) << "audioOut underrun";

        if( audioOutInterface ) {
            outputBuffer.write( silence, audioFormatOut.bytesForDuration( 20 * 1000 ) );
            audioOutInterface->start( &outputBuffer );
        } else {
            // resetAudio();
        }
    }

    if( s != QAudio::IdleState && s != QAudio::ActiveState ) {
        qCDebug( phxAudioOutput ) << "State changed:" << s;
    }

}

//
// Private
//

void AudioOutput::resetAudio() {

    // Reset the resampler

    if( resamplerState ) {
        src_delete( resamplerState );
        resamplerState = nullptr;
    }

    int errorCode;
    resamplerState = src_new( SRC_SINC_BEST_QUALITY, 2, &errorCode );

    if( !resamplerState ) {
        qCWarning( phxAudioOutput ) << "libresample could not init: " << src_strerror( errorCode ) ;
    }

    // Reset the output interface object

    if( audioOutInterface ) {
        audioOutInterface->stop();
        delete audioOutInterface;
        audioOutInterface = nullptr;
    }

    audioOutInterface = new QAudioOutput( audioFormatOut, this );
    Q_CHECK_PTR( audioOutInterface );

    // Granulatiry of our buffer position checker
    /*int notifyInterval = 16;
    connect( audioOutInterface, &QAudioOutput::notify, this, &AudioOutput::slotHandleNotify );
    audioOutInterface->setNotifyInterval( notifyInterval );

    if( audioOutInterface->notifyInterval() != notifyInterval ) {
        qCWarning( phxAudioOutput ) << "notifyInterval is" << audioOutInterface->notifyInterval() << "ms, not" << notifyInterval << "ms!";
    }*/

    connect( audioOutInterface, &QAudioOutput::stateChanged, this, &AudioOutput::slotAudioOutputStateChanged );
    // audioOutIODev = audioOutInterface->start();
    audioOutInterface->start( &outputBuffer );
    QAudio::State state = audioOutInterface->state();
    qCDebug( phxAudioOutput ) << state;

    //if( !coreIsRunning ) {
    //    audioOutInterface->suspend();
    //}

    // Reallocate space for scratch space conversion buffers

    auto outputBufferSizeSamples = audioOutInterface->bufferSize() * 2;
    qCDebug( phxAudioOutput ) << "Allocated" << outputBufferSizeSamples * 2 * 2 / 1024 << "kb for conversion.";

    if( inputDataFloat ) {
        delete inputDataFloat;
        inputDataFloat = nullptr;
    }

    if( outputDataFloat ) {
        delete outputDataFloat;
        outputDataFloat = nullptr;
    }

    if( outputDataShort ) {
        delete outputDataShort;
        outputDataShort = nullptr;
    }

    // Make the input buffer a bit bigger as the amount of data coming in may be a bit more than a frame's worth
    inputDataFloat = new float[( int )( ( double )sampleRate / coreFPS ) * 3];
    outputDataFloat = new float[outputBufferSizeSamples];
    outputDataShort = new short[outputBufferSizeSamples];

    // Seed the output buffer with a bit of silence

    outputBuffer.write( silence, 30000 );
    outputBufferPos = 30000;

}
