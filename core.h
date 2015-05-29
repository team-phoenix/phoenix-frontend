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
#include <QOpenGLFramebufferObject>
#include <QOpenGLContext>

#include <atomic>

#include "libretro.h"
#include "corevariable.h"
#include "logging.h"

/* Core is a class that manages the execution of a Libretro core and its associated game.
 *
 * Core is a state machine whose normal lifecycle goes like this:
 * Core::UNINITIALIZED, Core::READY, Core::FINISHED
 *
 * Core provides signalCoreStateChanged( newState, error ) to inform its controller that its state changed.
 *
 * Call Core's load methods with a valid path to a Libretro core and game, along with controller mappings, then call
 * slotInit() to begin loading the game and slot. Core should change to Core::READY and emit signalAVFormat() to inform
 * the controller and all consumers about what kind of data to expect from Core.
 *
 * You may now call slotFrame() to have the core emulate a video frame send out signals as data is produced.
 *
 * Call slotShutdown() to de-init the core and write any save games to disk. Core will then be in Core::FINISHED.
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

    void clear() {
        retro_api_version = nullptr;
        retro_cheat_reset = nullptr;
        retro_cheat_set = nullptr;
        retro_deinit = nullptr;
        retro_init = nullptr;
        retro_get_memory_data = nullptr;
        retro_get_memory_size = nullptr;
        retro_get_region = nullptr;
        retro_get_system_av_info = nullptr;
        retro_get_system_info = nullptr;
        retro_load_game = nullptr;
        retro_load_game_special = nullptr;
        retro_reset = nullptr;
        retro_run = nullptr;
        retro_serialize = nullptr;
        retro_serialize_size = nullptr;
        retro_unload_game = nullptr;
        retro_unserialize = nullptr;

        retro_set_audio_sample = nullptr;
        retro_set_audio_sample_batch = nullptr;
        retro_set_controller_port_device = nullptr;
        retro_set_environment = nullptr;
        retro_set_input_poll = nullptr;
        retro_set_input_state = nullptr;
        retro_set_video_refresh = nullptr;

        retro_audio = nullptr;
        retro_audio_set_state = nullptr;
        retro_frame_time = nullptr;
        retro_keyboard_event = nullptr;
    }

};

class Core: public QObject {
    Q_OBJECT

    Q_PROPERTY( RenderType renderType READ renderType NOTIFY renderTypeChanged )
    Q_PROPERTY( QString saveDirectory READ saveDirectory WRITE setSaveDirectory NOTIFY saveDirectoryChanged)
    Q_PROPERTY( QString systemDirectory READ systemDirectory WRITE setSystemDirectory NOTIFY systemDirectoryChanged )
    Q_PROPERTY( bool needsGame READ needsGame )

    bool qmlNeedsGame;
    QString qmlSaveDirectory;
    QString qmlSystemDirectory;

    void setNeedsGame( const bool need )
    {
        qmlNeedsGame = need;
        emit needsGameChanged();
    }

    public:

        Core();
        ~Core();

        bool needsGame() const
        {
            return qmlNeedsGame;
        }

        QString saveDirectory() const
        {
            return qmlSaveDirectory;
        }

        QString systemDirectory() const
        {
            return qmlSystemDirectory;
        }

        void setSaveDirectory( const QString saveDirectory )
        {
            qmlSaveDirectory = saveDirectory;
            emit saveDirectoryChanged();
        }

        void setSystemDirectory( const QString systemDirectory )
        {
            qmlSystemDirectory = systemDirectory;
            emit systemDirectoryChanged();
        }

        enum RenderType {
            OpenGL = 0,
            Software,
        };
        Q_ENUMS( RenderType )

        typedef enum : int {
            STATEUNINITIALIZED,
            STATEREADY,
            STATEFINISHED,
            STATEERROR
        } State;

        typedef enum : int {

            // Everything's okay!
            CORENOERROR,

            // Unable to load core, file could not be loaded as a shared library?
            // Wrong architecture? Wrong OS? Not even a shared library? File corrupt?
            CORELOAD,

            // The core does not have the right extension for the platform Phoenix is running on
            CORENOTLIBRARY,

            // Unable to load core, file was not found
            CORENOTFOUND,

            // Unable to load core, Phoenix did not have permission to open file
            COREACCESSDENIED,

            // Some other filesystem error preventing core from being loaded
            // IO Error, volume was dismounted, network resource not available
            COREUNKNOWNERROR,

            // Unable to load game, file was not found
            GAMENOTFOUND,

            // Unable to load game, Phoenix did not have permission to open file
            GAMEACCESSDENIED,

            // Some other filesystem error preventing game from being loaded
            // IO Error, volume was dismounted, network resource not available
            GAMEUNKNOWNERROR

        } Error;

        RenderType renderType() const
        {
            return qmlRenderType;
        }

        void setRenderType( RenderType type )
        {
            qmlRenderType = type;
            emit renderTypeChanged();
        }

        // State to text helper
        static QString stateToText( State state );

        // Do not delete these pointers, the core does not own them.
        QOpenGLContext *currentOpenGLContext;
        QOpenGLFramebufferObject *currentFrameBuffer;

        retro_hw_render_callback *retroHwCallback()
        {
            return &libretroOpenGLCallback;
        }

        QOpenGLFramebufferObject *getCurrentFramebuffer()
        {
            return currentFrameBuffer;
        }


        static uintptr_t currentFrameBufferID()
        {
            //auto id = (uintptr_t)core->currentFrameBuffer->handle();
            return 1;
        }

        static retro_proc_address_t procAddress( const char *sym )
        {
            return core->currentOpenGLContext->getProcAddress( sym );
        }


        QMap<QString, CoreVariable> getVariables() const
        {
            return variables;
        }

        bool updateCoreVariables;


    signals:
        void needsGameChanged();
        void saveDirectoryChanged();
        void systemDirectoryChanged();
        void renderTypeChanged();
        void signalCoreStateChanged( Core::State newState, Core::Error error );
        void signalAVFormat( retro_system_av_info avInfo, retro_pixel_format pixelFormat );
        void signalAudioData( int16_t *data, int bytes );
        void signalVideoData( uchar *data, unsigned width, unsigned height, int pitch );
        void signalUpdateVariables( const QString key,
                                    const QString value,
                                    const QString description,
                                    const QStringList choices,
                                    const int index );
    public slots:

        // Load the libretro core at the given path
        void slotLoadCore( const QString path );

        // Load the game at the given path
        void slotLoadGame( const QString path );

        // Run core for one frame
        void slotFrame();

        // Write save games to disk and free memory
        void slotShutdown();

        // Update core variables.
        void emitUpdateVariables( const QString key
                                  , const QString value
                                  , const QString description
                                  , const QStringList choices
                                  , const int index )
        {
            emit signalUpdateVariables( key, value, description, choices, index );
        }

        void setVariable( const QString key, const QString value );

    protected:

        // Only staticly-linked callbacks may access this data/call these methods

        // A hack that gives us the implicit C++ 'this' pointer while maintaining a C-style function signature
        // for the callbacks as required by libretro.h. Thanks to this, at this time we can only
        // have a single instance of Core running at any time.
        static Core *core;

        // Struct containing libretro methods
        LibretroSymbols symbols;

        // Used by environment callback
        // Info about the OpenGL context provided by the Phoenix frontend
        // for the core's internal use
        retro_hw_render_callback libretroOpenGLCallback;

        // Used by environment callback
        QByteArray libraryFilename;

        // Used by environment callback
        void emitStateReady();

        // Used by audio callback
        void emitAudioData( int16_t *data, int bytes );

        // Used by video callback
        void emitVideoData( uchar *data, unsigned width, unsigned height, size_t pitch );



    private:

        RenderType qmlRenderType;
        // Wrapper around shared library file (.dll, .dylib, .so)
        QLibrary libretroCore;
        QString libraryPath;

        //
        // Core-specific constants
        //

        // Audio and video rates/dimensions/types
        retro_system_av_info *avInfo;
        retro_pixel_format pixelFormat;

        // Information about the core
        retro_system_info *systemInfo;
        bool fullPathNeeded;

        // Mapping between a retropad button id and a human-readble (and core-defined) label (a string)
        // For use with controller setting UIs
        // TODO: Make this an array, we'll be getting many of these mappings, each with different button ids/labels
        retro_input_descriptor retropadToStringMap;

        //
        // Game
        //

        // Path to ROM/ISO, empty if (!fullPathNeeded)
        QString gamePath;

        // Raw ROM/ISO data, empty if (fullPathNeeded)
        QByteArray gameData;

        //
        // Audio
        //

        // Buffer pool. Since each buffer holds one frame, depending on core, 30 frames = ~500ms
        int16_t *audioBufferPool[30];
        int audioPoolCurrentBuffer;

        // Amount audioBufferPool[ audioBufferPoolIndex ] has been filled
        // Each frame, exactly ( sampleRate * 4 ) bytes should be copied to
        // audioBufferPool[ audioBufferPoolIndex ][ audioBufferCurrentByte ], total
        int audioBufferCurrentByte;

        //
        // Video
        //

        // Buffer pool. Depending on core, 30 frames = ~500ms
        uchar *videoBufferPool[30];
        int videoBufferPoolIndex;

        //
        // Save states
        //

        // bool loadGameState( QString save_path, QString game_name );
        // bool saveGameState( QString save_path, QString game_name );

        //
        // SRAM
        //

        void *SRAMDataRaw;
        void loadSRAM();
        void saveSRAM();

        //
        // Callbacks
        //

        static void audioSampleCallback( int16_t left, int16_t right );
        static size_t audioSampleBatchCallback( const int16_t *data, size_t frames );
        static bool environmentCallback( unsigned cmd, void *data );
        static void inputPollCallback( void );
        static void logCallback( enum retro_log_level level, const char *fmt, ... );
        static int16_t inputStateCallback( unsigned port, unsigned device, unsigned index, unsigned id );
        static void videoRefreshCallback( const void *data, unsigned width, unsigned height, size_t pitch );

        //
        // Misc
        //

        // Core-specific variables

        QMap<QString, CoreVariable> variables;

};


#endif
