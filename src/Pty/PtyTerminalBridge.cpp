/*
 * PtyTerminalBridge.cpp
 */

#include "PtyTerminalBridge.h"

// TODO: Include actual WindTerm header files
// #include "RenderEngine.h"
// #include "Pty.h"

#include <QDebug>

PtyTerminalBridge::PtyTerminalBridge(Pty *pty, RenderEngine *engine, QObject *parent)
    : QObject(parent)
    , m_pty(pty)
    , m_engine(engine)
    , m_handler(new TerminalHandler(this))
{
}

PtyTerminalBridge::~PtyTerminalBridge() = default;

void PtyTerminalBridge::initialize()
{
    // Connect TerminalHandler signals to our slots
    connect(m_handler, &TerminalHandler::renderData,
            this, &PtyTerminalBridge::onRenderData);

    connect(m_handler, &TerminalHandler::cursorPositionChanged,
            this, &PtyTerminalBridge::onCursorPositionChanged);

    connect(m_handler, &TerminalHandler::clearScreenRequested,
            this, &PtyTerminalBridge::onClearScreenRequested);

    connect(m_handler, &TerminalHandler::attributeChanged,
            this, &PtyTerminalBridge::onAttributeChanged);

    connect(m_handler, &TerminalHandler::synchronizedOutputChanged,
            this, &PtyTerminalBridge::onSynchronizedOutputChanged);

    // Connect Pty data ready signal
    // TODO: Adapt to actual Pty signal name
    // connect(m_pty, &Pty::readyRead, this, &PtyTerminalBridge::onPtyDataReady);
}

bool PtyTerminalBridge::isSynchronizedOutputActive() const
{
    return m_handler->isSynchronizedOutputEnabled();
}

void PtyTerminalBridge::forceFlush()
{
    m_handler->forceFlush();
}

void PtyTerminalBridge::onPtyDataReady()
{
    if (!m_pty) return;

    // Read all available data from Pty
    // TODO: Adapt to actual Pty API
    // QByteArray data = m_pty->readAll();

    // For now, assume we get data somehow
    QByteArray data; // = m_pty->readAll();

    if (data.isEmpty()) return;

    m_stats.totalBytesProcessed += data.size();

    // Process through TerminalHandler
    // This will parse CSI sequences and handle synchronized output
    m_handler->processInput(data);
}

void PtyTerminalBridge::onWindowResized(int rows, int columns)
{
    // Force flush when window is resized to ensure consistent state
    forceFlush();

    // TODO: Send resize signal to terminal if needed
    // QByteArray resizeSeq = QString("\x1B[8;%1;%2t").arg(rows).arg(columns).toLatin1();
}

void PtyTerminalBridge::onRenderData(const QByteArray &data,
                                      SynchronizedBuffer::BufferItemType type)
{
    if (!m_engine) return;

    updateStats(type);

    switch (type) {
    case SynchronizedBuffer::TEXT_DATA:
        // TODO: m_engine->drawText(data);
        qDebug() << "[Render] Text:" << data.left(50);
        break;

    case SynchronizedBuffer::CURSOR_MOVE:
        // Cursor move is handled separately via cursorPositionChanged signal
        break;

    case SynchronizedBuffer::CLEAR_SCREEN:
        // Clear screen is handled separately via clearScreenRequested signal
        break;

    case SynchronizedBuffer::SCROLL_REGION:
        // TODO: m_engine->setScrollRegion(param1, param2);
        break;

    case SynchronizedBuffer::ATTRIBUTE_CHANGE:
        // Attributes are handled separately via attributeChanged signal
        break;

    case SynchronizedBuffer::RESIZE:
        // TODO: m_engine->resize(param1, param2);
        break;

    default:
        qDebug() << "[Render] Unknown type:" << type;
        break;
    }
}

void PtyTerminalBridge::onCursorPositionChanged(int row, int column)
{
    if (!m_engine) return;

    // TODO: m_engine->setCursorPosition(row, column);
    qDebug() << "[Cursor] Move to row:" << row << "col:" << column;
}

void PtyTerminalBridge::onClearScreenRequested(int mode)
{
    if (!m_engine) return;

    // TODO: Adapt to actual render engine API
    switch (mode) {
    case 0:  // Clear from cursor to end
        // m_engine->clearToEnd();
        qDebug() << "[Clear] To end";
        break;
    case 1:  // Clear from start to cursor
        // m_engine->clearToStart();
        qDebug() << "[Clear] To start";
        break;
    case 2:  // Clear entire screen
        // m_engine->clear();
        qDebug() << "[Clear] Entire screen";
        break;
    case 3:  // Clear entire screen and scrollback
        // m_engine->clearWithScrollback();
        qDebug() << "[Clear] With scrollback";
        break;
    default:
        qDebug() << "[Clear] Unknown mode:" << mode;
        break;
    }
}

void PtyTerminalBridge::onAttributeChanged(const QList<int> &sgrParams)
{
    if (!m_engine) return;

    // TODO: Adapt to actual render engine API
    for (int param : sgrParams) {
        switch (param) {
        case 0:
            // m_engine->resetAttributes();
            qDebug() << "[SGR] Reset";
            break;
        case 1:
            // m_engine->setBold(true);
            qDebug() << "[SGR] Bold on";
            break;
        case 22:
            // m_engine->setBold(false);
            qDebug() << "[SGR] Bold off";
            break;
        case 3:
            // m_engine->setItalic(true);
            qDebug() << "[SGR] Italic on";
            break;
        case 23:
            // m_engine->setItalic(false);
            qDebug() << "[SGR] Italic off";
            break;
        case 4:
            // m_engine->setUnderline(true);
            qDebug() << "[SGR] Underline on";
            break;
        case 24:
            // m_engine->setUnderline(false);
            qDebug() << "[SGR] Underline off";
            break;
        case 7:
            // m_engine->setReverse(true);
            qDebug() << "[SGR] Reverse on";
            break;
        case 27:
            // m_engine->setReverse(false);
            qDebug() << "[SGR] Reverse off";
            break;
        default:
            if (param >= 30 && param <= 37) {
                // Standard foreground colors
                // m_engine->setForegroundColor(param - 30);
                qDebug() << "[SGR] FG color:" << (param - 30);
            } else if (param >= 40 && param <= 47) {
                // Standard background colors
                // m_engine->setBackgroundColor(param - 40);
                qDebug() << "[SGR] BG color:" << (param - 40);
            } else if (param >= 90 && param <= 97) {
                // Bright foreground colors
                // m_engine->setForegroundColor(param - 90 + 8);
                qDebug() << "[SGR] Bright FG color:" << (param - 90 + 8);
            } else if (param >= 100 && param <= 107) {
                // Bright background colors
                // m_engine->setBackgroundColor(param - 100 + 8);
                qDebug() << "[SGR] Bright BG color:" << (param - 100 + 8);
            }
            break;
        }
    }
}

void PtyTerminalBridge::onSynchronizedOutputChanged(bool enabled)
{
    qDebug() << "[Sync Output]" << (enabled ? "ENABLED" : "DISABLED");

    if (!enabled) {
        // Flush just completed - log stats
        m_stats.syncFramesRendered++;
    }
}

void PtyTerminalBridge::updateStats(SynchronizedBuffer::BufferItemType type)
{
    if (type == SynchronizedBuffer::TEXT_DATA) {
        if (isSynchronizedOutputActive()) {
            // Data is part of a synchronized frame
        } else {
            m_stats.normalFramesRendered++;
        }
    }
}

void PtyTerminalBridge::connectToRenderEngine()
{
    // Additional connections to render engine if needed
}
