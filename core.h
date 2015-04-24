#ifndef CORE_H
#define CORE_H

#include <QtCore>
#include <QFile>
#include <QString>
#include <QByteArray>
#include <QImage>
#include <QMap>
#include <QLibrary>
#include <QObject>
#include <QThread>
#include <QTimer>

#include "libretro.h"
#include "logging.h"

/* The Core class is a wrapper around any given libretro core.
 * The general functionality for this class is to load the core into memory,
 * connect to all of the core's callbacks, such as video and audio rendering,
 * and generate frames of video and audio data in the raw memory format.
 *
 * The Core class is instantiated inside of the VideoItem class, which lives in the videoitem.cpp file.
 * Currently this approach only supports loading a single core and game at any one time.
 *
 * Check out the static callbacks in order to see how data is passed from the core, to the screen.
 *
 */

// Helper for resolving libretro methods
#define resolved_sym( name ) symbols.name = ( decltype( symbols.name ) )libretroCore.resolve( #name );

struct LibretroSymbols {

    LibretroSymbols();

    // Libretro core functions
    unsigned( *retro_api_version )( void );
    void ( *retro_cheat_reset )( void );
    void ( *retro_cheat_set )( unsigned , bool , const char * );
    void ( *retro_deinit )( void );
    void *( *retro_get_memory_data )( unsigned );
    size_t ( *retro_get_memory_size )( unsigned );
    unsigned( *retro_get_region )( void );
    void ( *retro_get_system_av_info )( struct retro_system_av_info * );
    void ( *retro_get_system_info )( struct retro_system_info * );
    void ( *retro_init )( void );
    bool ( *retro_load_game )( const struct retro_game_info * );
    bool ( *retro_load_game_special )( unsigned , const struct retro_game_info *, size_t );
    void ( *retro_reset )( void );
    void ( *retro_run )( void );
    bool ( *retro_serialize )( void *, size_t );
    size_t ( *retro_serialize_size )( void );
    void ( *retro_unload_game )( void );
    bool ( *retro_unserialize )( const void *, size_t );

    // Frontend-defined callbacks
    void ( *retro_set_audio_sample )( retro_audio_sample_t );
    void ( *retro_set_audio_sample_batch )( retro_audio_sample_batch_t );
    void ( *retro_set_controller_port_device )( unsigned, unsigned );
    void ( *retro_set_environment )( retro_environment_t );
    void ( *retro_set_input_poll )( retro_input_poll_t );
    void ( *retro_set_input_state )( retro_input_state_t );
    void ( *retro_set_video_refresh )( retro_video_refresh_t );

    // Optional core-defined callbacks
    void ( *retro_audio )();
    void ( *retro_audio_set_state )( bool enabled );
    void ( *retro_frame_time )( retro_usec_t delta );
    void ( *retro_keyboard_event )( bool down, unsigned keycode, uint32_t character, uint16_t key_modifiers );

};

class FrameData {
public:
    uchar *data = nullptr;
    unsigned height = 0;
    unsigned width = 0;
    size_t pitch = 0;

    FrameData(uchar *_data, unsigned _height, unsigned _width, size_t _pitch)
    {
        data = _data;
        height = _height;
        width = _width;
        pitch = _pitch;
    }

};

class AudioData {
public:
    char *data = nullptr;
    size_t size = 0;

    AudioData( char *_data, int16_t _size)
    {
        data = _data;
        size = _size;
    }
};

class Core: public QObject {
    Q_OBJECT

public:

    Core();
    ~Core();

    enum CoreState {
        Running = 0,
        NeedsCore,
        NeedsGame,
        Unloaded,
        Loading,
        ShutDown,
        SaveRAM,
        LoadRAM,
    };

    Core::CoreState state() const
    {
        return currentState;
    }

    // Load a libretro core at the given path
    // Returns: true if successful, false otherwise
    bool loadCore( const char *path );

    // Load a game with the given path
    // Returns: true if the game was successfully loaded, false otherwise
    bool loadGame( const char *path );

    // Run core for one frame
    void doFrame();

    //
    // Misc
    //

    // A pointer to the last instance of the class used
    static Core *core;

    QByteArray getLibraryName() {
        return libraryName;
    }

    bool saveGameState( QString save_path, QString game_name );
    bool loadGameState( QString save_path, QString game_name );

    //
    // Video
    //

    retro_hw_render_callback getHWData() const {
        return hw_callback;
    }


    retro_pixel_format getPixelFormat() const {
        return pixel_format;
    }
    const retro_system_info *getSystemInfo() const {
        return system_info;
    }
    float getAspectRatio() const {
        if( system_av_info->geometry.aspect_ratio ) {
            return system_av_info->geometry.aspect_ratio;
        }

        return ( qreal )system_av_info->geometry.base_width / system_av_info->geometry.base_height;
    }

    void setSystemDirectory( QString system_directory );
    void setSaveDirectory( QString save_directory );

    //
    // Timing
    //

    double getFps() const {
        return system_av_info->timing.fps;
    }
    double getSampleRate() const {
        return system_av_info->timing.sample_rate;
    }
    bool isDupeFrame() const {
        return is_dupe_frame;
    }

    // Container class for a libretro core variable
    class Variable {
        public:
            Variable() {} // default constructor

            Variable( const retro_variable *var ) {
                m_key = var->key;

                // "Text before first ';' is description. This ';' must be followed by a space,
                // and followed by a list of possible values split up with '|'."
                QString valdesc( var->value );
                int splitidx = valdesc.indexOf( "; " );

                if( splitidx != -1 ) {
                    m_description = valdesc.mid( 0, splitidx ).toStdString();
                    auto _choices = valdesc.mid( splitidx + 2 ).split( '|' );

                    foreach( auto &choice, _choices ) {
                        m_choices.append( choice.toStdString() );
                    }
                } else {
                    // unknown value
                }
            };
            virtual ~Variable() {};

            const std::string &key() const {
                return m_key;
            };

            const std::string &value( const std::string &default_ ) const {
                if( m_value.empty() ) {
                    return default_;
                }

                return m_value;
            };

            const std::string &value() const {
                static std::string default_( "" );
                return value( default_ );
            }

            const std::string &description() const {
                return m_description;
            };

            const QVector<std::string> &choices() const {
                return m_choices;
            };

            bool isValid() const {
                return !m_key.empty();
            };

        private:
            // use std::strings instead of QStrings, since the later store data as 16bit chars
            // while cores probably use ASCII/utf-8 internally..
            std::string m_key;
            std::string m_value; // XXX: value should not be modified from the UI while a retro_run() call happens
            std::string m_description;
            QVector<std::string> m_choices;

    };

signals:
    void signalVideoRefreshCallback( FrameData *frame );
    void signalCallbackAudioData( AudioData *audioData );
    void signalTimeOut();
    void signalCoreStateChanged( Core::CoreState );
    void signalRunning( bool );

public slots:

    //
    // Control
    //

    void slotStartCoreThread( QThread::Priority priority )
    {
        if (!coreThread.isRunning())
            coreThread.start( priority );

        unsigned finishedImmediately = 0;

        coreTimer.start( finishedImmediately );
    }

    void slotCoreThreadTimeOut()
    {
        quint64 timeout = (1 / (qreal) core->getFps() ) * 1000000;

        QThread::usleep(timeout);
        emit signalTimeOut();
    }

    void slotDoFrame()
    {
        doFrame();
    }

    void slotCallbackFrameData( FrameData *frame )
    {
        emit signalVideoRefreshCallback( frame );
    }

    void slotCallbackAudioData( AudioData *audioData )
    {
        emit signalCallbackAudioData( audioData );
    }

    void slotRefreshCoreState()
    {
        system_av_info = new retro_system_av_info();
        system_info = new retro_system_info();
    }


    void slotHandleCoreStateChanged( Core::CoreState state )
    {
        currentState = state;

        switch ( state ) {

        case Unloaded:
            QMetaObject::invokeMethod( &coreTimer, "stop" );
            qCDebug( phxCore ) << "Began unloading core";

            emit signalCoreStateChanged( Core::SaveRAM );

            symbols.retro_unload_game();
            symbols.retro_deinit();
            libretroCore.unload();
            gameBuffer.clear();
            libraryName.clear();

            delete system_av_info;
            delete system_info;

            break;

        case Loading:
            Core::core = this;

            system_av_info = new retro_system_av_info;
            system_info = new retro_system_info;

            pixel_format = RETRO_PIXEL_FORMAT_UNKNOWN;

            is_dupe_frame = false;
            saveRAMPointer = nullptr;

            break;

        case ShutDown:
            emit signalCoreStateChanged( Core::Unloaded );
            this->deleteLater();

        case SaveRAM:
            saveSRAM();

        case LoadRAM:
            loadSRAM();

        case Running:
            //slotStartCoreThread( QThread::TimeCriticalPriority );
            break;

        default:
            break;
        }

    }

    //bool slotLoadCore( const char *path );
    //bool slotLoadGame( const char *path );
    //void slotSetSystemDirectory( QString system_directory );
    //void slotSetSaveDirectory( QString save_directory );

    //void slotDoFrame();



private:
    // Handle to the libretro core
    QThread coreThread;
    QTimer coreTimer;

    QLibrary libretroCore;
    QByteArray libraryName;

    QFileInfo gameInfo;


    CoreState currentState;
    // Struct containing libretro methods
    LibretroSymbols symbols;

    // Information about the core
    retro_system_av_info *system_av_info;
    retro_system_info *system_info;
    QMap<std::string, Core::Variable> variables;

    // Do something with retro_variable
    retro_input_descriptor input_descriptor;
    retro_game_geometry game_geometry;
    retro_system_timing system_timing;
    retro_hw_render_callback hw_callback;
    bool full_path_needed;
    QByteArray system_directory;
    QByteArray save_directory;

    // Game
    QByteArray gameBuffer;

    // Video

    retro_pixel_format pixel_format;

    // Timing
    bool is_dupe_frame;

    // Misc
    void *saveRAMPointer;
    void saveSRAM();
    void loadSRAM();

    // Callbacks
    static void audioSampleCallback( int16_t left, int16_t right );
    static size_t audioSampleBatchCallback( const int16_t *data, size_t frames );
    static bool environmentCallback( unsigned cmd, void *data );
    static void inputPollCallback( void );
    static void logCallback( enum retro_log_level level, const char *fmt, ... );
    static int16_t inputStateCallback( unsigned port, unsigned device, unsigned index, unsigned id );
    static void videoRefreshCallback( const void *data, unsigned width, unsigned height, size_t pitch );
};

// Do not scope this globally anymore, it is not thread-safe
// QDebug operator<<( QDebug, const Core::Variable & );

#endif
