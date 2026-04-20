# 快速开始 - 编译和运行

## 系统要求

| 系统 | 最低要求 |
|------|----------|
| Windows | Windows 10, Qt 5.15+ 或 Qt 6.x |
| Linux | Ubuntu 20.04+, Qt 5.15+ |
| macOS | macOS 10.14+, Qt 5.15+ |

## 一键编译

### Windows

1. **安装 Qt** (如果还没有)
   - 下载: https://www.qt.io/download-qt-installer
   - 安装 Qt 5.15.2 或 Qt 6.x + MinGW

2. **运行构建脚本**
   ```cmd
   build_windows.bat
   ```

3. **运行程序**
   ```cmd
   deploy\WindTermCSI2026\运行演示.bat
   ```

### Linux

1. **安装依赖**
   ```bash
   sudo apt update
   sudo apt install -y build-essential cmake qtbase5-dev
   ```

2. **运行构建脚本**
   ```bash
   chmod +x build.sh
   ./build.sh
   ```

3. **运行程序**
   ```bash
   ./deploy/WindTermCSI2026/run_demo.sh
   ```

### macOS

1. **安装依赖**
   ```bash
   brew install qt@5 cmake
   export PATH="/opt/homebrew/opt/qt@5/bin:$PATH"
   ```

2. **运行构建脚本**
   ```bash
   chmod +x build.sh
   ./build.sh
   ```

3. **运行程序**
   ```bash
   open deploy/WindTermCSI2026/WindTermCSI2026Demo.app
   ```

## 手动编译

### 使用 CMake

```bash
# 创建构建目录
mkdir build && cd build

# 配置
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译 (Linux/macOS)
make -j$(nproc)

# 编译 (Windows - MinGW)
mingw32-make -j%NUMBER_OF_PROCESSORS%

# 运行
./bin/WindTermCSI2026Demo
```

### 使用 Qt Creator

1. 打开 Qt Creator
2. 文件 -> 打开文件或项目
3. 选择 `CMakeLists.txt`
4. 配置项目 (选择 Qt 套件)
5. 点击构建按钮

## 测试

### 运行单元测试

```bash
# Windows
deploy\WindTermCSI2026\运行测试.bat

# Linux/macOS
./deploy/WindTermCSI2026/run_test.sh
```

### 手动测试 CSI 2026

```bash
# 测试同步输出模式
printf '\033[?2026hHello\nWorld\033[?2026l\n'

# 测试复杂屏幕更新
printf '\033[?2026h\033[2J\033[H=== Menu ===\n1. Item\n2. Item\033[?2026l\n'

# 测试属性变化
printf '\033[?2026h\033[1mBold\033[0m Normal\033[?2026l\n'
```

## 故障排除

### Windows: "找不到 Qt"

```cmd
set QTDIR=C:\Qt\5.15.2\mingw81_64
set PATH=%QTDIR%\bin;%PATH%
```

### Linux: "找不到 Qt"

```bash
export Qt5_DIR=/usr/lib/x86_64-linux-gnu/cmake/Qt5
```

### macOS: "找不到 Qt"

```bash
export Qt5_DIR=/opt/homebrew/opt/qt@5/lib/cmake/Qt5
export PATH="/opt/homebrew/opt/qt@5/bin:$PATH"
```

## 输出文件

编译完成后，输出文件在 `deploy/WindTermCSI2026/` 目录:

```
deploy/
├── WindTermCSI2026/
│   ├── WindTermCSI2026Demo    (或 .exe)
│   ├── WindTermCSI2026Test    (或 .exe)
│   ├── Qt5Core.dll            (Windows - Qt 依赖)
│   ├── libQt5Core.so.5        (Linux - Qt 依赖)
│   ├── README.md
│   └── 运行演示.bat           (Windows)
│   └── run_demo.sh            (Linux/macOS)
└── WindTermCSI2026_Windows.zip (或 .tar.gz)
```

## 下一步

- 查看 `BUILD_GUIDE.md` 了解详细的编译选项
- 查看 `INTEGRATION_QUICK.md` 了解如何集成到 WindTerm
- 查看 `src/Terminal/README.md` 了解模块设计
