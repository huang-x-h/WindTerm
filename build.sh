#!/bin/bash
#
# WindTerm CSI 2026 构建脚本 (Linux/macOS)
#

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo "=========================================="
echo "WindTerm CSI 2026 构建脚本"
echo "=========================================="
echo ""

# 检测操作系统
OS="unknown"
if [ "$OSTYPE" == "linux-gnu"* ] || [ "$OSTYPE" == "linux" ]; then
    OS="linux"
elif [ "$OSTYPE" == "darwin"* ]; then
    OS="macos"
else
    echo -e "${RED}[错误] 不支持的操作系统: $OSTYPE${NC}"
    exit 1
fi

echo "[信息] 操作系统: $OS"

# 检查 Qt
if command -v qmake &> /dev/null; then
    QT_VERSION=$(qmake -query QT_VERSION)
    echo "[信息] Qt 版本: $QT_VERSION"
elif command -v qmake6 &> /dev/null; then
    QT_VERSION=$(qmake6 -query QT_VERSION)
    echo "[信息] Qt 版本: $QT_VERSION"
else
    echo -e "${RED}[错误] 找不到 qmake${NC}"
    echo "请安装 Qt:"
    echo "  Ubuntu/Debian: sudo apt install qtbase5-dev"
    echo "  Fedora:        sudo dnf install qt5-qtbase-devel"
    echo "  macOS:         brew install qt@5"
    exit 1
fi

# 检查 CMake
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}[错误] 找不到 cmake${NC}"
    echo "请安装 CMake:"
    echo "  Ubuntu/Debian: sudo apt install cmake"
    echo "  Fedora:        sudo dnf install cmake"
    echo "  macOS:         brew install cmake"
    exit 1
fi

CMAKE_VERSION=$(cmake --version | head -1 | cut -d' ' -f3)
echo "[信息] CMake 版本: $CMAKE_VERSION"

# 检查编译器
if command -v g++ &> /dev/null; then
    CXX_COMPILER=g++
    CXX_VERSION=$(g++ --version | head -1)
    echo "[信息] 编译器: $CXX_VERSION"
elif command -v clang++ &> /dev/null; then
    CXX_COMPILER=clang++
    CXX_VERSION=$(clang++ --version | head -1)
    echo "[信息] 编译器: $CXX_VERSION"
else
    echo -e "${RED}[错误] 找不到 C++ 编译器${NC}"
    echo "请安装 build-essential 或 Xcode"
    exit 1
fi

# 获取 CPU 核心数
if [ "$OS" == "linux" ]; then
    CORES=$(nproc)
elif [ "$OS" == "macos" ]; then
    CORES=$(sysctl -n hw.ncpu)
else
    CORES=4
fi

echo "[信息] 使用 $CORES 核心并行编译"
echo ""

# 创建构建目录
echo "[1/4] 准备构建目录..."
mkdir -p build
cd build

# 清理旧构建
if [ -f "CMakeCache.txt" ]; then
    echo "  清理旧构建..."
    rm -f CMakeCache.txt
    rm -rf CMakeFiles
fi

# 运行 CMake
echo ""
echo "[2/4] 配置项目..."
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=$CXX_COMPILER
if [ $? -ne 0 ]; then
    echo -e "${RED}[错误] CMake 配置失败${NC}"
    echo "可能的解决方案:"
    echo "1. 检查 Qt 安装: qmake -query"
    echo "2. 手动指定 Qt 路径:"
    echo "   cmake .. -DQt5_DIR=/path/to/Qt/lib/cmake/Qt5"
    echo "3. 删除 build 目录后重试"
    exit 1
fi

# 编译
echo ""
echo "[3/4] 编译项目..."
make -j$CORES
if [ $? -ne 0 ]; then
    echo -e "${RED}[错误] 编译失败${NC}"
    exit 1
fi

# 部署
echo ""
echo "[4/4] 准备部署包..."
DEPLOY_DIR="../deploy/WindTermCSI2026"
mkdir -p $DEPLOY_DIR

# 复制可执行文件
echo "  复制可执行文件..."
cp bin/WindTermCSI2026Demo $DEPLOY_DIR/ 2>/dev/null || cp bin/WindTermCSI2026Demo.app/Contents/MacOS/WindTermCSI2026Demo $DEPLOY_DIR/ 2>/dev/null || true
cp bin/WindTermCSI2026Test $DEPLOY_DIR/ 2>/dev/null || cp bin/WindTermCSI2026Test.app/Contents/MacOS/WindTermCSI2026Test $DEPLOY_DIR/ 2>/dev/null || true

# 复制文档
echo "  复制文档..."
cp ../README.md $DEPLOY_DIR/ 2>/dev/null || true
cp ../INTEGRATION_QUICK.md $DEPLOY_DIR/ 2>/dev/null || true
cp ../BUILD_GUIDE.md $DEPLOY_DIR/ 2>/dev/null || true

# macOS 特定部署
if [ "$OS" == "macos" ]; then
    echo "  部署 macOS 应用..."
    if command -v macdeployqt &> /dev/null; then
        macdeployqt $DEPLOY_DIR/WindTermCSI2026Demo.app 2>/dev/null || true
        macdeployqt $DEPLOY_DIR/WindTermCSI2026Test.app 2>/dev/null || true
    fi
fi

# Linux 特定 - 复制依赖库
if [ "$OS" == "linux" ]; then
    echo "  复制依赖库..."
    mkdir -p $DEPLOY_DIR/lib
    ldd $DEPLOY_DIR/WindTermCSI2026Demo 2>/dev/null | grep "Qt" | awk '{print $3}' | while read lib; do
        if [ -f "$lib" ]; then
            cp "$lib" $DEPLOY_DIR/lib/ 2>/dev/null || true
        fi
    done

    # 创建启动脚本
    cat > $DEPLOY_DIR/run_demo.sh << 'EOF'
#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export LD_LIBRARY_PATH=$DIR/lib:$LD_LIBRARY_PATH
$DIR/WindTermCSI2026Demo "$@"
EOF
    chmod +x $DEPLOY_DIR/run_demo.sh

    cat > $DEPLOY_DIR/run_test.sh << 'EOF'
#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export LD_LIBRARY_PATH=$DIR/lib:$LD_LIBRARY_PATH
$DIR/WindTermCSI2026Test "$@"
EOF
    chmod +x $DEPLOY_DIR/run_test.sh
fi

# 压缩包
echo "  创建压缩包..."
cd ../deploy
if [ "$OS" == "linux" ]; then
    PACKAGE_NAME="WindTermCSI2026_Linux.tar.gz"
    tar -czf $PACKAGE_NAME WindTermCSI2026
elif [ "$OS" == "macos" ]; then
    PACKAGE_NAME="WindTermCSI2026_macOS.zip"
    zip -r $PACKAGE_NAME WindTermCSI2026
fi

cd ..

# 成功消息
echo ""
echo "=========================================="
echo -e "${GREEN}构建成功!${NC}"
echo "=========================================="
echo ""
echo "输出文件:"
echo -e "  ${BLUE}目录:${NC} $DEPLOY_DIR"
echo -e "  ${BLUE}压缩:${NC} deploy/$PACKAGE_NAME"
echo ""
echo "运行方式:"
if [ "$OS" == "linux" ]; then
    echo "  cd $DEPLOY_DIR"
    echo "  ./run_demo.sh    # 运行演示"
    echo "  ./run_test.sh    # 运行测试"
elif [ "$OS" == "macos" ]; then
    echo "  open $DEPLOY_DIR/WindTermCSI2026Demo.app"
fi
echo ""
echo "功能测试:"
echo "  printf '\\033[?2026hHello\\nWorld\\033[?2026l\\n'"
echo ""

# 询问是否运行测试
read -p "是否立即运行测试? (y/N) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo ""
    if [ "$OS" == "linux" ]; then
        $DEPLOY_DIR/run_test.sh
    elif [ "$OS" == "macos" ]; then
        $DEPLOY_DIR/WindTermCSI2026Test
    fi
fi

echo ""
