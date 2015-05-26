# Phoenix-Backend

This is the backend emulator client that powers [Phoenix](https://github.com/team-phoenix/Phoenix). In order to use this in your Qt project,
just copy over the entire project and declare your qml type like:

```
import QtQuick 2.2
import libretro.video 1.0

ApplicationWindow {
    id: root;
    height: 800;
    width: 600;
  
    VideoItem {
        id: gameOutput;
        Component.onCompleted: {
            libretroCore = "/path/to/core.dylib";
            game = "/path/to/game/rom/";
        }
    }

}
```

Make sure you have registered the QML type via 'qmlRegisterType'.
