/*
 * Copyright 2020, WindTerm.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file example_csi2026.cpp
 * @brief Example demonstrating CSI 2026 Synchronized Output usage
 *
 * This example shows how to use the TerminalHandler to process
 * terminal data with CSI 2026 synchronized output support.
 */

#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

#include "Terminal/TerminalHandler.h"

/**
 * @brief Simple renderer that prints what it receives
 */
class SimpleRenderer : public QObject
{
    Q_OBJECT

public:
    SimpleRenderer(QObject *parent = nullptr) : QObject(parent) {}

public slots:
    void onRenderData(const QByteArray &data, SynchronizedBuffer::BufferItemType type)
    {
        QString typeStr;
        switch (type) {
        case SynchronizedBuffer::TEXT_DATA: typeStr = "TEXT"; break;
        case SynchronizedBuffer::CURSOR_MOVE: typeStr = "CURSOR"; break;
        case SynchronizedBuffer::CLEAR_SCREEN: typeStr = "CLEAR"; break;
        case SynchronizedBuffer::ATTRIBUTE_CHANGE: typeStr = "ATTR"; break;
        default: typeStr = "OTHER"; break;
        }

        qDebug() << "[RENDER]" << typeStr << "Data:" << data;
    }

    void onCursorPositionChanged(int row, int column)
    {
        qDebug() << "[CURSOR] Row:" << row << "Col:" << column;
    }

    void onSyncModeChanged(bool enabled)
    {
        qDebug() << "[SYNC MODE]" << (enabled ? "ENABLED" : "DISABLED");
    }

    void onClearScreen(int mode)
    {
        qDebug() << "[CLEAR] Mode:" << mode;
    }
};

/**
 * @brief Demonstrate CSI 2026 synchronized output
 */
void demoSynchronizedOutput(TerminalHandler &handler)
{
    qDebug() << "\n=== Demo 1: Normal Mode ===";
    handler.processInput("Hello World\n");

    qDebug() << "\n=== Demo 2: Synchronized Mode ===";

    // Enable sync mode: ESC[?2026h
    handler.processInput("\x1B[?2026h");

    // These will be buffered, not rendered
    handler.processInput("Line 1\n");
    handler.processInput("Line 2\n");
    handler.processInput("Line 3\n");

    qDebug() << "(Above 3 lines are buffered, not yet rendered)";

    // Disable sync mode: ESC[?2026l - this triggers flush
    handler.processInput("\x1B[?2026l");

    qDebug() << "(Now all buffered lines are rendered at once)";
}

/**
 * @brief Demonstrate complex screen update
 */
void demoComplexUpdate(TerminalHandler &handler)
{
    qDebug() << "\n=== Demo 3: Complex Screen Update ===";

    // Simulate a TUI application doing a full screen update:
    // 1. Enable sync mode
    // 2. Clear screen
    // 3. Move cursor to home position
    // 4. Draw content
    // 5. Disable sync mode (atomic render)

    QByteArray update;
    update.append("\x1B[?2026h");      // Enable sync
    update.append("\x1B[2J");           // Clear screen
    update.append("\x1B[H");            // Cursor home (1,1)
    update.append("=== Main Menu ===\n");
    update.append("1. Option One\n");
    update.append("2. Option Two\n");
    update.append("3. Option Three\n");
    update.append("\x1B[4;5H");          // Move cursor to row 4, col 5
    update.append("> ");                // Draw prompt
    update.append("\x1B[?2026l");       // Disable sync (flush)

    handler.processInput(update);
}

/**
 * @brief Demonstrate cursor movement with sync mode
 */
void demoCursorMovement(TerminalHandler &handler)
{
    qDebug() << "\n=== Demo 4: Cursor Movement ===";

    // Enable sync mode
    handler.processInput("\x1B[?2026h");

    // Move cursor to specific positions
    handler.processInput("\x1B[10;20H");  // Row 10, Col 20
    handler.processInput("Center");
    handler.processInput("\x1B[H");        // Home
    handler.processInput("Top-Left");

    // Flush
    handler.processInput("\x1B[?2026l");
}

/**
 * @brief Demonstrate attribute changes (colors, styles)
 */
void demoAttributes(TerminalHandler &handler)
{
    qDebug() << "\n=== Demo 5: Attribute Changes ===";

    // Enable sync mode
    handler.processInput("\x1B[?2026h");

    // Set bold, red foreground
    handler.processInput("\x1B[1;31m");
    handler.processInput("Bold Red Text");

    // Reset attributes
    handler.processInput("\x1B[0m");
    handler.processInput(" Normal Text");

    // Set background color (green)
    handler.processInput("\x1B[42m");
    handler.processInput(" Green BG ");
    handler.processInput("\x1B[0m");
    handler.processInput("\n");

    // Flush
    handler.processInput("\x1B[?2026l");
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    qDebug() << "CSI 2026 Synchronized Output Demo";
    qDebug() << "=================================";

    // Create handler and renderer
    TerminalHandler handler;
    SimpleRenderer renderer;

    // Enable debug output
    handler.setDebugEnabled(true);

    // Connect signals
    QObject::connect(&handler, &TerminalHandler::renderData,
                     &renderer, &SimpleRenderer::onRenderData);
    QObject::connect(&handler, &TerminalHandler::cursorPositionChanged,
                     &renderer, &SimpleRenderer::onCursorPositionChanged);
    QObject::connect(&handler, &TerminalHandler::synchronizedOutputChanged,
                     &renderer, &SimpleRenderer::onSyncModeChanged);
    QObject::connect(&handler, &TerminalHandler::clearScreenRequested,
                     &renderer, &SimpleRenderer::onClearScreen);

    // Run demos
    demoSynchronizedOutput(handler);
    demoComplexUpdate(handler);
    demoCursorMovement(handler);
    demoAttributes(handler);

    qDebug() << "\n=== All demos completed ===";

    // Exit after a short delay
    QTimer::singleShot(100, &app, &QCoreApplication::quit);

    return app.exec();
}

#include "example_csi2026.moc"
