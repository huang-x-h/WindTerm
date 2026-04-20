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

#ifndef CSIPARSER_H
#define CSIPARSER_H

#pragma once

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QString>
#include "Utility/Spin.h"

/**
 * @brief CSI 控制序列解析器
 *
 * 解析 ANSI/VT 控制序列，特别是：
 * - CSI Ps h:  DECSET - 设置模式
 * - CSI Ps l:  DECRST - 重置模式
 * - CSI ? Ps h/l: 私有 DECSET/DECRST（包括同步输出模式 2026）
 */
class CSIParser : public QObject
{
    Q_OBJECT

public:
    explicit CSIParser(QObject *parent = nullptr);
    ~CSIParser() = default;

    /**
     * @brief CSI 命令类型
     */
    enum CommandType {
        UNKNOWN,
        CURSOR_UP,          // CSI A
        CURSOR_DOWN,        // CSI B
        CURSOR_FORWARD,     // CSI C
        CURSOR_BACKWARD,    // CSI D
        CURSOR_NEXT_LINE,   // CSI E
        CURSOR_PREV_LINE,   // CSI F
        CURSOR_HORIZONTAL,  // CSI G
        CURSOR_POSITION,    // CSI H
        ERASE_DISPLAY,      // CSI J
        ERASE_LINE,         // CSI K
        SCROLL_UP,          // CSI S
        SCROLL_DOWN,        // CSI T
        HORIZONTAL_POS,     // CSI f
        SELECT_GRAPHIC,     // CSI m  (SGR)
        DEVICE_STATUS,      // CSI n
        DECSET,             // CSI h  (DEC Private Mode Set)
        DECRST,             // CSI l  (DEC Private Mode Reset)
        DECSET_PRIVATE,     // CSI ? h
        DECRST_PRIVATE,     // CSI ? l
        SAVE_CURSOR,        // CSI s
        RESTORE_CURSOR,     // CSI u
        SOFT_RESET,         // CSI ! p
    };

    /**
     * @brief 解析后的 CSI 命令
     */
    struct Command {
        CommandType type;
        QList<int> params;      // 参数列表
        QByteArray intermediate; // 中间字符（如 ?、>、! 等）
        char finalChar;         // 最终字符（A-Z, a-z, @, ` 等）
        bool isPrivate;         // 是否为私有序列（包含 ?）

        Command()
            : type(UNKNOWN)
            , finalChar('\0')
            , isPrivate(false)
        {}
    };

    /**
     * @brief 解析状态
     */
    enum ParseState {
        STATE_GROUND,           // 初始状态
        STATE_ESCAPE,           // 收到 ESC
        STATE_CSI_ENTRY,        // 收到 CSI  introducer ([)
        STATE_CSI_PARAM,        // 解析参数
        STATE_CSI_INTERMEDIATE, // 解析中间字符
        STATE_CSI_FINAL,        // 解析最终字符
    };

    /**
     * @brief 处理输入字节
     * @param byte 输入字节
     * @return 如果完成一个完整命令的解析返回 true
     *
     * 调用者应检查 hasCommand() 来获取解析完成的命令
     */
    bool processByte(char byte);

    /**
     * @brief 处理字节数组
     * @param data 输入数据
     * @return 未处理的剩余数据（不完整序列）
     */
    QByteArray processData(const QByteArray &data);

    /**
     * @brief 检查是否有解析完成的命令
     */
    bool hasCommand() const;

    /**
     * @brief 获取并移除下一个解析完成的命令
     */
    Command takeCommand();

    /**
     * @brief 获取所有解析完成的命令
     */
    QList<Command> takeAllCommands();

    /**
     * @brief 重置解析器状态
     */
    void reset();

    /**
     * @brief 解析器是否处于空闲状态
     */
    bool isIdle() const;

    /**
     * @brief 获取当前状态
     */
    ParseState state() const { return m_state; }

    /**
     * @brief 检查序列是否是同步输出模式
     * @param cmd 解析后的命令
     * @return 如果是同步输出相关命令返回 true，并通过参数返回具体动作
     */
    static bool isSynchronizedOutputCommand(const Command &cmd, bool *enable);

    /**
     * @brief 将命令转换为可读字符串（用于调试）
     */
    static QString commandToString(const Command &cmd);

Q_SIGNALS:
    /**
     * @brief 命令解析完成
     */
    void commandParsed(const Command &cmd);

    /**
     * @brief 解析错误
     */
    void parseError(const QByteArray &context);

private:
    mutable SpinMutex m_mutex;

    ParseState m_state;
    QByteArray m_buffer;
    QList<int> m_params;
    QByteArray m_intermediate;
    QList<Command> m_completedCommands;

    void handleGround(char byte);
    void handleEscape(char byte);
    void handleCSIEntry(char byte);
    void handleCSIParam(char byte);
    void handleCSIIntermediate(char byte);

    void completeCommand(char finalChar);
    CommandType determineCommandType(char finalChar, bool isPrivate);
    void clearState();

    static constexpr char ESC = '\x1B';  // 27
    static constexpr char CSI = '[';      // 91
};

#endif // CSIPARSER_H
