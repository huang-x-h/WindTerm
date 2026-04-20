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

#include "TerminalHandler.h"

#include <QDebug>

TerminalHandler::TerminalHandler(QObject *parent)
    : QObject(parent)
    , m_modeManager(new TerminalMode(this))
    , m_syncBuffer(new SynchronizedBuffer(this))
    , m_csiParser(new CSIParser(this))
{
    // 连接信号
    connect(m_csiParser, &CSIParser::commandParsed,
            this, &TerminalHandler::onCSIParsed);

    connect(m_syncBuffer, &SynchronizedBuffer::dataReady,
            this, &TerminalHandler::onSyncDataReady);

    connect(m_modeManager, &TerminalMode::modeChanged,
            this, &TerminalHandler::onModeChanged);

    connect(m_modeManager, &TerminalMode::synchronizedOutputChanged,
            this, &TerminalHandler::synchronizedOutputChanged);
}

TerminalHandler::~TerminalHandler() = default;

void TerminalHandler::processInput(const QByteArray &data)
{
    QByteArray remaining = data;

    while (!remaining.isEmpty()) {
        // 找到 CSI 序列的开始
        int escPos = remaining.indexOf('\x1B');

        if (escPos == -1) {
            // 没有 CSI 序列，全部作为普通文本
            m_pendingText.append(remaining);
            flushPendingText();
            break;
        }

        if (escPos > 0) {
            // CSI 之前有普通文本
            m_pendingText.append(remaining.left(escPos));
            flushPendingText();
            remaining = remaining.mid(escPos);
        }

        // 解析 CSI 序列
        QByteArray leftover = m_csiParser->processData(remaining);

        // 处理所有解析完成的命令
        while (m_csiParser->hasCommand()) {
            CSIParser::Command cmd = m_csiParser->takeCommand();
            processCommand(cmd);
        }

        remaining = leftover;

        if (remaining == data) {
            // 没有进展，避免无限循环
            break;
        }
    }
}

void TerminalHandler::forceFlush()
{
    flushPendingText();
    m_syncBuffer->flush();
}

void TerminalHandler::clearBuffer()
{
    m_pendingText.clear();
    m_syncBuffer->clear();
}

bool TerminalHandler::isSynchronizedOutputEnabled() const
{
    return m_modeManager->isEnabled(TerminalMode::SYNCHRONIZED_OUTPUT);
}

void TerminalHandler::onCSIParsed(const CSIParser::Command &cmd)
{
    if (m_debugEnabled) {
        qDebug() << "CSI:" << CSIParser::commandToString(cmd);
    }

    processCommand(cmd);
}

void TerminalHandler::onSyncDataReady(const QList<SynchronizedBuffer::BufferItem> &items)
{
    for (const auto &item : items) {
        emit renderData(item.data, item.type);
    }
}

void TerminalHandler::onModeChanged(TerminalMode::Mode mode, bool enabled)
{
    if (mode == TerminalMode::SYNCHRONIZED_OUTPUT) {
        m_syncBuffer->setSynchronizedMode(enabled);
    }
}

void TerminalHandler::processCommand(const CSIParser::Command &cmd)
{
    switch (cmd.type) {
    case CSIParser::DECSET:
        if (!cmd.params.isEmpty()) {
            handleDECSET(cmd.params.first());
        }
        break;

    case CSIParser::DECRST:
        if (!cmd.params.isEmpty()) {
            handleDECRST(cmd.params.first());
        }
        break;

    case CSIParser::DECSET_PRIVATE:
        if (!cmd.params.isEmpty()) {
            // 检查是否是同步输出模式
            bool syncEnable;
            if (CSIParser::isSynchronizedOutputCommand(cmd, &syncEnable)) {
                if (syncEnable) {
                    m_modeManager->setMode(TerminalMode::SYNCHRONIZED_OUTPUT);
                }
            } else {
                handleDECSET(cmd.params.first());
            }
        }
        break;

    case CSIParser::DECRST_PRIVATE:
        if (!cmd.params.isEmpty()) {
            // 检查是否是同步输出模式
            bool syncEnable;
            if (CSIParser::isSynchronizedOutputCommand(cmd, &syncEnable)) {
                if (!syncEnable) {
                    m_modeManager->resetMode(TerminalMode::SYNCHRONIZED_OUTPUT);
                }
            } else {
                handleDECRST(cmd.params.first());
            }
        }
        break;

    case CSIParser::CURSOR_UP:
    case CSIParser::CURSOR_DOWN:
    case CSIParser::CURSOR_FORWARD:
    case CSIParser::CURSOR_BACKWARD:
    case CSIParser::CURSOR_NEXT_LINE:
    case CSIParser::CURSOR_PREV_LINE:
    case CSIParser::CURSOR_HORIZONTAL:
    case CSIParser::CURSOR_POSITION:
    case CSIParser::HORIZONTAL_POS:
        handleCursorMove(cmd);
        break;

    case CSIParser::ERASE_DISPLAY:
    case CSIParser::ERASE_LINE:
        handleErase(cmd);
        break;

    case CSIParser::SELECT_GRAPHIC:
        handleSGR(cmd.params);
        break;

    default:
        if (m_debugEnabled) {
            qDebug() << "Unhandled CSI command:" << CSIParser::commandToString(cmd);
        }
        break;
    }
}

void TerminalHandler::flushPendingText()
{
    if (m_pendingText.isEmpty()) {
        return;
    }

    SynchronizedBuffer::BufferItem item;
    item.type = SynchronizedBuffer::TEXT_DATA;
    item.data = m_pendingText;

    m_syncBuffer->write(item);
    m_pendingText.clear();
}

void TerminalHandler::handleSGR(const QList<int> &params)
{
    flushPendingText();

    SynchronizedBuffer::BufferItem item;
    item.type = SynchronizedBuffer::ATTRIBUTE_CHANGE;
    // 序列化参数到数据
    QByteArray data;
    for (int param : params) {
        data.append(QString::number(param).toLatin1());
        data.append(';');
    }
    item.data = data;

    m_syncBuffer->write(item);
    emit attributeChanged(params);
}

void TerminalHandler::handleDECSET(int mode)
{
    switch (mode) {
    case 25:  // 显示光标
        m_modeManager->setMode(TerminalMode::SHOW_CURSOR);
        break;
    case 1047:  // 备用屏幕
        m_modeManager->setMode(TerminalMode::ALTERNATE_SCREEN);
        break;
    case 1049:  // 备用屏幕不清空
        m_modeManager->setMode(TerminalMode::ALTERNATE_SCREEN_NO_CLEAR);
        break;
    case 2004:  // 括号粘贴模式
        m_modeManager->setMode(TerminalMode::BRACKETED_PASTE);
        break;
    default:
        break;
    }
}

void TerminalHandler::handleDECRST(int mode)
{
    switch (mode) {
    case 25:  // 隐藏光标
        m_modeManager->resetMode(TerminalMode::SHOW_CURSOR);
        break;
    case 1047:  // 返回主屏幕
        m_modeManager->resetMode(TerminalMode::ALTERNATE_SCREEN);
        break;
    case 1049:  // 返回主屏幕
        m_modeManager->resetMode(TerminalMode::ALTERNATE_SCREEN_NO_CLEAR);
        break;
    case 2004:  // 禁用括号粘贴模式
        m_modeManager->resetMode(TerminalMode::BRACKETED_PASTE);
        break;
    default:
        break;
    }
}

void TerminalHandler::handleCursorMove(const CSIParser::Command &cmd)
{
    flushPendingText();

    int n = cmd.params.isEmpty() ? 1 : cmd.params.first();
    int row = 0, col = 0;

    switch (cmd.type) {
    case CSIParser::CURSOR_UP:
        row = -n;
        break;
    case CSIParser::CURSOR_DOWN:
        row = n;
        break;
    case CSIParser::CURSOR_FORWARD:
        col = n;
        break;
    case CSIParser::CURSOR_BACKWARD:
        col = -n;
        break;
    case CSIParser::CURSOR_NEXT_LINE:
        row = n;
        col = 0;
        break;
    case CSIParser::CURSOR_PREV_LINE:
        row = -n;
        col = 0;
        break;
    case CSIParser::CURSOR_HORIZONTAL:
        col = n - 1;  // 1-based to 0-based
        break;
    case CSIParser::CURSOR_POSITION:
    case CSIParser::HORIZONTAL_POS:
        row = cmd.params.size() > 0 ? cmd.params[0] : 1;
        col = cmd.params.size() > 1 ? cmd.params[1] : 1;
        row = row > 0 ? row - 1 : 0;  // 1-based to 0-based
        col = col > 0 ? col - 1 : 0;
        break;
    default:
        break;
    }

    SynchronizedBuffer::BufferItem item;
    item.type = SynchronizedBuffer::CURSOR_MOVE;
    item.param1 = row;
    item.param2 = col;

    m_syncBuffer->write(item);
    emit cursorPositionChanged(row, col);
}

void TerminalHandler::handleErase(const CSIParser::Command &cmd)
{
    flushPendingText();

    int mode = cmd.params.isEmpty() ? 0 : cmd.params.first();

    SynchronizedBuffer::BufferItem item;
    item.type = (cmd.type == CSIParser::ERASE_DISPLAY)
                    ? SynchronizedBuffer::CLEAR_SCREEN
                    : SynchronizedBuffer::BufferItemType(5);  // ERASE_LINE
    item.param1 = mode;

    m_syncBuffer->write(item);
    emit clearScreenRequested(mode);
}
