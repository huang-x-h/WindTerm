# WindTerm CSI 2026 编译和打包指南

## 环境准备

### Windows

#### 方案 1: MinGW + Qt (推荐)

1. **安装 Qt (包含 MinGW)**
   - 下载 Qt Online Installer: https://www.qt.io/download-qt-installer
   - 选择组件:
     - Qt 5.15.2 / Qt 6.x
     - MinGW 8.1.0 64-bit
     - Qt Creator (可选)

2. **设置环境变量**
   ```cmd
   set PATH=C:\Qt\5.15.2\mingw81_64\bin;C:\Qt\Tools\mingw810_64\bin;%PATH%
   ```

#### 方案 2: Visual Studio + Qt

1. **安装 Visual Studio 2019/2022**
   - 选择 "Desktop development with C++"

2. **安装 Qt for MSVC**
   - 下载 Qt Online Installer
   - 选择 Qt 5.15.2 MSVC2019 64-bit

### Linux (Ubuntu/Debian)

```bash
# 安装 Qt 开发包
sudo apt update
sudo apt install -y \
    build-essential \
    qtbase5-dev \
    qtbase5-dev-tools \
    cmake \
    git

# 可选: Qt6
sudo apt install -y qt6-base-dev qt6-base-dev-tools
```

### macOS

```bash
# 安装 Homebrew (如果还没有)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 安装 Qt
brew install qt@5
# 或 Qt6
brew install qt@6

# 安装 CMake
brew install cmake
```

## 编译步骤

### 方法 1: 使用 CMake (推荐)

#### 1. 创建项目 CMakeLists.txt

在 WindTerm 根目录创建或修改 `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.16)
project(WindTermCSI2026 VERSION 1.0.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 自动处理 Qt 的 MOC, UIC, RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找 Qt
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

# 输出 Qt 信息
message(STATUS "Qt Version: ${QT_VERSION_MAJOR}")
message(STATUS "Qt Path: ${Qt${QT_VERSION_MAJOR}_DIR}")

# 添加 Terminal 模块
add_subdirectory(src/Terminal)

# 创建可执行文件
add_executable(WindTermCSI2026Demo
    src/Terminal/example_csi2026.cpp
)

target_link_libraries(WindTermCSI2026Demo PRIVATE
    Terminal
    Qt${QT_VERSION_MAJOR}::Core
)

# 创建测试可执行文件
add_executable(WindTermCSI2026Test
    src/Terminal/test_csi2026.cpp
)

target_link_libraries(WindTermCSI2026Test PRIVATE
    Terminal
    Qt${QT_VERSION_MAJOR}::Core
)

# 设置输出目录
set_target_properties(WindTermCSI2026Demo WindTermCSI2026Test
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Windows 特定配置
if(WIN32)
    set_property(TARGET WindTermCSI2026Demo PROPERTY WIN32_EXECUTABLE FALSE)
    set_property(TARGET WindTermCSI2026Test PROPERTY WIN32_EXECUTABLE FALSE)
    
    # 添加 Windows 控制台支持
    target_compile_definitions(WindTermCSI2026Demo PRIVATE CONSOLE)
    target_compile_definitions(WindTermCSI2026Test PRIVATE CONSOLE)
endif()
```

#### 2. 创建构建脚本

**Windows (build.bat):**

```batch
@echo off
chcp 65001 >nul
echo ==========================================
echo WindTerm CSI 2026 构建脚本
echo ==========================================

:: 检查 Qt 环境
if "%QTDIR%"=="" (
    echo [错误] QTDIR 环境变量未设置
    echo 请设置 QTDIR,例如: C:\Qt\5.15.2\mingw81_64
    exit /b 1
)

:: 添加到 PATH
set PATH=%QTDIR%\bin;%PATH%

:: 检查编译器
where g++ >nul 2>&1
if errorlevel 1 (
    echo [错误] 找不到 g++ 编译器
    echo 请确保 MinGW 在 PATH 中
    exit /b 1
)

echo [信息] 使用 Qt: %QTDIR%
echo [信息] 编译器: 
g++ --version | head -1

:: 创建构建目录
if not exist build mkdir build
cd build

:: 运行 CMake
echo.
echo [1/3] 配置 CMake...
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    echo [错误] CMake 配置失败
    exit /b 1
)

:: 编译
echo.
echo [2/3] 编译...
mingw32-make -j%NUMBER_OF_PROCESSORS%
if errorlevel 1 (
    echo [错误] 编译失败
    exit /b 1
)

:: 复制依赖
echo.
echo [3/3] 复制 Qt 依赖...
set DEPLOY_DIR=..\deploy\WindTermCSI2026
if not exist %DEPLOY_DIR% mkdir %DEPLOY_DIR%

copy bin\WindTermCSI2026Demo.exe %DEPLOY_DIR%\
copy bin\WindTermCSI2026Test.exe %DEPLOY_DIR%\

:: 使用 windeployqt
windeployqt %DEPLOY_DIR%\WindTermCSI2026Demo.exe --release

echo.
echo ==========================================
echo 构建完成!
echo 输出目录: %DEPLOY_DIR%
echo ==========================================
cd ..
```

**Linux/macOS (build.sh):**

```bash
#!/bin/bash

echo "=========================================="
echo "WindTerm CSI 2026 构建脚本"
echo "=========================================="

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 检查 Qt
if ! command -v qmake &> /dev/null; then
    echo -e "${RED}[错误] 找不到 qmake${NC}"
    echo "请安装 Qt 开发包"
    exit 1
fi

# 检查 CMake
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}[错误] 找不到 cmake${NC}"
    echo "请安装 CMake"
    exit 1
fi

# 显示版本
echo "[信息] Qt 版本: $(qmake -query QT_VERSION)"
echo "[信息] CMake 版本: $(cmake --version | head -1)"

# 获取 CPU 核心数
if [ "$OSTYPE" == "linux-gnu"* ]; then
    CORES=$(nproc)
elif [ "$OSTYPE" == "darwin"* ]; then
    CORES=$(sysctl -n hw.ncpu)
else
    CORES=4
fi

# 创建构建目录
mkdir -p build
cd build

# 配置
echo ""
echo "[1/3] 配置 CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release
if [ $? -ne 0 ]; then
    echo -e "${RED}[错误] CMake 配置失败${NC}"
    exit 1
fi

# 编译
echo ""
echo "[2/3] 编译 (使用 $CORES 核心)..."
make -j$CORES
if [ $? -ne 0 ]; then
    echo -e "${RED}[错误] 编译失败${NC}"
    exit 1
fi

# 准备部署目录
echo ""
echo "[3/3] 准备部署包..."
DEPLOY_DIR="../deploy/WindTermCSI2026"
mkdir -p $DEPLOY_DIR

# 复制可执行文件
cp bin/WindTermCSI2026Demo $DEPLOY_DIR/ 2>/dev/null || cp bin/WindTermCSI2026Demo.exe $DEPLOY_DIR/
cp bin/WindTermCSI2026Test $DEPLOY_DIR/ 2>/dev/null || cp bin/WindTermCSI2026Test.exe $DEPLOY_DIR/

# macOS 部署
if [ "$OSTYPE" == "darwin"* ]; then
    macdeployqt $DEPLOY_DIR/WindTermCSI2026Demo.app
fi

echo ""
echo -e "${GREEN}=========================================="
echo "构建完成!"
echo "输出目录: $DEPLOY_DIR"
echo "==========================================${NC}"

cd ..
```

#### 3. 执行构建

**Windows:**

```cmd
:: 设置 Qt 路径 (根据实际安装修改)
set QTDIR=C:\Qt\5.15.2\mingw81_64

:: 运行构建脚本
build.bat
```

**Linux:**

```bash
chmod +x build.sh
./build.sh
```

**macOS:**

```bash
chmod +x build.sh
./build.sh
```

### 方法 2: 使用 Qt 工具链 (qmake)

#### 1. 创建项目文件

创建 `WindTermCSI2026.pro`:

```pro
TEMPLATE = app
TARGET = WindTermCSI2026Demo
CONFIG += console c++17
CONFIG -= app_bundle
QT += core
QT -= gui

# 版本信息
VERSION = 1.0.0
DEFINES += APP_VERSION=\"$$VERSION\"

# 添加 Terminal 模块
include(src/Terminal/terminal.pri)

SOURCES += \
    src/Terminal/example_csi2026.cpp

# 测试目标
test_target.target = test
test_target.commands = $(CXX) -o WindTermCSI2026Test src/Terminal/test_csi2026.cpp $$QMAKE_CXXFLAGS $$QMAKE_LFLAGS
QMAKE_EXTRA_TARGETS += test_target

# Windows 特定
win32 {
    CONFIG += release
    CONFIG -= debug
    
    # 控制台输出
    CONFIG += console
    
    # 静态链接 (可选)
    # CONFIG += static
    
    # 部署规则
deploy.depends = release
deploy.commands = windeployqt $$TARGET --release --dir deploy
QMAKE_EXTRA_TARGETS += deploy
}

# Linux 特定
linux {
    # 静态链接 Qt (可选)
    # QMAKE_LFLAGS += -static-libgcc -static-libstdc++
}

# macOS 特定
macx {
    # 应用 bundle
    CONFIG += app_bundle
    
    # 部署
    deploy.depends = release
deploy.commands = macdeployqt $$TARGET.app
QMAKE_EXTRA_TARGETS += deploy
}

# 安装规则
INSTALLS += target
target.path = $$PWD/deploy
```

#### 2. 构建命令

```bash
# 生成 Makefile
qmake WindTermCSI2026.pro

# 编译
make -j$(nproc)  # Linux
mingw32-make -j%NUMBER_OF_PROCESSORS%  # Windows
make -j$(sysctl -n hw.ncpu)  # macOS

# 部署 (Windows)
windeployqt WindTermCSI2026Demo.exe --release --dir deploy

# 部署 (macOS)
macdeployqt WindTermCSI2026Demo.app
```

## 打包发布

### Windows 打包

创建 `package_windows.bat`:

```batch
@echo off
echo 打包 WindTerm CSI 2026...

set PACKAGE_NAME=WindTermCSI2026_v1.0.0_Windows
set BUILD_DIR=build\bin
set PACKAGE_DIR=packages\%PACKAGE_NAME%

:: 清理旧包
if exist %PACKAGE_DIR% rmdir /s /q %PACKAGE_DIR%
mkdir %PACKAGE_DIR%

:: 复制可执行文件
copy %BUILD_DIR%\WindTermCSI2026Demo.exe %PACKAGE_DIR%\
copy %BUILD_DIR%\WindTermCSI2026Test.exe %PACKAGE_DIR%\

:: 复制依赖 (使用 windeployqt)
windeployqt %PACKAGE_DIR%\WindTermCSI2026Demo.exe --release --no-translations

:: 复制文档
copy README.md %PACKAGE_DIR%\
copy INTEGRATION_QUICK.md %PACKAGE_DIR%\

:: 创建启动脚本
echo @echo off > %PACKAGE_DIR%\运行示例.bat
echo WindTermCSI2026Demo.exe >> %PACKAGE_DIR%\运行示例.bat
echo pause >> %PACKAGE_DIR%\运行示例.bat

echo @echo off > %PACKAGE_DIR%\运行测试.bat
echo WindTermCSI2026Test.exe >> %PACKAGE_DIR%\运行测试.bat
echo pause >> %PACKAGE_DIR%\运行测试.bat

:: 压缩
cd packages
if exist %PACKAGE_NAME%.zip del %PACKAGE_NAME%.zip
powershell -Command "Compress-Archive -Path %PACKAGE_NAME% -DestinationPath %PACKAGE_NAME%.zip"

echo 打包完成: packages/%PACKAGE_NAME%.zip
```

### Linux 打包

创建 `package_linux.sh`:

```bash
#!/bin/bash

PACKAGE_NAME="WindTermCSI2026_v1.0.0_Linux"
BUILD_DIR="build/bin"
PACKAGE_DIR="packages/$PACKAGE_NAME"

echo "打包 WindTerm CSI 2026..."

# 清理
rm -rf $PACKAGE_DIR
mkdir -p $PACKAGE_DIR

# 复制文件
cp $BUILD_DIR/WindTermCSI2026Demo $PACKAGE_DIR/
cp $BUILD_DIR/WindTermCSI2026Test $PACKAGE_DIR/
cp README.md $PACKAGE_DIR/
cp INTEGRATION_QUICK.md $PACKAGE_DIR/

# 复制 Qt 依赖库 (如果不是静态链接)
ldd $BUILD_DIR/WindTermCSI2026Demo | grep "Qt" | awk '{print $3}' | xargs -I {} cp {} $PACKAGE_DIR/lib/ 2>/dev/null || true

# 创建启动脚本
cat > $PACKAGE_DIR/run_demo.sh << 'EOF'
#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export LD_LIBRARY_PATH=$DIR/lib:$LD_LIBRARY_PATH
$DIR/WindTermCSI2026Demo "$@"
EOF
chmod +x $PACKAGE_DIR/run_demo.sh

# 创建 deb 包 (可选)
mkdir -p $PACKAGE_DIR/DEBIAN
cat > $PACKAGE_DIR/DEBIAN/control << EOF
Package: windterm-csi2026
Version: 1.0.0
Section: utils
Priority: optional
Architecture: amd64
Depends: libqt5core5a
Maintainer: Your Name <your.email@example.com>
Description: WindTerm CSI 2026 Terminal Module
 Implementation of CSI 2026 synchronized output for terminals.
EOF

# 压缩
cd packages
tar -czf $PACKAGE_NAME.tar.gz $PACKAGE_NAME

echo "打包完成: packages/$PACKAGE_NAME.tar.gz"
```

### macOS 打包

```bash
#!/bin/bash

PACKAGE_NAME="WindTermCSI2026_v1.0.0_macOS"
APP_NAME="WindTermCSI2026Demo.app"
PACKAGE_DIR="packages/$PACKAGE_NAME"

echo "打包 WindTerm CSI 2026..."

# 使用 macdeployqt
macdeployqt build/bin/$APP_NAME

# 复制到包目录
mkdir -p $PACKAGE_DIR
cp -R build/bin/$APP_NAME $PACKAGE_DIR/
cp README.md $PACKAGE_DIR/

# 创建 dmg
hdiutil create -volname "WindTermCSI2026" -srcfolder $PACKAGE_DIR -ov -format UDZO packages/$PACKAGE_NAME.dmg

echo "打包完成: packages/$PACKAGE_NAME.dmg"
```

## Docker 构建 (跨平台)

创建 `Dockerfile.build`:

```dockerfile
FROM ubuntu:22.04

# 安装依赖
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    qtbase5-dev \
    qtbase5-dev-tools \
    pkg-config \
    git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

# 复制源码
COPY . /build/

# 构建
RUN mkdir -p build && cd build && \
    cmake .. && \
    make -j$(nproc)

# 输出
CMD ["cp", "-r", "build/bin", "/output/"]
```

构建命令:

```bash
# 构建镜像
docker build -f Dockerfile.build -t windterm-csi2026-builder .

# 运行并提取输出
mkdir -p output
docker run --rm -v $(pwd)/output:/output windterm-csi2026-builder

# 结果在 output/ 目录
```

## 常见问题

### 问题 1: 找不到 Qt

**解决:**
```bash
# 设置 Qt 路径
export Qt5_DIR=/path/to/Qt/5.15.2/gcc_64/lib/cmake/Qt5

# 或
export PATH=/path/to/Qt/5.15.2/gcc_64/bin:$PATH
```

### 问题 2: 缺少头文件

**解决:**
```bash
# Ubuntu/Debian
sudo apt install qtbase5-dev

# Fedora/RHEL
sudo dnf install qt5-qtbase-devel

# macOS
brew install qt@5
```

### 问题 3: 链接错误

**解决:**
```cmake
# 在 CMakeLists.txt 中添加
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
target_link_libraries(your_target PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
)
```

## 验证安装

运行测试:

```bash
# 运行单元测试
./WindTermCSI2026Test

# 运行功能演示
./WindTermCSI2026Demo

# 手动测试同步输出
printf '\033[?2026hHello\nWorld\033[?2026l\n'
```

## 发布清单

- [ ] Windows 安装包 (.zip/.exe)
- [ ] Linux 安装包 (.tar.gz/.deb/.rpm)
- [ ] macOS 安装包 (.dmg)
- [ ] 源代码包 (.tar.gz)
- [ ] 文档 (README, CHANGELOG)
- [ ] 版本标签 (git tag v1.0.0)
