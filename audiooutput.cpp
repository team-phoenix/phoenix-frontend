
#include "audiooutput.h"

AudioOutput::AudioOutput()
    : resamplerState( nullptr ),
      inputDataFloat( nullptr ),
      inputDataChar( nullptr ),
      outputDataFloat( nullptr ),
      outputDataShort( nullptr ),
      coreIsRunning( false ),
      audioOutInterface( nullptr ),
      audioOutIODev( nullptr ),
      outputBufferPos( 0 ) {

}

AudioOutput::~AudioOutput() {

    if( audioOutInterface ) {

        audioOutInterface->deleteLater();

    }

    if( outputDataFloat ) {
        delete outputDataFloat;
    }

    if( outputDataShort ) {
        delete outputDataShort;
    }

}

void AudioOutput::slotAudioFormat( int sampleRate, double coreFPS, double hostFPS ) {

    this->sampleRate = sampleRate;
    this->coreFPS = coreFPS;
    this->hostFPS = hostFPS;

    qCDebug( phxAudioOutput, "slotAudioFormat( %i Hz, %f fps (core), %f fps (host) )", sampleRate, coreFPS, hostFPS );

    QAudioDeviceInfo info( QAudioDeviceInfo::defaultOutputDevice() );

    audioFormatIn.setSampleSize( 16 );
    audioFormatIn.setSampleRate( sampleRate );
    audioFormatIn.setChannelCount( 2 );
    audioFormatIn.setSampleType( QAudioFormat::SignedInt );
    audioFormatIn.setByteOrder( QAudioFormat::LittleEndian );
    audioFormatIn.setCodec( "audio/pcm" );

    // Try using the nearest supported format
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

void AudioOutput::slotAudioData( int16_t *data ) {

    // Handle the situation where there is an error opening the audio device
    if( audioOutInterface->error() == QAudio::OpenError ) {
        qWarning( phxAudioOutput ) << "QAudio::OpenError, attempting reset...";
        resetAudio();
    }

    auto samplesPerFrame = 2;

    // Max number of bytes/frames we can write to the output
    int outputBytesFree = audioOutInterface->bufferSize() - outputBufferPos;
    int outputFramesFree = audioFormatOut.framesForBytes( outputBytesFree );
    int outputSamplesFree = outputFramesFree * samplesPerFrame;
    Q_UNUSED( outputSamplesFree );

    //
    // TODO: Make these configurable
    //

    // Last term is the absolute bare minimum buffer target, add something to it to ensure no underruns
    int outputBufferTargetMs = ( double )2 * ( 1000.0 / coreFPS );

    // Max amount of stretching performed to compensate for output buffer position being off target
    double maxDeviation = 0.005;

    //
    // ---
    //

    int inputBytes = ( ( double )sampleRate / coreFPS ) * 4;
    int inputFrames = audioFormatIn.framesForBytes( inputBytes );
    int inputSamples = inputFrames * samplesPerFrame;

    // Calculate how much the read data should be scaled (shrunk or stretched) to keep the buffer on target
    int outputBufferTargetByte = audioFormatOut.bytesForDuration( outputBufferTargetMs * 1000 );
    int outputBufferVectorTargetToCurrent = outputBufferTargetByte - outputBufferPos;
    double bufferTargetCorrectionRatio = ( double )outputBufferVectorTargetToCurrent / outputBufferTargetByte;

    // Calculate the final DRC ratio
    double DRCRatio = 1.0 + maxDeviation * bufferTargetCorrectionRatio;
    double adjustedSampleRateRatio = sampleRateRatio * DRCRatio;

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
    int outputBytesWritten = audioOutIODev->write( ( char * ) outputDataShort, outputBytesConverted );
    outputBufferPos += outputBytesWritten;

    /*
    qCDebug( phxAudioOutput ) << "Output is" << ( ( ( double )( outputBufferPos ) / audioOutInterface->bufferSize() ) * 100 )
                              << "% full, DRC adjust =" << DRCRatio;
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
                              << "outputBufferVectorTargetToCurrent =" << outputBufferVectorTargetToCurrent
                              //<< "audioOut->bufferSize() =" << audioOut->bufferSize()
                              << "audioOutputBuffer->bytesToWrite() =" << audioOutIODev->bytesToWrite();
    qCDebug( phxAudioOutput ) << "\tOutput buffer is" << audioFormatOut.durationForBytes( audioOutInterface->bufferSize() ) / 1000
                              << "ms, buffer target is" << outputBufferTargetMs
                              << "ms (" << 100.0 * ( ( double )outputBufferTargetMs / ( audioFormatOut.durationForBytes( audioOutInterface->bufferSize() ) / 1000.0 ) )
                              << "%)"
                              << "audioOutInterface->bufferSize() =" << audioOutInterface->bufferSize() ;
    */

}

void AudioOutput::resetAudio() {

    // Reset the resampler

    if( resamplerState ) {
        src_delete( resamplerState );
    }

    int errorCode;
    resamplerState = src_new( SRC_SINC_BEST_QUALITY, 2, &errorCode );

    if( !resamplerState ) {
        qCWarning( phxAudioOutput ) << "libresample could not init: " << src_strerror( errorCode ) ;
    }

    // Re-create the output interface object
    if( audioOutInterface ) {
        audioOutInterface->stop();
        delete audioOutInterface;
    }

    audioOutInterface = new QAudioOutput( audioFormatOut, this );
    Q_CHECK_PTR( audioOutInterface );

    connect( audioOutInterface, &QAudioOutput::stateChanged, this, &AudioOutput::slotAudioOutputStateChanged );
    audioOutIODev = audioOutInterface->start();

    if( !coreIsRunning ) {
        audioOutInterface->suspend();
    }

    // Now that the IO devices are properly set up,
    // allocate space for buffers that'll hold up to their hardware couterpart's size in data
    auto outputDataSamples = audioOutInterface->bufferSize() * 2;
    qCDebug( phxAudioOutput ) << "Allocated" << outputDataSamples << "for conversion.";

    if( inputDataFloat ) {
        delete inputDataFloat;
    }

    if( outputDataFloat ) {
        delete outputDataFloat;
    }

    if( outputDataShort ) {
        delete outputDataShort;
    }

    inputDataFloat = new float[( int )( ( double )sampleRate / coreFPS ) * 2];
    outputDataFloat = new float[outputDataSamples];
    outputDataShort = new short[outputDataSamples];

    connect( audioOutInterface, &QAudioOutput::notify, this, &AudioOutput::slotHandleNotify );

    // Seed the output buffer with a bit of silence

    //audioOutIODev->write( silence, 3000 );
    //outputBufferPos = 3000;

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

void AudioOutput::slotAudioOutputStateChanged( QAudio::State s ) {

    if( s == QAudio::IdleState && audioOutInterface->error() == QAudio::UnderrunError ) {
        // qWarning( phxAudioOutput ) << "audioOut underrun";
        audioOutIODev = audioOutInterface->start();
    }

    if( s != QAudio::IdleState && s != QAudio::ActiveState ) {
        qCDebug( phxAudioOutput ) << "State changed:" << s;
    }

}

void AudioOutput::slotSetVolume( qreal level ) {
    if( audioOutInterface ) {
        audioOutInterface->setVolume( level );
    }
}

void AudioOutput::slotHandleNotify() {

    // qCDebug( phxAudioOutput ) << "\t50ms consumed";
    outputBufferPos -= audioFormatOut.bytesForDuration( 50 * 1000 );

}
