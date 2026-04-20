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

#ifndef TERMINALMODE_H
#define TERMINALMODE_H

#pragma once

#include <QObject>
#include <QSet>
#include <QHash>
#include "Utility/Spin.h"

/**
 * @brief 终端模式管理类
 *
 * 管理 DECSET/DECRST 模式状态，包括：
 * - 同步输出模式 (2026): 批量处理终端更新，避免屏幕闪烁
 * - 其他标准模式 (如: 插入模式、自动换行等)
 */
class TerminalMode : public QObject
{
    Q_OBJECT

public:
    explicit TerminalMode(QObject *parent = nullptr);
    ~TerminalMode() = default;

    /**
     * @brief 标准 DECSET 模式编号
     */
    enum Mode : int {
        // 标准模式 (1-1999)
        INSERT_MODE = 4,
        ORIGIN_MODE = 6,
        AUTO_WRAP = 7,
        AUTO_REPEAT = 8,
        TRACK_MOUSE = 9,
        SHOW_CURSOR = 25,
        ALTERNATE_SCREEN = 1047,
        ALTERNATE_SCREEN_NO_CLEAR = 1049,
        BRACKETED_PASTE = 2004,

        // 同步输出模式 (2026)
        SYNCHRONIZED_OUTPUT = 2026,

        // 其他扩展模式
        FOCUS_EVENTS = 1004,
        MOUSE_PROTOCOL_UTF8 = 1005,
        MOUSE_PROTOCOL_SGR = 1006,
        MOUSE_PROTOCOL_URXVT = 1015,
    };

    /**
     * @brief 启用指定模式 (DECSET)
     * @param mode 模式编号
     * @return 如果模式状态发生变化返回 true
     */
    bool setMode(Mode mode);

    /**
     * @brief 禁用指定模式 (DECRST)
     * @param mode 模式编号
     * @return 如果模式状态发生变化返回 true
     */
    bool resetMode(Mode mode);

    /**
     * @brief 切换指定模式状态
     * @param mode 模式编号
     * @return 切换后的状态
     */
    bool toggleMode(Mode mode);

    /**
     * @brief 查询模式是否启用
     * @param mode 模式编号
     * @return 如果模式已启用返回 true
     */
    bool isEnabled(Mode mode) const;

    /**
     * @brief 查询同步输出模式状态
     * @return 如果同步输出已启用返回 true
     */
    bool isSynchronizedOutputEnabled() const {
        return isEnabled(SYNCHRONIZED_OUTPUT);
    }

    /**
     * @brief 保存当前所有模式状态
     */
    void saveModes();

    /**
     * @brief 恢复上次保存的模式状态
     */
    void restoreModes();

    /**
     * @brief 重置所有模式到默认状态
     */
    void resetAllModes();

Q_SIGNALS:
    /**
     * @brief 模式状态改变信号
     * @param mode 改变的模式
     * @param enabled 新的状态
     */
    void modeChanged(Mode mode, bool enabled);

    /**
     * @brief 同步输出模式改变信号
     * @param enabled 新的状态
     */
    void synchronizedOutputChanged(bool enabled);

    /**
     * @brief 请求立即刷新屏幕
     * 当同步输出模式禁用时触发
     */
    void flushRequested();

private:
    mutable SpinMutex m_mutex;

    // 当前模式状态集合
    QSet<Mode> m_enabledModes;

    // 保存的模式状态（用于 DECSC/DECRC）
    QSet<Mode> m_savedModes;

    // 记录模式变化，用于批量通知
    QHash<Mode, bool> m_pendingChanges;

    void emitModeChanges();
};

#endif // TERMINALMODE_H
