# CSI 2026 Synchronized Output Implementation

This directory contains the implementation of CSI 2026 (Synchronized Output) support for WindTerm.

## Overview

CSI 2026 is a terminal control sequence that enables **synchronized output**, allowing applications to batch multiple terminal updates and render them atomically. This prevents screen flickering and tearing during complex screen updates.

### Control Sequences

| Sequence | Description |
|----------|-------------|
| `ESC[?2026h` | Enable synchronized output mode |
| `ESC[?2026l` | Disable synchronized output mode |

## Components

### 1. TerminalMode
**File**: `TerminalMode.h/cpp`

Manages terminal mode states including:
- Synchronized output mode (2026)
- Show cursor (25)
- Alternate screen (1047, 1049)
- Bracketed paste (2004)
- And other standard modes

Key features:
- Thread-safe mode state management
- Save/restore mode states
- Batch notification for mode changes

### 2. SynchronizedBuffer
**File**: `SynchronizedBuffer.h/cpp`

Buffers terminal output data during synchronized mode:
- Stores text data, cursor movements, screen clears, attribute changes
- Automatically flushes when buffer is full or timeout
- Prevents buffer overflow with size limits

Key features:
- Automatic mode transition handling
- Timeout-based flushing (100ms default)
- Buffer size limits (1MB default)

### 3. CSIParser
**File**: `CSIParser.h/cpp`

Parses ANSI/CSI control sequences:
- State machine-based parser
- Supports standard CSI sequences
- Private sequence detection (CSI ?...)
- Command type identification

Key features:
- Handles incomplete sequences
- Batch command processing
- Debug output support

### 4. TerminalHandler
**File**: `TerminalHandler.h/cpp`

Main integration class that combines all components:
- Routes input data through CSI parser
- Manages mode state transitions
- Coordinates synchronized buffering
- Emits rendering signals

## Usage Example

```cpp
#include "Terminal/TerminalHandler.h"

// Create handler
TerminalHandler handler;

// Connect to your renderer
connect(&handler, &TerminalHandler::renderData,
        this, &MyRenderer::onRenderData);

connect(&handler, &TerminalHandler::synchronizedOutputChanged,
        this, &MyRenderer::onSyncModeChanged);

// Process terminal data
QByteArray data = readFromPty();
handler.processInput(data);

// Force flush when needed (e.g., on window resize)
handler.forceFlush();
```

## How Synchronized Output Works

1. **Normal Mode** (default):
   - Data passes through immediately
   - Rendering happens in real-time

2. **Synchronized Mode** (`ESC[?2026h`):
   - All output is buffered in `SynchronizedBuffer`
   - No rendering occurs
   - Cursor moves, screen clears, text - all cached

3. **Flush** (`ESC[?2026l` or timeout):
   - All buffered data emitted at once
   - Single atomic render update
   - Back to normal mode

## Thread Safety

All components use `SpinMutex` for thread-safe access:
- Mode state changes are atomic
- Buffer operations are protected
- Signal emissions happen outside locks

## Integration with WindTerm

To integrate with WindTerm's existing architecture:

1. **Pty Layer**: Connect `Pty::readyRead` to `TerminalHandler::processInput`

2. **Rendering Layer**: Connect `TerminalHandler` signals to your render queue

3. **Mode Queries**: Use `TerminalMode` to check current state before operations

## Testing

Test sequences for CSI 2026:

```bash
# Enable sync mode, print text, disable sync mode
printf '\033[?2026h'
printf 'Line 1\nLine 2\nLine 3\n'
printf '\033[?2026l'

# Complex update (e.g., from a TUI application)
printf '\033[?2026h\033[2J\033[HNew Screen Content\033[?2026l'
```

## References

- [Synchronized Output Spec](https://gist.github.com/chriskempson/eb8c5cf6508bc47836b0b95c3a3e9c81)
- [DECSET/DECRST Documentation](https://vt100.net/docs/vt510-rm/DECSET.html)

## License

Apache-2.0 (same as WindTerm)
