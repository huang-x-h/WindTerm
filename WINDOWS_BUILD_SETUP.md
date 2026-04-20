# Windows 编译环境准备指南

## 一、安装 Qt (必需)

### 下载 Qt

1. **官方下载地址**: https://www.qt.io/download-qt-installer

2. **选择版本**:
   - Qt 5.15.2 (长期支持版，推荐)
   - Qt 6.x (最新版)

3. **安装组件** (必须选择):
   ```
   Qt 5.15.2
   ├── MinGW 8.1.0 64-bit        [必须] C++编译器
   └── Qt 5.15.2 源文件          [可选]

   Developer and Designer Tools
   ├── Qt Creator 10.x.x         [推荐] IDE
   ├── MinGW 8.1.0 64-bit        [必须] 工具链
   └── CMake 3.x.x               [推荐] 构建工具
   ```

### 安装路径建议

```
C:\Qt\5.15.2\              (Qt 库)
C:\Qt\Tools\mingw810_64\    (MinGW 编译器)
C:\Qt\Tools\CMake\bin\      (CMake)
```

## 二、配置环境变量

### 方法 1: 通过 Qt Creator (推荐新手)

1. 打开 Qt Creator
2. 工具 → 选项 → Kits
3. 确认已自动检测到 Qt 版本和编译器

### 方法 2: 手动配置环境变量

```cmd
# 打开系统属性 → 高级 → 环境变量
# 添加或修改以下系统变量:

QTDIR=C:\Qt\5.15.2\mingw81_64

PATH 添加:
C:\Qt\5.15.2\mingw81_64\bin
C:\Qt\Tools\mingw810_64\bin
C:\Qt\Tools\CMake\bin
```

### 验证安装

打开新的命令提示符 (CMD) 或 PowerShell:

```cmd
# 检查 Qt
qmake --version
# 输出: QMake version 3.1, Qt version 5.15.2

# 检查编译器
g++ --version
# 输出: g++ (x86_64-posix-seh-rev0, Built by MinGW-W64 project) 8.1.0

# 检查 CMake
cmake --version
# 输出: cmake version 3.26.4
```

## 三、编译步骤

### 方式 1: 使用一键脚本 (最简单)

```cmd
# 1. 打开命令提示符，进入项目目录
cd D:\codebase\github\WindTerm

# 2. 运行构建脚本
build_windows.bat

# 3. 等待编译完成，输出在 deploy\WindTermCSI2026\
```

### 方式 2: 使用 Qt Creator

1. **打开项目**:
   - 启动 Qt Creator
   - 文件 → 打开文件或项目
   - 选择 `CMakeLists.txt`

2. **配置项目**:
   - 选择 Qt 套件 (MinGW 64-bit)
   - 点击 "配置项目"

3. **构建**:
   - 点击左下角的 ▶️ (运行) 按钮
   - 或按 Ctrl+R

4. **查找输出**:
   - 构建目录: `build-...-Release/`
   - 可执行文件: `bin/WindTermCSI2026Demo.exe`

### 方式 3: 手动命令行编译

```cmd
# 1. 进入项目目录
cd D:\codebase\github\WindTerm

# 2. 创建构建目录
mkdir build
cd build

# 3. 配置 CMake
cmake .. -G "MinGW Makefiles" ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_PREFIX_PATH=C:\Qt\5.15.2\mingw81_64

# 4. 编译
mingw32-make -j4

# 5. 部署 (复制 Qt 依赖)
windeployqt bin\WindTermCSI2026Demo.exe --release
```

## 四、输出文件

编译成功后，生成以下文件:

```
deploy/
└── WindTermCSI2026/
    ├── WindTermCSI2026Demo.exe      # 演示程序
    ├── WindTermCSI2026Test.exe      # 测试程序
    ├── Qt5Core.dll                  # Qt 核心库
    ├── libgcc_s_seh-1.dll           # GCC 运行时
    ├── libstdc++-6.dll              # C++ 标准库
    ├── libwinpthread-1.dll          # POSIX 线程库
    ├── 运行演示.bat                  # 启动脚本
    ├── 运行测试.bat
    └── README.md
```

## 五、常见问题解决

### 问题 1: "找不到 qmake"

**解决**:
```cmd
set PATH=C:\Qt\5.15.2\mingw81_64\bin;%PATH%
```

### 问题 2: "找不到 g++"

**解决**:
```cmd
set PATH=C:\Qt\Tools\mingw810_64\bin;%PATH%
```

### 问题 3: "CMake 找不到 Qt"

**解决**:
```cmd
cmake .. -DCMAKE_PREFIX_PATH=C:\Qt\5.15.2\mingw81_64
```

### 问题 4: "无法找到 windeployqt"

**解决**:
```cmd
set PATH=C:\Qt\5.15.2\mingw81_64\bin;%PATH%
windeployqt your_app.exe
```

### 问题 5: 运行 exe 提示缺少 DLL

**解决**:
使用 `windeployqt` 复制依赖:
```cmd
windeployqt WindTermCSI2026Demo.exe --release
```

或手动复制以下 DLL 到 exe 同级目录:
- `Qt5Core.dll`
- `libgcc_s_seh-1.dll`
- `libstdc++-6.dll`
- `libwinpthread-1.dll`

## 六、最小安装 (仅命令行)

如果只需要命令行编译，不需要 Qt Creator:

1. 下载 Qt 时只选择:
   - MinGW 8.1.0 64-bit
   - Qt 5.15.2 的 MinGW 预构建库

2. 不需要安装:
   - Qt Creator
   - 其他 Qt 组件

3. 最小占用约 2GB 空间

## 七、替代方案

### 方案 A: 使用 Visual Studio

```
1. 安装 Visual Studio 2019/2022 (选择 C++ 桌面开发)
2. 安装 Qt for MSVC (Qt 安装器中选择 MSVC2019 64-bit)
3. 使用 Visual Studio 打开 CMakeLists.txt
4. 选择 x64-Release 配置，点击生成
```

### 方案 B: 使用 MSYS2

```bash
# 1. 安装 MSYS2: https://www.msys2.org/
# 2. 打开 MSYS2 MinGW 64-bit 终端

# 3. 安装工具
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-qt5

# 4. 编译
cd /d/codebase/github/WindTerm
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
mingw32-make
```

## 八、验证编译成功

运行测试:

```cmd
cd deploy\WindTermCSI2026

# 运行单元测试
WindTermCSI2026Test.exe

# 运行演示程序
WindTermCSI2026Demo.exe

# 手动测试 CSI 2026
printf '\033[?2026hHello\nWorld\033[?2026l\n'
```

## 九、打包分发

编译完成后，将以下文件打包即可分发:

```
WindTermCSI2026/
├── WindTermCSI2026Demo.exe
├── WindTermCSI2026Test.exe
├── Qt5Core.dll
├── libgcc_s_seh-1.dll
├── libstdc++-6.dll
├── libwinpthread-1.dll
└── 运行演示.bat
```

压缩为 zip 文件:
```cmd
cd deploy
powershell Compress-Archive -Path WindTermCSI2026 -DestinationPath WindTermCSI2026_Windows.zip
```
