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

#include "SynchronizedBuffer.h"

#include <QElapsedTimer>

SynchronizedBuffer::SynchronizedBuffer(QObject *parent)
    : QObject(parent)
    , m_synchronizedMode(false)
    , m_currentSize(0)
    , m_maxBufferSize(1024 * 1024)  // 默认 1MB
    , m_syncStartTime(0)
{
}

SynchronizedBuffer::~SynchronizedBuffer() = default;

bool SynchronizedBuffer::setSynchronizedMode(bool enabled)
{
    bool changed = false;
    bool needFlush = false;

    {
        ThreadLocker<SpinMutex> locker(m_mutex);

        if (m_synchronizedMode == enabled) {
            return false;
        }

        m_synchronizedMode = enabled;
        changed = true;

        if (enabled) {
            // 启用同步模式，记录开始时间
            m_syncStartTime = QDateTime::currentMSecsSinceEpoch();
        } else {
            // 禁用同步模式，需要刷新缓冲区
            m_syncStartTime = 0;
            needFlush = true;
        }
    }  // 锁在这里自动释放

    if (changed) {
        emit synchronizedModeChanged(enabled);
    }

    // 如果禁用同步模式，立即刷新 (在锁外调用)
    if (needFlush) {
        flush();
    }

    return changed;
}

bool SynchronizedBuffer::isSynchronized() const
{
    ThreadLocker<SpinMutex> locker(m_mutex);
    return m_synchronizedMode;
}

bool SynchronizedBuffer::write(const BufferItem &item)
{
    {
        ThreadLocker<SpinMutex> locker(m_mutex);

        if (!m_synchronizedMode) {
            // 非同步模式，直接发送 (复制数据后解锁)
            emit dataReady(QList<BufferItem>() << item);
            return true;
        }

        // 同步模式，添加到缓冲区
        if (!appendItem(item)) {
            return false;
        }

        // 检查是否需要自动刷新
        if (needsFlush()) {
            // 需要在锁外调用 flush
        } else {
            return true;
        }
    }  // 锁在这里释放

    // 在锁外调用 flush
    flush();
    return true;
}

bool SynchronizedBuffer::writeData(const QByteArray &data)
{
    BufferItem item(TEXT_DATA, data);
    return write(item);
}

void SynchronizedBuffer::flush()
{
    QList<BufferItem> items;

    {
        ThreadLocker<SpinMutex> locker(m_mutex);

        if (m_buffer.isEmpty()) {
            return;
        }

        items = m_buffer;
        m_buffer.clear();
        m_currentSize = 0;
    }

    if (!items.isEmpty()) {
        emit dataReady(items);
    }
}

void SynchronizedBuffer::clear()
{
    ThreadLocker<SpinMutex> locker(m_mutex);
    m_buffer.clear();
    m_currentSize = 0;
}

size_t SynchronizedBuffer::bufferSize() const
{
    ThreadLocker<SpinMutex> locker(m_mutex);
    return m_currentSize;
}

int SynchronizedBuffer::itemCount() const
{
    ThreadLocker<SpinMutex> locker(m_mutex);
    return m_buffer.size();
}

void SynchronizedBuffer::setMaxBufferSize(size_t maxSize)
{
    ThreadLocker<SpinMutex> locker(m_mutex);
    m_maxBufferSize = maxSize;
}

bool SynchronizedBuffer::needsFlush() const
{
    // 检查缓冲区是否已满
    if (m_currentSize >= m_maxBufferSize) {
        return true;
    }

    // 检查是否超时
    if (m_syncStartTime > 0) {
        quint64 elapsed = QDateTime::currentMSecsSinceEpoch() - m_syncStartTime;
        if (elapsed >= SYNC_TIMEOUT_MS) {
            return true;
        }
    }

    return false;
}

bool SynchronizedBuffer::appendItem(const BufferItem &item)
{
    // 检查缓冲区是否已满
    if (m_currentSize >= m_maxBufferSize) {
        emit bufferOverflow();
        return false;
    }

    // 估算项大小
    size_t itemSize = item.data.size() + sizeof(BufferItem);

    // 如果单个项超过最大缓冲区，截断处理
    if (itemSize > m_maxBufferSize) {
        BufferItem truncatedItem = item;
        truncatedItem.data = item.data.left(m_maxBufferSize / 2);
        itemSize = truncatedItem.data.size() + sizeof(BufferItem);
        m_buffer.append(truncatedItem);
    } else {
        m_buffer.append(item);
    }

    m_currentSize += itemSize;
    return true;
}
