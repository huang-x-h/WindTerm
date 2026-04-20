/*
 * PtyWithCSI2026.h
 *
 * Example of how to modify WindTerm's Pty class to support CSI 2026
 * This shows the minimal changes needed for integration
 */

#ifndef PTYWITHCSI2026_H
#define PTYWITHCSI2026_H

#pragma once

#include <QObject>
#include <QByteArray>
#include "Terminal/TerminalHandler.h"

// Forward declarations - replace with actual WindTerm types
class RenderEngine;
class QWinEventNotifier;

/**
 * @brief Example Pty class with CSI 2026 support
 *
 * This shows how to integrate TerminalHandler into the existing Pty class.
 * Copy relevant parts to WindTerm's actual Pty.h/cpp files.
 */
class PtyWithCSI2026 : public QObject
{
    Q_OBJECT

public:
    PtyWithCSI2026();
    virtual ~PtyWithCSI2026() = default;

    // Existing methods
    virtual bool createProcess(QString command, const QString &arguments,
                               const QString &workingDirectory, const QStringList &environment,
                               qint16 rows, qint16 columns) = 0;

    virtual QByteArray readAll() = 0;
    virtual bool resizeWindow(qint16 rows, qint16 columns) = 0;
    virtual qint64 write(const QByteArray &text) = 0;

    // ===== CSI 2026 ADDED METHODS =====

    /**
     * @brief Check if synchronized output mode is active
     */
    bool isSynchronizedOutputActive() const {
        return m_terminalHandler.isSynchronizedOutputEnabled();
    }

    /**
     * @brief Force flush of any buffered data
     */
    void forceFlush() {
        m_terminalHandler.forceFlush();
    }

    /**
     * @brief Get the terminal handler (for advanced usage)
     */
    TerminalHandler *terminalHandler() { return &m_terminalHandler; }

Q_SIGNALS:
    // Existing signals
    void errorOccurred();
    void readyRead();

    // ===== CSI 2026 ADDED SIGNALS =====

    /**
     * @brief Data is ready for rendering
     */
    void renderDataReady(const QByteArray &data, int type);

    /**
     * @brief Cursor position has changed
     */
    void cursorPositionChanged(int row, int column);

    /**
     * @brief Screen clear requested
     */
    void clearScreenRequested(int mode);

    /**
     * @brief Text attributes changed (colors, styles)
     */
    void attributesChanged(const QList<int> &params);

protected:
    // Existing members
    qint16 m_columns;
    qint16 m_rows;

    // ===== CSI 2026 ADDED MEMBER =====
    TerminalHandler m_terminalHandler;

protected Q_SLOTS:
    // ===== CSI 2026 ADDED SLOTS =====

    /**
     * @brief Handle data from TerminalHandler
     */
    void onTerminalRenderData(const QByteArray &data,
                              SynchronizedBuffer::BufferItemType type);

    /**
     * @brief Handle cursor position changes
     */
    void onTerminalCursorPositionChanged(int row, int column);

    /**
     * @brief Handle screen clear requests
     */
    void onTerminalClearScreen(int mode);

    /**
     * @brief Handle attribute changes
     */
    void onTerminalAttributesChanged(const QList<int> &params);

private:
    /**
     * @brief Process raw data through TerminalHandler
     * Call this from readAll() implementations
     */
    void processTerminalData(const QByteArray &rawData);
};

#endif // PTYWITHCSI2026_H
