# DeaDBeeF Vinyl Record Plugin

A [DeaDBeeF](http://deadbeef.sourceforge.net) audio player plugin that displays album art as a realistic rotating vinyl record.

When music plays, the vinyl spins. When paused or stopped, it stays still. Album art is shown in the center of the record.

## Features

- Realistic vinyl record rendering with concentric grooves and light reflection
- Smooth rotation animation during playback
- Multi-strategy album cover retrieval (embedded ID3v2, artwork plugin, directory scan)
- Adjustable widget size and cover area via plugin settings
- Right-click context menu

## Screenshots

*(add screenshots here)*

## Requirements

- DeaDBeeF 1.10+
- GTK+ 3
- Cairo
- GdkPixbuf

## Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Windows

Outputs `vinyl_record.dll`. Requires GTK3 development libraries.

### Linux

Outputs `vinyl_record.so`.

```bash
# Install dependencies (Debian/Ubuntu)
sudo apt install libgtk-3-dev cmake gcc
```

## Install

Copy the built plugin (`vinyl_record.dll` or `vinyl_record.so`) to DeaDBeeF's plugin directory, then enable it in **Edit > Preferences > Plugins**. Add the "Vinyl Record" widget to your design mode layout.

## License

MIT License - Copyright (c) 2026 Jiangxing Zhao
