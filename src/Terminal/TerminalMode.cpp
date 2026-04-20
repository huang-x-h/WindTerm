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

#include "TerminalMode.h"

TerminalMode::TerminalMode(QObject *parent)
    : QObject(parent)
{
    // 初始化默认模式状态
    resetAllModes();
}

bool TerminalMode::setMode(Mode mode)
{
    bool changed = false;

    {
        ThreadLocker<SpinMutex> locker(m_mutex);

        if (m_enabledModes.contains(mode)) {
            return false; // 模式已经是启用状态
        }

        m_enabledModes.insert(mode);
        m_pendingChanges[mode] = true;
        changed = true;
    } // 锁在这里自动释放

    if (changed) {
        emitModeChanges();
    }

    return changed;
}

bool TerminalMode::resetMode(Mode mode)
{
    bool changed = false;
    bool needFlush = false;

    {
        ThreadLocker<SpinMutex> locker(m_mutex);

        if (!m_enabledModes.contains(mode)) {
            return false; // 模式已经是禁用状态
        }

        m_enabledModes.remove(mode);
        m_pendingChanges[mode] = false;
        changed = true;

        // 如果是同步输出模式被禁用，触发立即刷新
        needFlush = (mode == SYNCHRONIZED_OUTPUT);
    } // 锁在这里自动释放

    if (changed) {
        emitModeChanges();
    }

    if (needFlush) {
        emit flushRequested();
    }

    return changed;
}

bool TerminalMode::toggleMode(Mode mode)
{
    bool currentlyEnabled;

    {
        ThreadLocker<SpinMutex> locker(m_mutex);
        currentlyEnabled = m_enabledModes.contains(mode);
    }

    if (currentlyEnabled) {
        return resetMode(mode);
    } else {
        return setMode(mode);
    }
}

bool TerminalMode::isEnabled(Mode mode) const
{
    ThreadLocker<SpinMutex> locker(m_mutex);
    return m_enabledModes.contains(mode);
}

void TerminalMode::saveModes()
{
    ThreadLocker<SpinMutex> locker(m_mutex);
    m_savedModes = m_enabledModes;
}

void TerminalMode::restoreModes()
{
    bool changed = false;
    bool syncWasDisabled = false;

    {
        ThreadLocker<SpinMutex> locker(m_mutex);

        // 计算需要禁用的模式
        QSet<Mode> toDisable = m_enabledModes - m_savedModes;
        // 计算需要启用的模式
        QSet<Mode> toEnable = m_savedModes - m_enabledModes;

        if (toDisable.isEmpty() && toEnable.isEmpty()) {
            return;
        }

        m_enabledModes = m_savedModes;

        for (Mode mode : toDisable) {
            m_pendingChanges[mode] = false;
        }
        for (Mode mode : toEnable) {
            m_pendingChanges[mode] = true;
        }

        syncWasDisabled = toDisable.contains(SYNCHRONIZED_OUTPUT);
        changed = true;
    }

    if (changed) {
        emitModeChanges();
    }

    if (syncWasDisabled) {
        emit flushRequested();
    }
}

void TerminalMode::resetAllModes()
{
    bool syncWasEnabled = false;
    bool changed = false;

    {
        ThreadLocker<SpinMutex> locker(m_mutex);

        syncWasEnabled = m_enabledModes.contains(SYNCHRONIZED_OUTPUT);

        m_enabledModes.clear();
        m_pendingChanges.clear();

        // 默认启用的模式
        m_enabledModes.insert(AUTO_WRAP);
        m_pendingChanges[AUTO_WRAP] = true;
        changed = true;
    }

    if (syncWasEnabled) {
        emit flushRequested();
    }
}

void TerminalMode::emitModeChanges()
{
    // 批量发送信号
    QHash<Mode, bool> changes;
    {
        ThreadLocker<SpinMutex> locker(m_mutex);
        changes = m_pendingChanges;
        m_pendingChanges.clear();
    }

    for (auto it = changes.constBegin(); it != changes.constEnd(); ++it) {
        emit modeChanged(it.key(), it.value());

        if (it.key() == SYNCHRONIZED_OUTPUT) {
            emit synchronizedOutputChanged(it.value());
        }
    }
}
