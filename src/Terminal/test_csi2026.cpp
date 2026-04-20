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
 * @file test_csi2026.cpp
 * @brief Unit tests for CSI 2026 Synchronized Output
 */

#include <QtTest>
#include <QSignalSpy>

#include "Terminal/TerminalMode.h"
#include "Terminal/SynchronizedBuffer.h"
#include "Terminal/CSIParser.h"
#include "Terminal/TerminalHandler.h"

class TestCSI2026 : public QObject
{
    Q_OBJECT

private slots:
    // TerminalMode tests
    void testModeSetAndReset();
    void testSynchronizedOutputMode();
    void testModeSaveAndRestore();
    void testToggleMode();

    // SynchronizedBuffer tests
    void testSyncBufferBasic();
    void testSyncBufferFlush();
    void testSyncBufferClear();
    void testSyncBufferOverflow();

    // CSIParser tests
    void testParseCSI2026Enable();
    void testParseCSI2026Disable();
    void testParseCursorMove();
    void testParseSGR();
    void testParseMultipleCommands();

    // TerminalHandler integration tests
    void testHandlerSyncEnable();
    void testHandlerSyncDisable();
    void testHandlerNormalMode();
    void testHandlerComplexSequence();
};

// ========== TerminalMode Tests ==========

void TestCSI2026::testModeSetAndReset()
{
    TerminalMode mode;

    QVERIFY(!mode.isEnabled(TerminalMode::SHOW_CURSOR));

    QVERIFY(mode.setMode(TerminalMode::SHOW_CURSOR));
    QVERIFY(mode.isEnabled(TerminalMode::SHOW_CURSOR));

    // Setting same mode again should return false
    QVERIFY(!mode.setMode(TerminalMode::SHOW_CURSOR));

    QVERIFY(mode.resetMode(TerminalMode::SHOW_CURSOR));
    QVERIFY(!mode.isEnabled(TerminalMode::SHOW_CURSOR));

    // Resetting same mode again should return false
    QVERIFY(!mode.resetMode(TerminalMode::SHOW_CURSOR));
}

void TestCSI2026::testSynchronizedOutputMode()
{
    TerminalMode mode;

    QSignalSpy spy(&mode, &TerminalMode::synchronizedOutputChanged);

    QVERIFY(mode.setMode(TerminalMode::SYNCHRONIZED_OUTPUT));
    QVERIFY(mode.isSynchronizedOutputEnabled());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toBool(), true);

    QVERIFY(mode.resetMode(TerminalMode::SYNCHRONIZED_OUTPUT));
    QVERIFY(!mode.isSynchronizedOutputEnabled());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toBool(), false);
}

void TestCSI2026::testModeSaveAndRestore()
{
    TerminalMode mode;

    mode.setMode(TerminalMode::SHOW_CURSOR);
    mode.setMode(TerminalMode::AUTO_WRAP);
    mode.saveModes();

    mode.resetMode(TerminalMode::SHOW_CURSOR);
    QVERIFY(!mode.isEnabled(TerminalMode::SHOW_CURSOR));

    mode.restoreModes();
    QVERIFY(mode.isEnabled(TerminalMode::SHOW_CURSOR));
    QVERIFY(mode.isEnabled(TerminalMode::AUTO_WRAP));
}

void TestCSI2026::testToggleMode()
{
    TerminalMode mode;

    QVERIFY(!mode.isEnabled(TerminalMode::SHOW_CURSOR));

    QVERIFY(mode.toggleMode(TerminalMode::SHOW_CURSOR));
    QVERIFY(mode.isEnabled(TerminalMode::SHOW_CURSOR));

    QVERIFY(mode.toggleMode(TerminalMode::SHOW_CURSOR));
    QVERIFY(!mode.isEnabled(TerminalMode::SHOW_CURSOR));
}

// ========== SynchronizedBuffer Tests ==========

void TestCSI2026::testSyncBufferBasic()
{
    SynchronizedBuffer buffer;

    QVERIFY(!buffer.isSynchronized());

    QSignalSpy modeSpy(&buffer, &SynchronizedBuffer::synchronizedModeChanged);

    QVERIFY(buffer.setSynchronizedMode(true));
    QVERIFY(buffer.isSynchronized());
    QCOMPARE(modeSpy.count(), 1);

    // Already enabled
    QVERIFY(!buffer.setSynchronizedMode(true));
}

void TestCSI2026::testSyncBufferFlush()
{
    SynchronizedBuffer buffer;

    QSignalSpy dataSpy(&buffer, &SynchronizedBuffer::dataReady);

    // Normal mode: data should pass through immediately
    buffer.writeData("Hello");
    QCOMPARE(dataSpy.count(), 1);

    // Enable sync mode
    buffer.setSynchronizedMode(true);
    dataSpy.clear();

    // Data should be buffered
    buffer.writeData("World");
    QCOMPARE(dataSpy.count(), 0);

    // Disable sync mode: should flush
    buffer.setSynchronizedMode(false);
    QCOMPARE(dataSpy.count(), 1);
}

void TestCSI2026::testSyncBufferClear()
{
    SynchronizedBuffer buffer;

    buffer.setSynchronizedMode(true);
    buffer.writeData("Test data");

    QVERIFY(buffer.bufferSize() > 0);

    buffer.clear();
    QCOMPARE(buffer.bufferSize(), (size_t)0);
}

void TestCSI2026::testSyncBufferOverflow()
{
    SynchronizedBuffer buffer;

    QSignalSpy overflowSpy(&buffer, &SynchronizedBuffer::bufferOverflow);

    buffer.setSynchronizedMode(true);
    buffer.setMaxBufferSize(100);  // Very small buffer

    // Write data larger than buffer
    buffer.writeData(QByteArray(200, 'X'));

    QCOMPARE(overflowSpy.count(), 1);
}

// ========== CSIParser Tests ==========

void TestCSI2026::testParseCSI2026Enable()
{
    CSIParser parser;

    // CSI ? 2026 h
    QByteArray data = "\x1B[?2026h";
    parser.processData(data);

    QVERIFY(parser.hasCommand());
    CSIParser::Command cmd = parser.takeCommand();
    QCOMPARE(cmd.type, CSIParser::DECSET_PRIVATE);
    QVERIFY(cmd.isPrivate);
    QCOMPARE(cmd.params.size(), 1);
    QCOMPARE(cmd.params[0], 2026);

    bool enable = false;
    QVERIFY(CSIParser::isSynchronizedOutputCommand(cmd, &enable));
    QVERIFY(enable);
}

void TestCSI2026::testParseCSI2026Disable()
{
    CSIParser parser;

    // CSI ? 2026 l
    QByteArray data = "\x1B[?2026l";
    parser.processData(data);

    QVERIFY(parser.hasCommand());
    CSIParser::Command cmd = parser.takeCommand();
    QCOMPARE(cmd.type, CSIParser::DECRST_PRIVATE);

    bool enable = true;
    QVERIFY(CSIParser::isSynchronizedOutputCommand(cmd, &enable));
    QVERIFY(!enable);
}

void TestCSI2026::testParseCursorMove()
{
    CSIParser parser;

    // CSI 10;20H - move cursor to row 10, column 20
    QByteArray data = "\x1B[10;20H";
    parser.processData(data);

    QVERIFY(parser.hasCommand());
    CSIParser::Command cmd = parser.takeCommand();
    QCOMPARE(cmd.type, CSIParser::CURSOR_POSITION);
    QCOMPARE(cmd.params.size(), 2);
    QCOMPARE(cmd.params[0], 10);
    QCOMPARE(cmd.params[1], 20);
}

void TestCSI2026::testParseSGR()
{
    CSIParser parser;

    // CSI 1;31;40m - bold, red foreground, black background
    QByteArray data = "\x1B[1;31;40m";
    parser.processData(data);

    QVERIFY(parser.hasCommand());
    CSIParser::Command cmd = parser.takeCommand();
    QCOMPARE(cmd.type, CSIParser::SELECT_GRAPHIC);
    QCOMPARE(cmd.params.size(), 3);
    QCOMPARE(cmd.params[0], 1);
    QCOMPARE(cmd.params[1], 31);
    QCOMPARE(cmd.params[2], 40);
}

void TestCSI2026::testParseMultipleCommands()
{
    CSIParser parser;

    // Multiple commands: clear screen + move cursor
    QByteArray data = "\x1B[2J\x1B[H";
    parser.processData(data);

    QVERIFY(parser.hasCommand());
    QList<CSIParser::Command> cmds = parser.takeAllCommands();
    QCOMPARE(cmds.size(), 2);

    QCOMPARE(cmds[0].type, CSIParser::ERASE_DISPLAY);
    QCOMPARE(cmds[0].params[0], 2);

    QCOMPARE(cmds[1].type, CSIParser::CURSOR_POSITION);
}

// ========== TerminalHandler Integration Tests ==========

void TestCSI2026::testHandlerSyncEnable()
{
    TerminalHandler handler;

    QSignalSpy syncSpy(&handler, &TerminalHandler::synchronizedOutputChanged);

    // Enable sync mode
    handler.processInput("\x1B[?2026h");

    QCOMPARE(syncSpy.count(), 1);
    QCOMPARE(syncSpy.takeFirst().at(0).toBool(), true);
    QVERIFY(handler.isSynchronizedOutputEnabled());
}

void TestCSI2026::testHandlerSyncDisable()
{
    TerminalHandler handler;

    // First enable
    handler.processInput("\x1B[?2026h");

    QSignalSpy syncSpy(&handler, &TerminalHandler::synchronizedOutputChanged);
    QSignalSpy renderSpy(&handler, &TerminalHandler::renderData);

    // Write some data
    handler.processInput("Test");

    // Disable sync mode - should trigger flush
    handler.processInput("\x1B[?2026l");

    QCOMPARE(syncSpy.count(), 1);
    QCOMPARE(syncSpy.takeFirst().at(0).toBool(), false);
    QVERIFY(!handler.isSynchronizedOutputEnabled());
}

void TestCSI2026::testHandlerNormalMode()
{
    TerminalHandler handler;

    QSignalSpy renderSpy(&handler, &TerminalHandler::renderData);

    // In normal mode, data should pass through
    handler.processInput("Hello\n");

    QCOMPARE(renderSpy.count(), 1);
}

void TestCSI2026::testHandlerComplexSequence()
{
    TerminalHandler handler;

    // Complex sequence from a TUI application
    QByteArray sequence;
    sequence.append("\x1B[?2026h");      // Enable sync
    sequence.append("\x1B[2J");           // Clear screen
    sequence.append("\x1B[H");            // Cursor home
    sequence.append("Menu\n");             // Content
    sequence.append("\x1B[1;31m");        // Bold red
    sequence.append("Item\n");             // Styled content
    sequence.append("\x1B[0m");            // Reset
    sequence.append("\x1B[?2026l");       // Disable sync (flush)

    QSignalSpy renderSpy(&handler, &TerminalHandler::renderData);
    QSignalSpy clearSpy(&handler, &TerminalHandler::clearScreenRequested);
    QSignalSpy cursorSpy(&handler, &TerminalHandler::cursorPositionChanged);

    handler.processInput(sequence);

    // Should have received clear screen signal
    QCOMPARE(clearSpy.count(), 1);

    // Should have received cursor position signal
    QCOMPARE(cursorSpy.count(), 1);

    // Should have batched data ready
    QVERIFY(renderSpy.count() > 0);
}

QTEST_MAIN(TestCSI2026)

#include "test_csi2026.moc"
