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

#ifndef TERMINALHANDLER_H
#define TERMINALHANDLER_H

#pragma once

#include <QObject>
#include <QPointer>
#include "TerminalMode.h"
#include "SynchronizedBuffer.h"
#include "CSIParser.h"

/**
 * @brief 终端处理管理器
 *
 * 整合 CSI 解析、同步输出缓冲和模式管理。
 * 这是终端模拟器的核心入口点。
 *
 * 使用示例:
 * @code
 *   TerminalHandler handler;
 *   connect(&handler, &TerminalHandler::renderData, this, &MyRenderer::render);
 *
 *   // 处理输入数据
 *   handler.processInput(data);
 * @endcode
 */
class TerminalHandler : public QObject
{
    Q_OBJECT

public:
    explicit TerminalHandler(QObject *parent = nullptr);
    ~TerminalHandler();

    /**
     * @brief 处理输入数据
     * @param data 原始终端输入数据
     *
     * 数据可能包含普通文本和 CSI 控制序列混合。
     * 方法会自动解析 CSI 2026 序列并管理同步输出。
     */
    void processInput(const QByteArray &data);

    /**
     * @brief 获取模式管理器
     */
    TerminalMode *modeManager() const { return m_modeManager; }

    /**
     * @brief 获取同步缓冲区
     */
    SynchronizedBuffer *syncBuffer() const { return m_syncBuffer; }

    /**
     * @brief 获取 CSI 解析器
     */
    CSIParser *csiParser() const { return m_csiParser; }

    /**
     * @brief 强制刷新所有缓冲数据
     *
     * 即使在同步模式下也会立即渲染
     */
    void forceFlush();

    /**
     * @brief 清空所有缓冲数据
     */
    void clearBuffer();

    /**
     * @brief 设置调试模式
     */
    void setDebugEnabled(bool enabled) { m_debugEnabled = enabled; }

    /**
     * @brief 检查同步输出是否启用
     */
    bool isSynchronizedOutputEnabled() const;

Q_SIGNALS:
    /**
     * @brief 需要渲染的数据
     * @param data 渲染数据
     * @param type 数据类型
     */
    void renderData(const QByteArray &data, SynchronizedBuffer::BufferItemType type);

    /**
     * @brief 光标移动请求
     * @param row 目标行
     * @param column 目标列
     */
    void cursorPositionChanged(int row, int column);

    /**
     * @brief 屏幕清除请求
     * @param mode 清除模式 (0=光标到末尾, 1=开头到光标, 2=整个屏幕)
     */
    void clearScreenRequested(int mode);

    /**
     * @brief 属性变化（颜色、样式等）
     * @param sgrParams SGR 参数
     */
    void attributeChanged(const QList<int> &sgrParams);

    /**
     * @brief 同步输出模式状态变化
     * @param enabled 新的状态
     */
    void synchronizedOutputChanged(bool enabled);

private Q_SLOTS:
    void onCSIParsed(const CSIParser::Command &cmd);
    void onSyncDataReady(const QList<SynchronizedBuffer::BufferItem> &items);
    void onModeChanged(TerminalMode::Mode mode, bool enabled);

private:
    QPointer<TerminalMode> m_modeManager;
    QPointer<SynchronizedBuffer> m_syncBuffer;
    QPointer<CSIParser> m_csiParser;

    QByteArray m_pendingText;  // 等待处理的普通文本
    bool m_debugEnabled = false;

    void processCommand(const CSIParser::Command &cmd);
    void flushPendingText();
    void handleSGR(const QList<int> &params);
    void handleDECSET(int mode);
    void handleDECRST(int mode);
    void handleCursorMove(const CSIParser::Command &cmd);
    void handleErase(const CSIParser::Command &cmd);
};

#endif // TERMINALHANDLER_H
