#include "core.h"
#include "phoenixglobals.h"

/*
QDebug operator<<( QDebug debug, const CoreVariable &var ) {

    // join a QVector of std::strings. (Really, C++ ?)
    auto &choices = var.choices();
    std::string joinedchoices;

    foreach( auto &choice, choices ) {
        joinedchoices.append( choice );

        if( &choice != &choices.last() ) {
            joinedchoices.append( ", " );
        }
    }

    auto QStr = QString::fromStdString; // shorter alias



    debug << qPrintable( QString( "Core::Variable(%1=%2, description=\"%3\", choices=[%4])" ).
                         arg( QStr( var.key() ) ).arg( QStr( var.value( "<not set>" ) ) ).
                         arg( QStr( var.description() ) ).arg( QStr( joinedchoices ) ) );
    return debug;

}
*/

LibretroSymbols::LibretroSymbols()
    : retro_audio( nullptr ),
      retro_audio_set_state( nullptr ),
      retro_frame_time( nullptr ),
      retro_keyboard_event( nullptr ) {

}

//
// Public
//

Core::Core()
    : pixelFormat( RETRO_PIXEL_FORMAT_RGB565 ),
      SRAMDataRaw( nullptr ),
      currentFrameBuffer( nullptr ),
      currentOpenGLContext( nullptr ),
      updateCoreVariables( false ),
      qmlRenderType( RenderType::Software ),
      qmlNeedsGame( true )
{

    Core::core = this;

    setSaveDirectory( phxGlobals.savePath() );
    setSystemDirectory( phxGlobals.biosPath() );

    avInfo = new retro_system_av_info;
    systemInfo = new retro_system_info;

    audioPoolCurrentBuffer = 0;
    audioBufferCurrentByte = 0;
    videoBufferPoolIndex = 0;

    for( int i = 0; i < 30; i++ ) {

        audioBufferPool[i] = nullptr;

        videoBufferPool[i] = nullptr;

    }

    emit signalCoreStateChanged( STATEUNINITIALIZED, CORENOERROR );


}

Core::~Core() {

}

QString Core::stateToText( Core::State state ) {

    switch( state ) {

        case STATEUNINITIALIZED:
            return "Uninitialized";

        case STATEREADY:
            return "Ready";

        case STATEFINISHED:
            return "Finished";

        case STATEERROR:
            return "Error";

    }

    return "Unknown";

}

//
// Slots
//

void Core::slotLoadCore( QString path ) {

    libraryPath = QUrl( path ).toLocalFile();

    qCDebug( phxCore ) << "slotLoadCore(" << libraryPath << ")";

    libretroCore.setFileName( libraryPath );
    libretroCore.load();

    if( libretroCore.isLoaded() ) {

        libraryFilename = libretroCore.fileName().toLocal8Bit();

        // Resolve symbols
        resolved_sym( retro_set_environment );
        resolved_sym( retro_set_video_refresh );
        resolved_sym( retro_set_audio_sample );
        resolved_sym( retro_set_audio_sample_batch );
        resolved_sym( retro_set_input_poll );
        resolved_sym( retro_set_input_state );
        resolved_sym( retro_init );
        resolved_sym( retro_deinit );
        resolved_sym( retro_api_version );
        resolved_sym( retro_get_system_info );
        resolved_sym( retro_get_system_av_info );
        resolved_sym( retro_set_controller_port_device );
        resolved_sym( retro_reset );
        resolved_sym( retro_run );
        resolved_sym( retro_serialize );
        resolved_sym( retro_serialize_size );
        resolved_sym( retro_unserialize );
        resolved_sym( retro_cheat_reset );
        resolved_sym( retro_cheat_set );
        resolved_sym( retro_load_game );
        resolved_sym( retro_load_game_special );
        resolved_sym( retro_unload_game );
        resolved_sym( retro_get_region );
        resolved_sym( retro_get_memory_data );
        resolved_sym( retro_get_memory_size );

        // Set callbacks
        symbols.retro_set_environment( environmentCallback );
        symbols.retro_set_audio_sample( audioSampleCallback );
        symbols.retro_set_audio_sample_batch( audioSampleBatchCallback );
        symbols.retro_set_input_poll( inputPollCallback );
        symbols.retro_set_input_state( inputStateCallback );
        symbols.retro_set_video_refresh( videoRefreshCallback );
        //symbols.retro_get_memory_data( getMemoryData );
        //symbols.retro_get_memory_size( getMemorySize );

        // Init the core
        symbols.retro_init();


        // Get some info about the game
        symbols.retro_get_system_info( systemInfo );
        fullPathNeeded = systemInfo->need_fullpath;

        if ( !needsGame() ) {
            symbols.retro_load_game( nullptr );
            symbols.retro_get_system_av_info( avInfo );
            core->emitStateReady();
        }

    }

    else {
        emit signalCoreStateChanged( STATEERROR, COREUNKNOWNERROR );
    }

}

void Core::slotLoadGame( const QString path ) {

    gamePath = QUrl( path ).toLocalFile();

    retro_game_info gameInfo;

    qCDebug( phxCore ) << "slotLoadGame(" << gamePath << ")";

    // Argument struct for symbols.retro_load_game()

    QFileInfo info( gamePath );

    // Full path needed, simply pass the game's file path to the core
    if( fullPathNeeded ) {
        gameInfo.path = gamePath.toUtf8().constData();
        gameInfo.data = nullptr;
        gameInfo.size = 0;
        gameInfo.meta = "";
    }

    // Full path not needed, read the file to a buffer and pass that to the core
    else {
        QFile game( info.canonicalFilePath() );

        if( !game.open( QIODevice::ReadOnly ) ) {
            ;// TODO: Set error state
        }

        // read into memory
        gameData = game.readAll();

        gameInfo.path = nullptr;
        gameInfo.data = gameData.data();
        gameInfo.size = game.size();
        gameInfo.meta = "";

    }

    if( !symbols.retro_load_game( &gameInfo ) ) {

        emit signalCoreStateChanged( Core::STATEERROR, Core::GAMEUNKNOWNERROR );

    }

    // Get the AV timing/dimensions/format
    symbols.retro_get_system_av_info( avInfo );

    // loadSRAM();

    // Allocate buffers now that we know how large to make them
    // Assume 16-bit stereo audio, 32-bit video
    for( int i = 0; i < 30; i++ ) {

        // Allocate a bit extra as some cores' numbers do not add up...
        audioBufferPool[i] = ( int16_t * )calloc( 1, avInfo->timing.sample_rate * 5 );

        videoBufferPool[i] = ( uchar * )calloc( 1, avInfo->geometry.max_width * avInfo->geometry.max_height * 4 );

    }

    core->emitStateReady();

}

void Core::slotFrame() {

    // Tell the core to run a frame
    symbols.retro_run();

    // This should never be used...
    /*if( symbols.retro_audio ) {
        symbols.retro_audio();
    }*/

}

void Core::slotShutdown() {

    qCDebug( phxCore ) << "slotShutdown() start";

    // saveSRAM();

    // symbols.retro_audio is the first symbol set to null in the constructor, so check that one
    if( symbols.retro_audio ) {
        symbols.retro_unload_game();
        symbols.retro_deinit();
    }

    gameData.clear();
    libretroCore.unload();
    libraryFilename.clear();

    // Make sure the CoreVariables are pointers, or else this will segfault.

    delete avInfo;
    delete systemInfo;

    for( int i = 0; i < 30; i++ ) {

        if( audioBufferPool[i] ) {
            free( audioBufferPool[i] );
        }

        if( videoBufferPool[i] ) {
            free( videoBufferPool[i] );
        }

    }

    emit signalCoreStateChanged( STATEFINISHED, CORENOERROR );

    qCDebug( phxCore ) << "slotShutdown() end";

}

void Core::setVariable(const QString key, const QString value)
{
    if ( variables.contains( key ) ) {
        auto vari = variables[ key ];
        vari.setValue( value );

        variables[key] = vari;

        updateCoreVariables = true;

    }

}

//
// Protected
//

// Must always point to the last Core instance used
Core *Core::core = nullptr;

void Core::emitStateReady() {

    emit signalAVFormat( *avInfo, pixelFormat );
    emit signalCoreStateChanged( Core::STATEREADY, Core::CORENOERROR );

}

void Core::emitAudioData( int16_t *data , int bytes ) {

    emit signalAudioData( data, bytes );

}

void Core::emitVideoData( uchar *data, unsigned width, unsigned height, size_t pitch ) {

    emit signalVideoData( data, width, height, pitch );

}

//
// Private
//


// Save states

void Core::loadSRAM() {

    SRAMDataRaw = symbols.retro_get_memory_data( RETRO_MEMORY_SAVE_RAM );

    QFile file( saveDirectory() + phxGlobals.selectedGame().baseName() + ".srm" );

    if( file.open( QIODevice::ReadOnly ) ) {
        QByteArray data = file.readAll();
        memcpy( SRAMDataRaw, data.data(), data.size() );

        qCDebug( phxCore ) << "Loading SRAM from: " << file.fileName();
        file.close();
    }

}

void Core::saveSRAM() {

    if( SRAMDataRaw == nullptr ) {
        return;
    }

    QFile file( saveDirectory() + phxGlobals.selectedGame().baseName() + ".srm" );
    qCDebug( phxCore ) << "Saving SRAM to: " << file.fileName();

    if( file.open( QIODevice::WriteOnly ) ) {
        char *data = static_cast<char *>( SRAMDataRaw );
        size_t size = symbols.retro_get_memory_size( RETRO_MEMORY_SAVE_RAM );
        file.write( data, size );
        file.close();
    }

}

// SRAM

/*bool Core::loadGameState( QString path, QString name ) {
    Q_UNUSED( path );
    Q_UNUSED( name );

    QFile file( phxGlobals.savePath() + phxGlobals.selectedGame().baseName() + "_STATE.sav" );

    bool loaded = false;

    if( file.open( QIODevice::ReadOnly ) ) {
        QByteArray state = file.readAll();
        void *data = state.data();
        size_t size = static_cast<int>( state.size() );

        file.close();

        if( symbols.retro_unserialize( data, size ) ) {
            qCDebug( phxCore ) << "Save State loaded";
            loaded = true;
        }
    }

    return loaded;

}*/

/*bool Core::saveGameState( QString path, QString name ) {
    Q_UNUSED( path );
    Q_UNUSED( name );

    size_t size = core->symbols.retro_serialize_size();

    if( !size ) {
        return false;
    }

    char *data = new char[size];
    bool loaded = false;

    if( symbols.retro_serialize( data, size ) ) {
        QFile *file = new QFile( phxGlobals.savePath() + phxGlobals.selectedGame().baseName() + "_STATE.sav" );
        qCDebug( phxCore ) << file->fileName();

        if( file->open( QIODevice::WriteOnly ) ) {
            file->write( QByteArray( static_cast<char *>( data ), static_cast<int>( size ) ) );
            qCDebug( phxCore ) << "Save State wrote to " << file->fileName();
            file->close();
            loaded = true;
        }

        delete file;

    }

    delete[] data;
    return loaded;

}*/

// Callbacks

void Core::audioSampleCallback( int16_t left, int16_t right ) {

    Core *core = Core::core;

    // Sanity check
    Q_ASSERT( core->audioBufferCurrentByte < core->avInfo->timing.sample_rate * 4 );

    // Stereo audio is interleaved, left then right
    core->audioBufferPool[core->audioPoolCurrentBuffer][core->audioBufferCurrentByte / 2] = left;
    core->audioBufferPool[core->audioPoolCurrentBuffer][core->audioBufferCurrentByte / 2 + 1] = right;

    // Each frame is 4 bytes (16-bit stereo)
    core->audioBufferCurrentByte += 4;

}

size_t Core::audioSampleBatchCallback( const int16_t *data, size_t frames ) {

    Core *core = Core::core;
/*
    // Sanity check
    Q_ASSERT( core->audioBufferCurrentByte < core->avInfo->timing.sample_rate * 4 );

    // Need to do a bit of pointer arithmetic to get the right offset (the buffer is counted in increments of 2 bytes)
    int16_t *dst_init = core->audioBufferPool[core->audioPoolCurrentBuffer];
    int16_t *dst = dst_init + ( core->audioBufferCurrentByte / 2 );

    // Copy the incoming data
    memcpy( dst, data, frames * 4 );

    // Each frame is 4 bytes (16-bit stereo)
    core->audioBufferCurrentByte += frames * 4;
*/
    return frames;

}

bool Core::environmentCallback( unsigned cmd, void *data ) {

    switch( cmd ) {
        case RETRO_ENVIRONMENT_SET_ROTATION: // 1
            qDebug() << "\tRETRO_ENVIRONMENT_SET_ROTATION (1)";
            break;

        case RETRO_ENVIRONMENT_GET_OVERSCAN: // 2
            qDebug() << "\tRETRO_ENVIRONMENT_GET_OVERSCAN (2) (handled)";
            // Crop away overscan
            return true;

        case RETRO_ENVIRONMENT_GET_CAN_DUPE: // 3
            *( bool * )data = true;
            return true;

        // 4 and 5 have been deprecated

        case RETRO_ENVIRONMENT_SET_MESSAGE: // 6
            qDebug() << "\tRETRO_ENVIRONMENT_SET_MESSAGE (6)";
            break;

        case RETRO_ENVIRONMENT_SHUTDOWN: // 7
            qDebug() << "\tRETRO_ENVIRONMENT_SHUTDOWN (7)";
            break;

        case RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL: // 8
            qDebug() << "\tRETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL (8)";
            break;

        case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY: {// 9
            qCDebug( phxCore ) << "\tRETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY (9)";
            auto saveDir = core->systemDirectory().toLocal8Bit();
            *static_cast<const char **>( data ) = saveDir.constData();
            return true;
        }

        case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: { // 10
            qDebug() << "\tRETRO_ENVIRONMENT_SET_PIXEL_FORMAT (10) (handled)";

            retro_pixel_format *pixelformat = ( enum retro_pixel_format * )data;
            Core::core->pixelFormat = *pixelformat;

            switch( *pixelformat ) {
                case RETRO_PIXEL_FORMAT_0RGB1555:
                    qDebug() << "\t\tPixel format: 0RGB1555\n";
                    return true;

                case RETRO_PIXEL_FORMAT_RGB565:
                    qDebug() << "\t\tPixel format: RGB565\n";


                case RETRO_PIXEL_FORMAT_XRGB8888:
                    qDebug() << "\t\tPixel format: XRGB8888\n";
                    return true;

                default:
                    qDebug() << "\t\tError: Pixel format is not supported. (" << pixelformat << ")";
                    break;
            }

            return false;
        }

        case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS: // 11
            qDebug() << "\tRETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS (11) (handled)";
            Core::core->retropadToStringMap = *( retro_input_descriptor * )data;
            return true;

        case RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK: // 12
            qDebug() << "\tRETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK (12) (handled)";
            Core::core->symbols.retro_keyboard_event = ( decltype( LibretroSymbols::retro_keyboard_event ) )data;
            break;

        case RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE: // 13
            qDebug() << "\tRETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE (13)";
            break;

        case RETRO_ENVIRONMENT_SET_HW_RENDER: {// 14
            qDebug() << "\tRETRO_ENVIRONMENT_SET_HW_RENDER (14) (handled)";
            auto *cb = ( retro_hw_render_callback * )data;

            core->setRenderType( OpenGL );

            switch( cb->context_type ) {
                case RETRO_HW_CONTEXT_NONE:
                    qDebug() << "No hardware context was selected";
                    return false;

                case RETRO_HW_CONTEXT_OPENGL:
                    qDebug() << "OpenGL 2 context was selected";
                    cb->version_major = 2;
                    cb->version_minor = 0;
                    break;

                case RETRO_HW_CONTEXT_OPENGLES2:
                    qDebug() << "OpenGL ES 2 context was selected";
                    cb->context_type = RETRO_HW_CONTEXT_OPENGLES2;
                    break;

                case RETRO_HW_CONTEXT_OPENGLES3:
                    qDebug() << "OpenGL 3 context was selected";
                    cb->version_major = 3;
                    cb->version_minor = 0;
                    break;

                default:
                    qCritical() << "RETRO_HW_CONTEXT: " << cb->context_type << " was not handled";
                    return false;
            }

            cb->get_current_framebuffer = core->currentFrameBufferID;
            cb->get_proc_address = core->procAddress;
            Core::core->libretroOpenGLCallback = *cb;

            return true;
        }

        case RETRO_ENVIRONMENT_GET_VARIABLE: { // 15
            auto *rv = static_cast<struct retro_variable *>( data );

            if( core->variables.contains( rv->key ) ) {
                const auto var = core->variables[rv->key];

                if( var.isValid() ) {
                    rv->value = var.value().toStdString().c_str();
                }
            }

            return true;
        }

        case RETRO_ENVIRONMENT_SET_VARIABLES: { // 16
            qCDebug( phxCore ) << "SET_VARIABLES:";
            auto *rv = static_cast<const struct retro_variable *>( data );

            int i = 0;
            for( ; rv->key != NULL; rv++ ) {
                CoreVariable v( rv );
                core->variables.insert( v.key(), v );
                core->emitUpdateVariables( v.key(), v.value(), v.description(), v.choices(), i );
                //qCDebug( phxCore ) << "\t" << v;
                i++;
            }

            return true;
        }

        case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE: {// 17
            //qDebug() << "\tRETRO_ENVIRONMENT_GET_VARIABLE_UPDATE (17)";
            bool *update = static_cast<bool *>( data );

            if ( core->updateCoreVariables ) {
                *update = true;
                core->updateCoreVariables = false;
                return true;
            }

            *update = false;
            return false;
        }

        case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME: // 18
            qDebug() << "\tRETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME (18)";
            core->setNeedsGame( !*static_cast<bool *>( data ) );
            break;

        case RETRO_ENVIRONMENT_GET_LIBRETRO_PATH: // 19
            *static_cast<const char **>( data ) = core->libraryFilename.constData();
            break;

        // 20 has been deprecated

        case RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK: // 21
            qDebug() << "RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK (21) (handled)";
            Core::core->symbols.retro_frame_time = ( decltype( LibretroSymbols::retro_frame_time ) )data;
            break;

        case RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK: {// 22
            retro_audio_callback *cb = static_cast<retro_audio_callback *>( data );
            Q_UNUSED( cb )
            qDebug() << "\tRETRO_ENVIRONMENT_SET_AUDIO_CALLBACK (22)";
            break;
        }

        case RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE: // 23
            qDebug() << "\tRETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE (23)";
            break;

        case RETRO_ENVIRONMENT_GET_INPUT_DEVICE_CAPABILITIES: // 24
            qDebug() << "\tRETRO_ENVIRONMENT_GET_INPUT_DEVICE_CAPABILITIES (24)";
            break;

        case RETRO_ENVIRONMENT_GET_SENSOR_INTERFACE: // 25
            qDebug() << "\tRETRO_ENVIRONMENT_GET_SENSOR_INTERFACE (25)";
            break;

        case RETRO_ENVIRONMENT_GET_CAMERA_INTERFACE: // 26
            qDebug() << "\tRETRO_ENVIRONMENT_GET_CAMERA_INTERFACE (26)";
            break;

        case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: {// 27
            qDebug() << "\tRETRO_ENVIRONMENT_GET_LOG_INTERFACE (27) (handled)";
            struct retro_log_callback *logcb = ( struct retro_log_callback * )data;
            logcb->log = logCallback;
            return true;
        }

        case RETRO_ENVIRONMENT_GET_PERF_INTERFACE: {// 28
            retro_perf_callback *cb = static_cast<retro_perf_callback *>( data );
            cb->get_cpu_features = 0;
            cb->get_perf_counter = 0;
            cb->get_time_usec = 0;
            cb->perf_log = 0;
            cb->perf_register = 0;
            cb->perf_start = 0;
            cb->perf_stop = 0;
            qDebug() << "\tRETRO_ENVIRONMENT_GET_PERF_INTERFACE (28)";
            break;
        }

        case RETRO_ENVIRONMENT_GET_LOCATION_INTERFACE: // 29
            qDebug() << "\tRETRO_ENVIRONMENT_GET_LOCATION_INTERFACE (29)";
            break;

        case RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY: {// 30
            auto dir = core->systemDirectory().toLocal8Bit();
            *static_cast<const char **>( data ) = dir.constData();
            qDebug() << "\tRETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY (30)";
            break;
        }

        case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY: {// 31
            qCDebug( phxCore ) << "\tRETRO_ENVIRONMENT_GET_saveDirectory (31) (handled)";
            auto byteArray = core->saveDirectory().toLocal8Bit();
            *static_cast<const char **>( data ) = byteArray.constData();
            qCDebug( phxCore ) << "Save Directory: " << byteArray;
            return true;
        }

        case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO: // 32
            qDebug() << "\tRETRO_ENVIRONMENT_SET_systemAVInfo (32)";
            break;

        case RETRO_ENVIRONMENT_SET_PROC_ADDRESS_CALLBACK: // 33
            qDebug() << "\tRETRO_ENVIRONMENT_SET_PROC_ADDRESS_CALLBACK (33)";
            break;

        case RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO: // 34
            qDebug() << "\tRETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO (34)";
            break;

        case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO: // 35
            qDebug() << "\tRETRO_ENVIRONMENT_SET_CONTROLLER_INFO (35)";
            break;

        default:
            qDebug() << "Error: Environment command " << cmd << " is not defined in the frontend's libretro.h!.";
            return false;
    }

    // Command was not handled
    return false;

}

void Core::inputPollCallback( void ) {

    // qDebug() << "Core::inputPollCallback";
    return;

}

void Core::logCallback( enum retro_log_level level, const char *fmt, ... ) {

    QVarLengthArray<char, 1024> outbuf( 1024 );
    va_list args;
    va_start( args, fmt );
    int ret = vsnprintf( outbuf.data(), outbuf.size(), fmt, args );

    if( ret < 0 ) {
        qCDebug( phxCore ) << "logCallback: could not format string";
        return;
    } else if( ( ret + 1 ) > outbuf.size() ) {
        outbuf.resize( ret + 1 );
        ret = vsnprintf( outbuf.data(), outbuf.size(), fmt, args );

        if( ret < 0 ) {
            qCDebug( phxCore ) << "logCallback: could not format string";
            return;
        }
    }

    va_end( args );

    // remove trailing newline, which are already added by qCDebug
    if( outbuf.value( ret - 1 ) == '\n' ) {
        outbuf[ret - 1] = '\0';

        if( outbuf.value( ret - 2 ) == '\r' ) {
            outbuf[ret - 2] = '\0';
        }
    }

    switch( level ) {
        case RETRO_LOG_DEBUG:
            qCDebug( phxCore ) << outbuf.data();
            break;

        case RETRO_LOG_INFO:
            qCDebug( phxCore ) << outbuf.data();
            break;

        case RETRO_LOG_WARN:
            qCWarning( phxCore ) << outbuf.data();
            break;

        case RETRO_LOG_ERROR:
            qCCritical( phxCore ) << outbuf.data();
            break;

        default:
            qCWarning( phxCore ) << outbuf.data();
            break;
    }

}

int16_t Core::inputStateCallback( unsigned port, unsigned device, unsigned index, unsigned id ) {
    Q_UNUSED( port )
    Q_UNUSED( device )
    Q_UNUSED( index )
    Q_UNUSED( id )

    return 0;

}

void Core::videoRefreshCallback( const void *data, unsigned width, unsigned height, size_t pitch ) {

    // No need to emit a signal when the hardware callback is used.
    if ( data == RETRO_HW_FRAME_BUFFER_VALID ) {
        return;
    }

    // Current frame exists, send it on its way
    else if( data ) {
        memcpy( core->videoBufferPool[core->videoBufferPoolIndex], data, height * pitch );
        core->emitVideoData( core->videoBufferPool[core->videoBufferPoolIndex], width, height, pitch );
        core->videoBufferPoolIndex = ( core->videoBufferPoolIndex + 1 ) % 30;

    }

    // Current frame is a dupe, send the last actual frame again
    else {

        core->emitVideoData( core->videoBufferPool[core->videoBufferPoolIndex], width, height, pitch );

    }

    // Flush the audio used so far
    core->emitAudioData( core->audioBufferPool[core->audioPoolCurrentBuffer], core->audioBufferCurrentByte );
    core->audioBufferCurrentByte = 0;
    core->audioPoolCurrentBuffer = ( core->audioPoolCurrentBuffer + 1 ) % 30;

    return;

}
