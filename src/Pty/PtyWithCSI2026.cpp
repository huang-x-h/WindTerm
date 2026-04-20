/*
 * PtyWithCSI2026.cpp
 *
 * Example implementation showing how to integrate CSI 2026 into Pty
 */

#include "PtyWithCSI2026.h"

// TODO: Include actual WindTerm render engine
// #include "RenderEngine.h"

#include <QDebug>

PtyWithCSI2026::PtyWithCSI2026()
    : m_columns(-1)
    , m_rows(-1)
{
    // ===== CSI 2026: CONNECT SIGNALS =====

    // Data ready for rendering (text, clears, etc.)
    connect(&m_terminalHandler, &TerminalHandler::renderData,
            this, &PtyWithCSI2026::onTerminalRenderData);

    // Cursor position changes
    connect(&m_terminalHandler, &TerminalHandler::cursorPositionChanged,
            this, &PtyWithCSI2026::onTerminalCursorPositionChanged);

    // Screen clear requests
    connect(&m_terminalHandler, &TerminalHandler::clearScreenRequested,
            this, &PtyWithCSI2026::onTerminalClearScreen);

    // Text attribute changes (colors, styles)
    connect(&m_terminalHandler, &TerminalHandler::attributeChanged,
            this, &PtyWithCSI2026::onTerminalAttributesChanged);

    // Debug: Log when sync mode changes
    connect(&m_terminalHandler, &TerminalHandler::synchronizedOutputChanged,
            this, [](bool enabled) {
        qDebug() << "[CSI 2026] Synchronized output:" << (enabled ? "ON" : "OFF");
    });
}

void PtyWithCSI2026::processTerminalData(const QByteArray &rawData)
{
    // Process data through TerminalHandler
    // This will:
    // 1. Parse CSI sequences
    // 2. Handle CSI 2026 (synchronized output)
    // 3. Emit signals for rendering
    m_terminalHandler.processInput(rawData);
}

void PtyWithCSI2026::onTerminalRenderData(const QByteArray &data,
                                          SynchronizedBuffer::BufferItemType type)
{
    // Forward to rendering engine
    // TODO: Replace with actual WindTerm render engine calls

    switch (type) {
    case SynchronizedBuffer::TEXT_DATA:
        // renderEngine->addText(data);
        emit renderDataReady(data, 0);  // Type 0 = text
        break;

    case SynchronizedBuffer::CURSOR_MOVE:
        // Cursor move handled separately via cursorPositionChanged signal
        break;

    case SynchronizedBuffer::CLEAR_SCREEN:
        // Clear screen handled separately via clearScreenRequested signal
        break;

    case SynchronizedBuffer::SCROLL_REGION:
        // renderEngine->setScrollRegion(top, bottom);
        emit renderDataReady(data, 3);  // Type 3 = scroll region
        break;

    case SynchronizedBuffer::ATTRIBUTE_CHANGE:
        // Attributes handled separately via attributeChanged signal
        break;

    case SynchronizedBuffer::RESIZE:
        // renderEngine->resize(rows, cols);
        emit renderDataReady(data, 5);  // Type 5 = resize
        break;
    }
}

void PtyWithCSI2026::onTerminalCursorPositionChanged(int row, int column)
{
    // Update cursor position in render engine
    // TODO: Replace with actual render engine call
    // renderEngine->setCursorPosition(row, column);

    emit cursorPositionChanged(row, column);
}

void PtyWithCSI2026::onTerminalClearScreen(int mode)
{
    // Clear screen in render engine
    // TODO: Replace with actual render engine calls

    switch (mode) {
    case 0:
        // renderEngine->clearFromCursorToEnd();
        break;
    case 1:
        // renderEngine->clearFromStartToCursor();
        break;
    case 2:
        // renderEngine->clearScreen();
        break;
    case 3:
        // renderEngine->clearScreenAndScrollback();
        break;
    }

    emit clearScreenRequested(mode);
}

void PtyWithCSI2026::onTerminalAttributesChanged(const QList<int> &params)
{
    // Apply text attributes to render engine
    // TODO: Replace with actual render engine calls

    for (int param : params) {
        switch (param) {
        case 0:
            // renderEngine->resetAttributes();
            break;
        case 1:
            // renderEngine->setBold(true);
            break;
        case 22:
            // renderEngine->setBold(false);
            break;
        case 3:
            // renderEngine->setItalic(true);
            break;
        case 23:
            // renderEngine->setItalic(false);
            break;
        case 4:
            // renderEngine->setUnderline(true);
            break;
        case 24:
            // renderEngine->setUnderline(false);
            break;
        case 7:
            // renderEngine->setReverse(true);
            break;
        case 27:
            // renderEngine->setReverse(false);
            break;
        default:
            if (param >= 30 && param <= 37) {
                // Standard foreground: param - 30
                // renderEngine->setForegroundColor(param - 30);
            } else if (param >= 40 && param <= 47) {
                // Standard background: param - 40
                // renderEngine->setBackgroundColor(param - 40);
            } else if (param >= 90 && param <= 97) {
                // Bright foreground: param - 90 + 8
                // renderEngine->setForegroundColor(param - 90 + 8);
            } else if (param >= 100 && param <= 107) {
                // Bright background: param - 100 + 8
                // renderEngine->setBackgroundColor(param - 100 + 8);
            } else if (param == 38 || param == 48) {
                // 256 color or true color (handled separately)
                // This would need special parsing in CSIParser
            }
            break;
        }
    }

    emit attributesChanged(params);
}

/*
 * EXAMPLE: Modified readAll() implementation
 *
 * This shows how existing Pty subclasses should modify their readAll()
 * method to support CSI 2026.
 */

/*
QByteArray PtyConcrete::readAll()
{
    // 1. Read raw data from underlying source
    QByteArray rawData = readFromPipe();  // or similar

    // 2. Process through TerminalHandler
    // This parses CSI sequences and handles buffering
    processTerminalData(rawData);

    // 3. Return empty (rendering now happens via signals)
    // OR return any unprocessed data
    return QByteArray();
}
*/

/*
 * EXAMPLE: Modified resizeWindow() implementation
 */

/*
bool PtyConcrete::resizeWindow(qint16 rows, qint16 columns)
{
    // 1. Update internal size
    m_rows = rows;
    m_columns = columns;

    // 2. Force flush of any buffered data
    // This ensures consistent state during resize
    forceFlush();

    // 3. Perform actual resize
    // ... platform-specific code ...

    return true;
}
*/

/*
 * EXAMPLE: Modified write() for bracketed paste
 *
 * TerminalHandler can also track bracketed paste mode (2004)
 * for proper paste handling.
 */

/*
qint64 PtyConcrete::write(const QByteArray &text)
{
    // Check if bracketed paste mode is enabled
    if (m_terminalHandler.modeManager()->isEnabled(
            TerminalMode::BRACKETED_PASTE)) {
        // Wrap paste data with escape sequences
        QByteArray wrapped;
        wrapped.append("\x1B[200~");  // Paste start
        wrapped.append(text);
        wrapped.append("\x1B[201~");  // Paste end
        return writeToPipe(wrapped);
    }

    return writeToPipe(text);
}
*/

/*
 * EXAMPLE: Debug output
 *
 * Enable debug mode to see CSI sequence parsing:
 */

/*
void PtyConcrete::enableDebug()
{
    m_terminalHandler.setDebugEnabled(true);
}
*/
