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

#ifndef SYNCHRONIZEDBUFFER_H
#define SYNCHRONIZEDBUFFER_H

#pragma once

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QDateTime>
#include "Utility/Spin.h"

/**
 * @brief 同步输出缓冲区
 *
 * 当 CSI 2026h（同步输出模式）启用时，
 * 所有渲染数据被缓存到缓冲区中，直到收到同步结束标记才一次性渲染。
 *
 * 工作原理:
 * 1. 收到 CSI 2026h -> 启用同步模式
 * 2. 所有后续数据写入缓冲区
 * 3. 收到 CSI 2026l 或同步结束标记 -> 禁用同步模式并刷新
 */
class SynchronizedBuffer : public QObject
{
    Q_OBJECT

public:
    explicit SynchronizedBuffer(QObject *parent = nullptr);
    ~SynchronizedBuffer();

    /**
     * @brief 缓冲区项类型
     */
    enum BufferItemType {
        TEXT_DATA,          // 普通文本数据
        CURSOR_MOVE,        // 光标移动
        CLEAR_SCREEN,       // 清屏操作
        SCROLL_REGION,      // 滚动区域设置
        ATTRIBUTE_CHANGE,   // 属性变化（颜色、样式等）
        RESIZE,             // 窗口大小变化
    };

    /**
     * @brief 缓冲区项
     */
    struct BufferItem {
        BufferItemType type;
        QByteArray data;
        int param1;  // 通用参数1
        int param2;  // 通用参数2
        quint64 timestamp;

        BufferItem() : type(TEXT_DATA), param1(0), param2(0), timestamp(0) {}
        BufferItem(BufferItemType t, const QByteArray &d, int p1 = 0, int p2 = 0)
            : type(t), data(d), param1(p1), param2(p2), timestamp(0) {}
    };

    /**
     * @brief 设置同步模式状态
     * @param enabled true 启用同步模式，false 禁用
     * @return 如果状态变化返回 true
     */
    bool setSynchronizedMode(bool enabled);

    /**
     * @brief 查询同步模式状态
     */
    bool isSynchronized() const;

    /**
     * @brief 写入数据到缓冲区
     * @param item 缓冲区项
     * @return 如果成功返回 true
     *
     * 如果同步模式启用，数据被缓存；否则直接传递
     */
    bool write(const BufferItem &item);

    /**
     * @brief 写入原始字节数据
     * @param data 原始数据
     * @return 如果成功返回 true
     */
    bool writeData(const QByteArray &data);

    /**
     * @brief 刷新缓冲区内容
     *
     * 将所有缓存的数据一次性发送给渲染器
     */
    void flush();

    /**
     * @brief 清空缓冲区（丢弃所有缓存数据）
     */
    void clear();

    /**
     * @brief 获取当前缓冲区大小
     */
    size_t bufferSize() const;

    /**
     * @brief 获取缓冲区项数量
     */
    int itemCount() const;

    /**
     * @brief 设置最大缓冲区大小（字节）
     */
    void setMaxBufferSize(size_t maxSize);

    /**
     * @brief 获取最大缓冲区大小
     */
    size_t maxBufferSize() const { return m_maxBufferSize; }

    /**
     * @brief 检查是否需要刷新
     * @return 如果缓冲区已满或超时返回 true
     */
    bool needsFlush() const;

Q_SIGNALS:
    /**
     * @brief 数据准备好可以渲染
     * @param items 缓冲区项列表
     */
    void dataReady(const QList<BufferItem> &items);

    /**
     * @brief 同步模式状态变化
     * @param enabled 新的状态
     */
    void synchronizedModeChanged(bool enabled);

    /**
     * @brief 缓冲区溢出警告
     */
    void bufferOverflow();

private:
    mutable SpinMutex m_mutex;

    // 同步模式状态
    bool m_synchronizedMode;

    // 缓冲区项列表
    QList<BufferItem> m_buffer;

    // 当前缓冲区总大小（字节）
    size_t m_currentSize;

    // 最大缓冲区大小
    size_t m_maxBufferSize;

    // 同步开始时间戳
    quint64 m_syncStartTime;

    // 超时时间（毫秒）
    static constexpr quint64 SYNC_TIMEOUT_MS = 100;

    bool appendItem(const BufferItem &item);
};

#endif // SYNCHRONIZEDBUFFER_H
