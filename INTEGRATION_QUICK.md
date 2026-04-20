# WindTerm CSI 2026 快速集成指南

## 概述

本指南说明如何将 CSI 2026 同步输出支持集成到 WindTerm 项目中。

## 文件结构

```
src/Terminal/              # 新增目录
├── TerminalMode.h/cpp     # 模式管理
├── SynchronizedBuffer.h/cpp  # 同步缓冲
├── CSIParser.h/cpp        # CSI 解析器
├── TerminalHandler.h/cpp  # 整合处理器
├── terminal.pri          # Qt 项目文件
├── README.md             # 模块文档
└── INTEGRATION.md        # 详细集成文档

src/Pty/                   # 修改目录
├── PtyTerminalBridge.h    # 新增桥接类
└── PtyTerminalBridge.cpp  # 新增桥接实现
```

## 快速集成步骤

### 步骤 1: 添加模块到项目

**方法 A: 使用 .pri 文件（推荐）**

在 `WindTerm.pro` 中添加：

```pro
# 添加 Terminal 模块
include(src/Terminal/terminal.pri)

# 添加桥接文件
SOURCES += src/Pty/PtyTerminalBridge.cpp
HEADERS += src/Pty/PtyTerminalBridge.h
```

**方法 B: 使用 CMake**

在 `CMakeLists.txt` 中添加：

```cmake
# 添加 Terminal 子目录
add_subdirectory(src/Terminal)

# 链接到主项目
target_link_libraries(WindTerm PRIVATE Terminal)

# 添加桥接文件
target_sources(WindTerm PRIVATE
    src/Pty/PtyTerminalBridge.cpp
    src/Pty/PtyTerminalBridge.h
)
```

### 步骤 2: 修改 Pty 类

在 `src/Pty/Pty.h` 中添加：

```cpp
#include "Terminal/TerminalHandler.h"

class Pty : public QObject {
    // ... 现有代码 ...

protected:
    TerminalHandler m_terminalHandler;  // 添加这行

private slots:
    void onTerminalDataReady(const QByteArray &data,
                             SynchronizedBuffer::BufferItemType type);
    void onTerminalCursorPositionChanged(int row, int column);
    void onTerminalClearScreen(int mode);
    void onTerminalAttributeChanged(const QList<int> &params);
};
```

在 `src/Pty/Pty.cpp` 的构造函数中添加：

```cpp
Pty::Pty() {
    // ... 现有初始化代码 ...

    // 连接 TerminalHandler 信号
    connect(&m_terminalHandler, &TerminalHandler::renderData,
            this, &Pty::onTerminalDataReady);

    connect(&m_terminalHandler, &TerminalHandler::cursorPositionChanged,
            this, &Pty::onTerminalCursorPositionChanged);

    connect(&m_terminalHandler, &TerminalHandler::clearScreenRequested,
            this, &Pty::onTerminalClearScreen);

    connect(&m_terminalHandler, &TerminalHandler::attributeChanged,
            this, &Pty::onTerminalAttributeChanged);
}
```

### 步骤 3: 修改数据读取方法

找到 Pty 中读取数据的函数（如 `readAll()`），修改为：

```cpp
QByteArray Pty::readAll() {
    // 读取原始数据
    QByteArray rawData = // ... 从底层读取 ...

    // 通过 TerminalHandler 处理（解析 CSI 序列）
    m_terminalHandler.processInput(rawData);

    // 返回处理后的数据或空
    // 注意：实际渲染现在通过信号完成
    return QByteArray();  // 或返回未处理的数据
}
```

### 步骤 4: 实现槽函数

在 `Pty.cpp` 中添加槽函数实现：

```cpp
void Pty::onTerminalDataReady(const QByteArray &data,
                               SynchronizedBuffer::BufferItemType type) {
    switch (type) {
    case SynchronizedBuffer::TEXT_DATA:
        // 发送文本到渲染引擎
        m_renderEngine->addText(data);
        break;

    case SynchronizedBuffer::CLEAR_SCREEN:
        m_renderEngine->clear();
        break;

    case SynchronizedBuffer::SCROLL_REGION:
        // 处理滚动区域
        break;

    // ... 其他类型 ...
    }
}

void Pty::onTerminalCursorPositionChanged(int row, int column) {
    m_renderEngine->setCursorPosition(row, column);
}

void Pty::onTerminalClearScreen(int mode) {
    switch (mode) {
    case 0: m_renderEngine->clearToEnd(); break;
    case 1: m_renderEngine->clearToStart(); break;
    case 2: m_renderEngine->clear(); break;
    case 3: m_renderEngine->clearWithScrollback(); break;
    }
}

void Pty::onTerminalAttributeChanged(const QList<int> &params) {
    // 处理 SGR 属性
    for (int param : params) {
        switch (param) {
        case 0: m_renderEngine->resetAttributes(); break;
        case 1: m_renderEngine->setBold(true); break;
        case 4: m_renderEngine->setUnderline(true); break;
        // ... 其他属性 ...
        }
    }
}
```

### 步骤 5: 处理窗口大小变化

在窗口调整大小时强制刷新：

```cpp
void Pty::resizeWindow(qint16 rows, qint16 columns) {
    // ... 现有代码 ...

    // 强制刷新同步缓冲区
    m_terminalHandler.forceFlush();
}
```

## 测试验证

### 1. 编译测试

```bash
# 清理并重新构建
qmake WindTerm.pro
make clean
make

# 或 CMake
mkdir build && cd build
cmake ..
make
```

### 2. 功能测试

启动 WindTerm 并运行以下测试命令：

```bash
# 测试 1: 基本同步输出
printf '\033[?2026hHello\nWorld\033[?2026l\n'

# 测试 2: 复杂屏幕更新
printf '\033[?2026h\033[2J\033[H=== Menu ===\n1. Item\n2. Item\033[?2026l\n'

# 测试 3: 属性变化
printf '\033[?2026h\033[1mBold\033[0m Normal\033[?2026l\n'

# 测试 4: 光标移动
printf '\033[?2026h\033[10;20HPosition\033[HHome\033[?2026l\n'
```

### 3. 真实应用测试

测试以下应用：

| 应用 | 测试场景 |
|------|----------|
| Vim | 打开大文件，快速滚动 |
| Neovim | 启用 termguicolors |
| tmux | 分割窗格，调整大小 |
| htop | 排序列，滚动进程列表 |
| ncdu | 浏览目录树 |
| fzf | 使用 fzf 进行模糊查找 |

## 故障排除

### 问题 1: 编译错误 "找不到头文件"

**解决**: 确保 INCLUDEPATH 包含 src 目录：

```pro
INCLUDEPATH += $$PWD/src
```

### 问题 2: 链接错误 "未定义的符号"

**解决**: 确保所有 .cpp 文件都在 SOURCES 中：

```pro
SOURCES += \
    src/Terminal/TerminalMode.cpp \
    src/Terminal/SynchronizedBuffer.cpp \
    src/Terminal/CSIParser.cpp \
    src/Terminal/TerminalHandler.cpp
```

### 问题 3: 数据没有显示

**检查**: 确保正确连接了信号：

```cpp
connect(&m_terminalHandler, &TerminalHandler::renderData,
        this, &Pty::onTerminalDataReady);
```

### 问题 4: 同步模式不生效

**调试**: 启用调试输出：

```cpp
m_terminalHandler.setDebugEnabled(true);
```

## 性能优化

### 1. 调整缓冲区大小

```cpp
// 在初始化时设置
m_terminalHandler.syncBuffer()->setMaxBufferSize(5 * 1024 * 1024);  // 5MB
```

### 2. 超时调整

编辑 `SynchronizedBuffer.h`：

```cpp
static constexpr quint64 SYNC_TIMEOUT_MS = 50;  // 从 100ms 减少到 50ms
```

### 3. 批量渲染

在渲染引擎中批量处理：

```cpp
void RenderEngine::onBatchDataReady(const QList<BufferItem> &items) {
    // 一次性处理所有项目
    beginUpdate();
    for (const auto &item : items) {
        processItem(item);
    }
    endUpdate();  // 统一刷新
}
```

## 完整示例

参考 `src/Pty/PtyTerminalBridge.h` 和 `.cpp` 获取完整实现示例。

参考 `src/Terminal/example_csi2026.cpp` 获取使用示例。

## 参考文档

- `src/Terminal/README.md` - 模块概述
- `src/Terminal/INTEGRATION.md` - 详细集成文档
- `src/Terminal/test_csi2026.cpp` - 单元测试

## 获取帮助

如果遇到问题：

1. 检查 `INTEGRATION.md` 中的详细说明
2. 运行单元测试: `test_csi2026`
3. 启用调试模式: `setDebugEnabled(true)`
4. 查看控制台输出
