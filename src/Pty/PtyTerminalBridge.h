/*
 * PtyTerminalBridge.h
 *
 * Bridges Pty layer with TerminalHandler for CSI 2026 support
 * Add this file to WindTerm's Pty directory or appropriate location
 */

#ifndef PTYTERMINALBRIDGE_H
#define PTYTERMINALBRIDGE_H

#pragma once

#include <QObject>
#include <QPointer>
#include "Terminal/TerminalHandler.h"

// Forward declaration - replace with actual WindTerm render engine class
class RenderEngine;
class Pty;

/**
 * @brief Bridges Pty I/O with Terminal processing
 *
 * This class connects the raw data from Pty to TerminalHandler
 * and forwards processed data to the rendering engine.
 */
class PtyTerminalBridge : public QObject
{
    Q_OBJECT

public:
    explicit PtyTerminalBridge(Pty *pty, RenderEngine *engine, QObject *parent = nullptr);
    ~PtyTerminalBridge();

    /**
     * @brief Initialize the bridge connections
     */
    void initialize();

    /**
     * @brief Get the terminal handler
     */
    TerminalHandler *handler() const { return m_handler; }

    /**
     * @brief Check if synchronized output is active
     */
    bool isSynchronizedOutputActive() const;

    /**
     * @brief Force immediate flush of buffered data
     */
    void forceFlush();

public slots:
    /**
     * @brief Called when Pty has data ready
     */
    void onPtyDataReady();

    /**
     * @brief Called when window is resized
     */
    void onWindowResized(int rows, int columns);

private slots:
    void onRenderData(const QByteArray &data, SynchronizedBuffer::BufferItemType type);
    void onCursorPositionChanged(int row, int column);
    void onClearScreenRequested(int mode);
    void onAttributeChanged(const QList<int> &sgrParams);
    void onSynchronizedOutputChanged(bool enabled);

private:
    QPointer<Pty> m_pty;
    QPointer<RenderEngine> m_engine;
    TerminalHandler *m_handler;

    // Statistics for debugging
    struct Stats {
        qint64 totalBytesProcessed = 0;
        qint64 syncFramesRendered = 0;
        qint64 normalFramesRendered = 0;
        qint64 commandsParsed = 0;
    } m_stats;

    void updateStats(SynchronizedBuffer::BufferItemType type);
    void connectToRenderEngine();
};

#endif // PTYTERMINALBRIDGE_H
