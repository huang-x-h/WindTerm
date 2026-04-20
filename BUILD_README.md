# WindTerm CSI 2026 终端模块

[![Build and Release](https://github.com/kingToolbox/WindTerm/actions/workflows/build.yml/badge.svg)](https://github.com/kingToolbox/WindTerm/actions/workflows/build.yml)
[![Test and Validation](https://github.com/kingToolbox/WindTerm/actions/workflows/test.yml/badge.svg)](https://github.com/kingToolbox/WindTerm/actions/workflows/test.yml)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![Qt Version](https://img.shields.io/badge/Qt-5.15%2B-green.svg)](https://www.qt.io/)

高性能终端 CSI 2026 同步输出模式实现，用于 WindTerm 终端模拟器。

## 功能特性

- ✨ **CSI 2026 同步输出** - 批量渲染避免屏幕闪烁
- 🚀 **高性能** - 线程安全，低内存占用
- 🔧 **完整 CSI 解析** - 支持光标移动、SGR 属性、清屏等
- 📱 **跨平台** - Windows/Linux/macOS 全支持
- 🔒 **稳定可靠** - 自动超时刷新，防止数据卡住

## 快速开始

### 下载预编译版本

从 [GitHub Releases](https://github.com/kingToolbox/WindTerm/releases) 下载对应平台的包：

| 平台 | 下载 | 运行 |
|------|------|------|
| Windows | `WindTermCSI2026_Windows.zip` | 解压后运行 `运行演示.bat` |
| Linux | `WindTermCSI2026_Linux.tar.gz` | `tar -xzf` 后运行 `./run.sh` |
| macOS | `WindTermCSI2026_macOS.dmg` | 打开后拖动到 Applications |

### 手动编译

**Windows:**
```cmd
git clone https://github.com/kingToolbox/WindTerm.git
cd WindTerm
build_windows.bat
```

**Linux/macOS:**
```bash
git clone https://github.com/kingToolbox/WindTerm.git
cd WindTerm
chmod +x build.sh
./build.sh
```

详细说明: [BUILD_GUIDE.md](BUILD_GUIDE.md)

## 功能测试

```bash
# 测试同步输出模式
printf '\033[?2026hHello\nWorld\033[?2026l\n'

# 复杂屏幕更新
printf '\033[?2026h\033[2J\033[H=== Menu ===\n1. Item\n2. Item\033[?2026l\n'

# 属性变化
printf '\033[?2026h\033[1mBold\033[0m Normal\033[?2026l\n'
```

## 项目结构

```
src/Terminal/              # 核心模块
├── TerminalMode.h/cpp     # 模式管理
├── SynchronizedBuffer.h/cpp  # 同步缓冲
├── CSIParser.h/cpp        # CSI 解析器
├── TerminalHandler.h/cpp  # 整合处理器
└── ...

src/Pty/                   # 集成示例
├── PtyTerminalBridge.h/cpp
└── PtyWithCSI2026.h/cpp

.github/workflows/         # CI/CD
├── build.yml             # 自动构建发布
└── test.yml              # 测试验证
```

## 集成到 WindTerm

查看 [INTEGRATION_QUICK.md](INTEGRATION_QUICK.md) 了解详细集成步骤。

简化的 3 步集成:

1. **添加模块到项目**
   ```pro
   include(src/Terminal/terminal.pri)
   ```

2. **修改 Pty 类**
   ```cpp
   #include "Terminal/TerminalHandler.h"
   TerminalHandler m_terminalHandler;
   ```

3. **处理数据**
   ```cpp
   m_terminalHandler.processInput(rawData);
   ```

## 自动构建 (GitHub Actions)

本项目使用 GitHub Actions 自动构建和发布：

- ✅ **自动编译** - 每次推送自动在 Windows/Linux/macOS 上编译
- ✅ **自动测试** - 运行 Python 模拟测试和代码检查
- ✅ **自动发布** - 推送标签自动创建 Release

查看构建状态: [Actions 页面](../../actions)

### 触发自动发布

```bash
# 创建版本标签
git tag v1.0.0
git push origin v1.0.0
```

GitHub Actions 会自动:
1. 在三个平台上编译
2. 运行所有测试
3. 创建 GitHub Release
4. 上传构建产物

## 文档

| 文档 | 说明 |
|------|------|
| [QUICKSTART.md](QUICKSTART.md) | 3步快速开始 |
| [BUILD_GUIDE.md](BUILD_GUIDE.md) | 详细编译指南 |
| [INTEGRATION_QUICK.md](INTEGRATION_QUICK.md) | 集成指南 |
| [WINDOWS_BUILD_SETUP.md](WINDOWS_BUILD_SETUP.md) | Windows 环境配置 |
| [src/Terminal/README.md](src/Terminal/README.md) | 模块设计文档 |
| [src/Terminal/INTEGRATION.md](src/Terminal/INTEGRATION.md) | 详细集成文档 |

## 系统要求

| 平台 | 最低要求 |
|------|----------|
| Windows | Windows 10, Qt 5.15+ |
| Linux | Ubuntu 20.04+, Qt 5.15+ |
| macOS | macOS 10.14+, Qt 5.15+ |

## 贡献

欢迎提交 Issue 和 Pull Request!

1. Fork 本仓库
2. 创建功能分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送分支 (`git push origin feature/AmazingFeature`)
5. 创建 Pull Request

## 许可证

Apache-2.0 License - 详见 [LICENSE](LICENSE) 文件

## 致谢

- [WindTerm](https://github.com/kingToolbox/WindTerm) - 优秀的终端模拟器
- [Qt](https://www.qt.io/) - 跨平台应用程序框架

---

**注意**: 本项目是 WindTerm 的部分开源实现，用于添加 CSI 2026 同步输出支持。
