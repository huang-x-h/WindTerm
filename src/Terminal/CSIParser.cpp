/*
 * Copyright 2020, WindTerm.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with License.
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

#include "CSIParser.h"

#include <QDebug>

CSIParser::CSIParser(QObject *parent)
    : QObject(parent)
    , m_state(STATE_GROUND)
{
}

bool CSIParser::processByte(char byte)
{
    ThreadLocker<SpinMutex> locker(m_mutex);

    switch (m_state) {
    case STATE_GROUND:
        handleGround(byte);
        break;
    case STATE_ESCAPE:
        handleEscape(byte);
        break;
    case STATE_CSI_ENTRY:
        handleCSIEntry(byte);
        break;
    case STATE_CSI_PARAM:
        handleCSIParam(byte);
        break;
    case STATE_CSI_INTERMEDIATE:
        handleCSIIntermediate(byte);
        break;
    case STATE_CSI_FINAL:
        // 不应该到达这里
        break;
    }

    return hasCommand();
}

QByteArray CSIParser::processData(const QByteArray &data)
{
    for (char byte : data) {
        processByte(byte);
    }

    // 如果有不完整的序列，返回它
    ThreadLocker<SpinMutex> locker(m_mutex);
    if (m_state != STATE_GROUND && !m_buffer.isEmpty()) {
        QByteArray incomplete = m_buffer;
        clearState();
        return incomplete;
    }
    return QByteArray();
}

bool CSIParser::hasCommand() const
{
    ThreadLocker<SpinMutex> locker(m_mutex);
    return !m_completedCommands.isEmpty();
}

CSIParser::Command CSIParser::takeCommand()
{
    ThreadLocker<SpinMutex> locker(m_mutex);
    if (m_completedCommands.isEmpty()) {
        return Command();
    }
    return m_completedCommands.takeFirst();
}

QList<CSIParser::Command> CSIParser::takeAllCommands()
{
    ThreadLocker<SpinMutex> locker(m_mutex);
    QList<Command> commands = m_completedCommands;
    m_completedCommands.clear();
    return commands;
}

void CSIParser::reset()
{
    ThreadLocker<SpinMutex> locker(m_mutex);
    clearState();
    m_completedCommands.clear();
}

bool CSIParser::isIdle() const
{
    ThreadLocker<SpinMutex> locker(m_mutex);
    return m_state == STATE_GROUND;
}

void CSIParser::handleGround(char byte)
{
    if (byte == ESC) {
        m_state = STATE_ESCAPE;
        m_buffer.append(byte);
    }
    // 其他字符在地面状态被忽略或作为普通数据
}

void CSIParser::handleEscape(char byte)
{
    if (byte == CSI) {
        m_state = STATE_CSI_ENTRY;
        m_buffer.append(byte);
    } else {
        // 不是 CSI 序列，回到地面状态
        clearState();
    }
}

void CSIParser::handleCSIEntry(char byte)
{
    if (byte >= '0' && byte <= '9') {
        // 开始参数
        m_state = STATE_CSI_PARAM;
        m_params.append(byte - '0');
        m_buffer.append(byte);
    } else if (byte == ';') {
        // 空参数
        m_state = STATE_CSI_PARAM;
        m_params.append(0);
        m_buffer.append(byte);
    } else if (byte >= 0x20 && byte <= 0x2F) {
        // 中间字符
        m_state = STATE_CSI_INTERMEDIATE;
        m_intermediate.append(byte);
        m_buffer.append(byte);
    } else if (byte >= 0x40 && byte <= 0x7E) {
        // 立即完成的序列（无参数）
        completeCommand(byte);
    } else {
        // 无效序列
        emit parseError(m_buffer);
        clearState();
    }
}

void CSIParser::handleCSIParam(char byte)
{
    if (byte >= '0' && byte <= '9') {
        // 继续参数
        if (!m_params.isEmpty()) {
            m_params.last() = m_params.last() * 10 + (byte - '0');
        }
        m_buffer.append(byte);
    } else if (byte == ';') {
        // 下一个参数
        m_params.append(0);
        m_buffer.append(byte);
    } else if (byte >= 0x20 && byte <= 0x2F) {
        // 中间字符
        m_state = STATE_CSI_INTERMEDIATE;
        m_intermediate.append(byte);
        m_buffer.append(byte);
    } else if (byte >= 0x40 && byte <= 0x7E) {
        // 最终字符，完成序列
        completeCommand(byte);
    } else {
        // 无效字符
        emit parseError(m_buffer);
        clearState();
    }
}

void CSIParser::handleCSIIntermediate(char byte)
{
    if (byte >= 0x20 && byte <= 0x2F) {
        // 继续中间字符
        m_intermediate.append(byte);
        m_buffer.append(byte);
    } else if (byte >= 0x40 && byte <= 0x7E) {
        // 最终字符，完成序列
        completeCommand(byte);
    } else {
        // 无效字符
        emit parseError(m_buffer);
        clearState();
    }
}

void CSIParser::completeCommand(char finalChar)
{
    Command cmd;
    cmd.params = m_params;
    cmd.intermediate = m_intermediate;
    cmd.finalChar = finalChar;
    cmd.isPrivate = m_intermediate.contains('?');
    cmd.type = determineCommandType(finalChar, cmd.isPrivate);

    m_completedCommands.append(cmd);

    emit commandParsed(cmd);

    clearState();
}

CSIParser::CommandType CSIParser::determineCommandType(char finalChar, bool isPrivate)
{
    switch (finalChar) {
    case 'A': return CURSOR_UP;
    case 'B': return CURSOR_DOWN;
    case 'C': return CURSOR_FORWARD;
    case 'D': return CURSOR_BACKWARD;
    case 'E': return CURSOR_NEXT_LINE;
    case 'F': return CURSOR_PREV_LINE;
    case 'G': return CURSOR_HORIZONTAL;
    case 'H': return CURSOR_POSITION;
    case 'J': return ERASE_DISPLAY;
    case 'K': return ERASE_LINE;
    case 'S': return SCROLL_UP;
    case 'T': return SCROLL_DOWN;
    case 'f': return HORIZONTAL_POS;
    case 'm': return SELECT_GRAPHIC;
    case 'n': return DEVICE_STATUS;
    case 'h': return isPrivate ? DECSET_PRIVATE : DECSET;
    case 'l': return isPrivate ? DECRST_PRIVATE : DECRST;
    case 's': return SAVE_CURSOR;
    case 'u': return RESTORE_CURSOR;
    case 'p':
        if (m_intermediate.contains('!')) {
            return SOFT_RESET;
        }
        return UNKNOWN;
    default:
        return UNKNOWN;
    }
}

void CSIParser::clearState()
{
    m_state = STATE_GROUND;
    m_buffer.clear();
    m_params.clear();
    m_intermediate.clear();
}

bool CSIParser::isSynchronizedOutputCommand(const Command &cmd, bool *enable)
{
    // 同步输出模式: CSI ? 2026 h/l
    if (!cmd.isPrivate) {
        return false;
    }

    if (cmd.params.isEmpty() || cmd.params.first() != 2026) {
        return false;
    }

    if (cmd.type == DECSET_PRIVATE) {
        if (enable) *enable = true;
        return true;
    } else if (cmd.type == DECRST_PRIVATE) {
        if (enable) *enable = false;
        return true;
    }

    return false;
}

QString CSIParser::commandToString(const Command &cmd)
{
    QString str = QString("CSI ");

    if (cmd.isPrivate) {
        str += "? ";
    }

    for (int i = 0; i < cmd.params.size(); ++i) {
        if (i > 0) str += ";";
        str += QString::number(cmd.params[i]);
    }

    if (!cmd.intermediate.isEmpty() && !cmd.isPrivate) {
        str += QString::fromLatin1(cmd.intermediate);
    }

    str += QChar(cmd.finalChar);

    str += " [" + QString::number(cmd.type) + "]";

    return str;
}
