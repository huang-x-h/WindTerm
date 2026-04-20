# CSI 2026 Integration Guide for WindTerm

This guide explains how to integrate the CSI 2026 Synchronized Output support into WindTerm's existing codebase.

## Architecture Overview

```
Pty (src/Pty/)
    │
    ├── reads data from SSH/Telnet/Serial connection
    │
    ▼
TerminalHandler (src/Terminal/) [NEW]
    │
    ├── CSIParser: parses escape sequences
    ├── TerminalMode: manages mode states
    └── SynchronizedBuffer: buffers during sync mode
    │
    ▼
Rendering Engine (existing, proprietary)
```

## Integration Steps

### Step 1: Add Terminal Module to Build

Update WindTerm's main `CMakeLists.txt`:

```cmake
# Add Terminal module
add_subdirectory(src/Terminal)

# Link to main application
target_link_libraries(WindTerm PRIVATE
    # ... existing libraries ...
    Terminal
)
```

### Step 2: Integrate with Pty Classes

Modify `src/Pty/Pty.h` and implementations:

```cpp
// Pty.h - Add TerminalHandler as member

#include "Terminal/TerminalHandler.h"

class Pty : public QObject
{
    // ... existing code ...

protected:
    TerminalHandler m_terminalHandler;  // Add this

private slots:
    void onTerminalDataReady(const QByteArray &data,
                             SynchronizedBuffer::BufferItemType type);
    void onTerminalRenderData(const QByteArray &data,
                              SynchronizedBuffer::BufferItemType type);
};
```

Modify `Pty::readAll()` or data receiving path:

```cpp
// In Pty.cpp

QByteArray Pty::readAll()
{
    QByteArray rawData = // ... read from connection ...

    // Pass through TerminalHandler for CSI parsing
    m_terminalHandler.processInput(rawData);

    // Note: actual rendering is now done via signals
    return rawData;  // May return empty if fully buffered
}

// Or use signal-based approach:
void Pty::setupConnections()
{
    connect(&m_terminalHandler, &TerminalHandler::renderData,
            this, &Pty::onTerminalRenderData);

    connect(&m_terminalHandler, &TerminalHandler::synchronizedOutputChanged,
            this, &Pty::onSyncModeChanged);
}

void Pty::onTerminalRenderData(const QByteArray &data,
                                SynchronizedBuffer::BufferItemType type)
{
    // Send to rendering engine
    switch (type) {
    case SynchronizedBuffer::TEXT_DATA:
        renderEngine->addText(data);
        break;
    case SynchronizedBuffer::CURSOR_MOVE:
        renderEngine->moveCursor(...);
        break;
    case SynchronizedBuffer::CLEAR_SCREEN:
        renderEngine->clearScreen();
        break;
    case SynchronizedBuffer::ATTRIBUTE_CHANGE:
        renderEngine->setAttributes(...);
        break;
    }
}
```

### Step 3: Connect to Rendering Engine

Create an adapter class to bridge TerminalHandler signals with WindTerm's rendering:

```cpp
// TerminalRenderBridge.h

class TerminalRenderBridge : public QObject
{
    Q_OBJECT

public:
    TerminalRenderBridge(TerminalHandler *handler,
                         RenderEngine *engine,
                         QObject *parent = nullptr);

private slots:
    void onRenderData(const QByteArray &data,
                      SynchronizedBuffer::BufferItemType type);
    void onCursorPositionChanged(int row, int column);
    void onClearScreenRequested(int mode);
    void onAttributeChanged(const QList<int> &sgrParams);

private:
    RenderEngine *m_engine;
};

// TerminalRenderBridge.cpp

TerminalRenderBridge::TerminalRenderBridge(
    TerminalHandler *handler,
    RenderEngine *engine,
    QObject *parent)
    : QObject(parent)
    , m_engine(engine)
{
    connect(handler, &TerminalHandler::renderData,
            this, &TerminalRenderBridge::onRenderData);
    connect(handler, &TerminalHandler::cursorPositionChanged,
            this, &TerminalRenderBridge::onCursorPositionChanged);
    connect(handler, &TerminalHandler::clearScreenRequested,
            this, &TerminalRenderBridge::onClearScreenRequested);
    connect(handler, &TerminalHandler::attributeChanged,
            this, &TerminalRenderBridge::onAttributeChanged);
}

void TerminalRenderBridge::onRenderData(const QByteArray &data,
                                        SynchronizedBuffer::BufferItemType type)
{
    switch (type) {
    case SynchronizedBuffer::TEXT_DATA:
        m_engine->drawText(data);
        break;
    case SynchronizedBuffer::CURSOR_MOVE:
        // Cursor move is handled separately
        break;
    case SynchronizedBuffer::CLEAR_SCREEN:
        m_engine->clear();
        break;
    case SynchronizedBuffer::ATTRIBUTE_CHANGE:
        // Parse SGR attributes from data
        break;
    case SynchronizedBuffer::SCROLL_REGION:
        m_engine->setScrollRegion(...);
        break;
    case SynchronizedBuffer::RESIZE:
        m_engine->resize(...);
        break;
    }
}

void TerminalRenderBridge::onCursorPositionChanged(int row, int column)
{
    m_engine->setCursorPosition(row, column);
}

void TerminalRenderBridge::onClearScreenRequested(int mode)
{
    // mode: 0=to end, 1=to start, 2=entire screen, 3=entire + scrollback
    switch (mode) {
    case 0:
        m_engine->clearToEnd();
        break;
    case 1:
        m_engine->clearToStart();
        break;
    case 2:
        m_engine->clear();
        break;
    case 3:
        m_engine->clearWithScrollback();
        break;
    }
}

void TerminalRenderBridge::onAttributeChanged(const QList<int> &sgrParams)
{
    for (int param : sgrParams) {
        switch (param) {
        case 0:  m_engine->resetAttributes(); break;
        case 1:  m_engine->setBold(true); break;
        case 2:  m_engine->setDim(true); break;
        case 3:  m_engine->setItalic(true); break;
        case 4:  m_engine->setUnderline(true); break;
        case 5:  m_engine->setBlink(true); break;
        case 7:  m_engine->setReverse(true); break;
        case 8:  m_engine->setHidden(true); break;
        case 30: case 31: case 32: case 33:
        case 34: case 35: case 36: case 37:
            m_engine->setForegroundColor(param - 30);
            break;
        case 40: case 41: case 42: case 43:
        case 44: case 45: case 46: case 47:
            m_engine->setBackgroundColor(param - 40);
            break;
        // ... 256 colors, true color ...
        }
    }
}
```

### Step 4: Handle Mode Queries

Some applications query terminal capabilities using escape sequences. Add support for responding to these:

```cpp
// Add to TerminalHandler

void TerminalHandler::processDeviceStatusRequest(const Command &cmd)
{
    if (cmd.type != DEVICE_STATUS) return;

    int mode = cmd.params.isEmpty() ? 0 : cmd.params.first();

    switch (mode) {
    case 5:  // Request device status
        // Reply: CSI 0 n (OK)
        emit responseData("\x1B[0n");
        break;
    case 6:  // Request cursor position
        // Reply: CSI row ; col R
        emit responseData(QString("\x1B[%1;%2R")
                         .arg(currentRow + 1)
                         .arg(currentCol + 1)
                         .toLatin1());
        break;
    }
}

void TerminalHandler::processModeQuery(const Command &cmd)
{
    if (cmd.type != CSIParser::DEVICE_STATUS) return;

    int mode = cmd.params.isEmpty() ? 0 : cmd.params.first();

    // Check if mode is enabled
    bool enabled = m_modeManager->isEnabled(
        static_cast<TerminalMode::Mode>(mode));

    // Reply: CSI ? mode ; status $
    // status: 1=set, 2=reset, 3=permanently set, 4=permanently reset
    int status = enabled ? 1 : 2;
    emit responseData(QString("\x1B[?%1;%2$").arg(mode).arg(status).toLatin1());
}
```

### Step 5: Add UI Feedback (Optional)

Show sync mode status in UI:

```cpp
// In your main window or status bar

void MainWindow::onSyncModeChanged(bool enabled)
{
    ui->statusBar->showMessage(
        enabled ? tr("Synchronized Output: ON") : tr("Synchronized Output: OFF"),
        2000);
}
```

## Testing

### Manual Test Commands

```bash
# Test 1: Basic synchronized output
printf '\033[?2026hSynchronized\nContent\033[?2026l\n'

# Test 2: Complex TUI simulation
printf '\033[?2026h\033[2J\033[HHeader\nLine 1\nLine 2\n\033[?2026l'

# Test 3: Nested (should be handled gracefully)
printf '\033[?2026h\033[?2026hContent\033[?2026l\033[?2026l'

# Test 4: Attributes in sync mode
printf '\033[?2026h\033[1mBold\033[0m Normal\033[?2026l\n'

# Test 5: Cursor movements
printf '\033[?2026h\033[10;20HPosition\033[5;10HMore\033[?2026l\n'
```

### Real Application Testing

Applications that benefit from CSI 2026:

| Application | Test Command |
|-------------|--------------|
| Vim/Neovim | Open large files, scroll rapidly |
| Emacs | `M-x term`, run commands |
| tmux | Split panes, resize windows |
| htop | Sort columns, scroll |
| ncdu | Navigate directories |
| ranger | Browse files |
| fzf | `ls | fzf` |

### Automated Testing

Build and run the unit tests:

```bash
cd src/Terminal
mkdir build && cd build
cmake ..
make
ctest -V
```

## Performance Considerations

### Memory Usage

- **Default buffer size**: 1MB per terminal
- **Maximum items**: Limited by buffer size, not count
- **Timeout**: 100ms flush to prevent stuck output

### Optimization Tips

1. **Batch Rendering**: Process all buffered items in one paint event
2. **Double Buffering**: Use QPainter with QWidget::setAutoFillBackground(false)
3. **Dirty Region Tracking**: Only redraw changed cells
4. **Font Caching**: Pre-render common glyphs

### Profiling

```cpp
// Add performance counters

class PerformanceMonitor {
    QElapsedTimer timer;
    int frameCount = 0;
    int syncFrames = 0;

public:
    void onFrameRendered() {
        frameCount++;
    }

    void onSyncFrameRendered(int itemCount) {
        syncFrames++;
        qDebug() << "Sync frame:" << itemCount << "items";
    }

    void report() {
        qDebug() << "Total frames:" << frameCount
                 << "Sync frames:" << syncFrames
                 << "Percentage:" << (100.0 * syncFrames / frameCount);
    }
};
```

## Debugging

### Enable Debug Output

```cpp
// In your initialization
terminalHandler.setDebugEnabled(true);

// Or set via environment variable
if (qEnvironmentVariableIsSet("WINDTERM_DEBUG_CSI")) {
    terminalHandler.setDebugEnabled(true);
}
```

### Common Issues

| Issue | Cause | Solution |
|-------|-------|----------|
| No output in sync mode | Forgot to handle flush signal | Connect `synchronizedOutputChanged` and call `forceFlush()` |
| Partial sequences | Network fragmentation | Buffer incomplete sequences in Pty layer |
| Missing cursor | Cursor hidden by app | Handle DECTCEM (DECSET 25) |
| Wrong colors | SGR parsing incomplete | Extend `handleSGR()` for 256/true color |

### Debug Dump

Add to `TerminalHandler` for troubleshooting:

```cpp
void TerminalHandler::dumpState() {
    qDebug() << "=== Terminal State ===";
    qDebug() << "Sync mode:" << isSynchronizedOutputEnabled();
    qDebug() << "Buffer size:" << m_syncBuffer->bufferSize();
    qDebug() << "Buffer items:" << m_syncBuffer->itemCount();
    qDebug() << "Pending text:" << m_pendingText.size();
    qDebug() << "Parser idle:" << m_csiParser->isIdle();
    qDebug() << "=====================";
}
```

## Compatibility

### Terminal Emulators Supporting CSI 2026

| Terminal | Support | Notes |
|----------|---------|-------|
| iTerm2 | ✅ Full | macOS |
| Windows Terminal | ✅ Full | Windows 10+ |
| kitty | ✅ Full | Linux/macOS |
| Alacritty | ✅ Full | Cross-platform |
| WezTerm | ✅ Full | Cross-platform |
| Konsole | ⚠️ Partial | KDE |
| gnome-terminal | ⚠️ Partial | VTE-based |
| xterm | ❌ No | Traditional |

### Application Support

| Application | Uses CSI 2026 | When |
|-------------|---------------|------|
| Neovim | ✅ | When `termguicolors` enabled |
| Emacs (vterm) | ✅ | When `vterm` module loaded |
| tmux | ✅ | Version 3.0+ |
| zsh (syntax highlighting) | ✅ | With fast-syntax-highlighting |
| fish | ⚠️ | Under development |
| bash | ❌ | No plans |

## References

1. [Synchronized Output Specification](https://gist.github.com/chriskempson/eb8c5cf6508bc47836b0b95c3a3e9c81)
2. [ECMA-48 Control Functions](https://www.ecma-international.org/publications-and-standards/standards/ecma-48/)
3. [DEC Private Set Mode](https://vt100.net/docs/vt510-rm/DECSET.html)
4. [Windows Terminal Issue #137](https://github.com/microsoft/terminal/issues/137)

## Migration Checklist

- [ ] Add Terminal module to build
- [ ] Integrate with Pty layer
- [ ] Connect to rendering engine
- [ ] Add mode query responses
- [ ] Test with manual commands
- [ ] Test with real applications
- [ ] Profile performance
- [ ] Add UI feedback (optional)
- [ ] Document for users

## License

All new code follows WindTerm's Apache-2.0 license.
