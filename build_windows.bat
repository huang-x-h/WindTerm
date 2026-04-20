@echo off
chcp 65001 >nul
echo ==========================================
echo WindTerm CSI 2026 构建脚本 (Windows)
echo ==========================================
echo.

:: 检查管理员权限
net session >nul 2>&1
if %errorLevel% == 0 (
    echo [警告] 建议以普通用户身份运行
    echo.
)

:: 检查 Qt 环境
if "%QTDIR%"=="" (
    :: 尝试自动检测 Qt 路径
    for /d %%D in (C:\Qt\5.* C:\Qt\6.* D:\Qt\5.* D:\Qt\6.*) do (
        if exist "%%D\mingw*\bin\qmake.exe" (
            set QTDIR=%%D\mingw64
            goto :found_qt
        )
    )

    echo [错误] 找不到 Qt 安装路径
    echo 请设置 QTDIR 环境变量，例如:
    echo   set QTDIR=C:\Qt\5.15.2\mingw81_64
    echo.
    echo 或者将 Qt 的 bin 目录添加到 PATH
    pause
    exit /b 1
)

:found_qt
echo [信息] Qt 路径: %QTDIR%

:: 添加到 PATH
set PATH=%QTDIR%\bin;%QTDIR%\..\..\Tools\mingw810_64\bin;%PATH%

:: 检查编译器
where g++ >nul 2>&1
if errorlevel 1 (
    echo [错误] 找不到 g++ 编译器
    echo 请确保 MinGW 在 PATH 中
    pause
    exit /b 1
)

echo [信息] 编译器:
for /f "tokens=*" %%a in ('g++ --version ^| findstr /B "g++"') do echo   %%a

:: 检查 CMake
where cmake >nul 2>&1
if errorlevel 1 (
    echo [错误] 找不到 CMake
    echo 请从 https://cmake.org/download/ 下载安装
    pause
    exit /b 1
)

echo [信息] CMake:
cmake --version | findstr /B "cmake"

:: 创建构建目录
echo.
echo [1/4] 准备构建目录...
if not exist build mkdir build
cd build

:: 清理旧构建 (可选)
if exist CMakeCache.txt (
    echo 清理旧构建...
    del /q CMakeCache.txt 2>nul
    rmdir /s /q CMakeFiles 2>nul
)

:: 运行 CMake
echo.
echo [2/4] 配置项目...
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=%QTDIR%
if errorlevel 1 (
    echo.
    echo [错误] CMake 配置失败
    echo 可能的解决方案:
    echo 1. 检查 Qt 安装是否完整
    echo 2. 删除 build 目录后重试
    pause
    exit /b 1
)

:: 编译
echo.
echo [3/4] 编译项目 (这可能需要几分钟)...
mingw32-make -j%NUMBER_OF_PROCESSORS%
if errorlevel 1 (
    echo.
    echo [错误] 编译失败
    pause
    exit /b 1
)

:: 部署
echo.
echo [4/4] 准备部署包...
set DEPLOY_DIR=..\deploy\WindTermCSI2026
if not exist %DEPLOY_DIR% mkdir %DEPLOY_DIR%

:: 复制可执行文件
copy bin\WindTermCSI2026Demo.exe %DEPLOY_DIR% >nul
copy bin\WindTermCSI2026Test.exe %DEPLOY_DIR% >nul

:: 复制文档
copy ..\README.md %DEPLOY_DIR% >nul 2>&1
copy ..\INTEGRATION_QUICK.md %DEPLOY_DIR% >nul 2>&1
copy ..\BUILD_GUIDE.md %DEPLOY_DIR% >nul 2>&1

:: 使用 windeployqt 复制 Qt 依赖
echo   复制 Qt 依赖库...
windeployqt %DEPLOY_DIR%\WindTermCSI2026Demo.exe --release --no-translations >nul 2>&1

:: 创建启动脚本
echo   创建启动脚本...
(
echo @echo off
echo echo ==========================================
echo echo WindTerm CSI 2026 演示程序
echo echo ==========================================
echo echo.
echo WindTermCSI2026Demo.exe
echo.
echo pause
) > %DEPLOY_DIR%\运行演示.bat

(
echo @echo off
echo echo ==========================================
echo echo WindTerm CSI 2026 测试程序
echo echo ==========================================
echo echo.
echo WindTermCSI2026Test.exe
echo.
echo pause
) > %DEPLOY_DIR%\运行测试.bat

:: 压缩包
echo   创建压缩包...
cd %DEPLOY_DIR%\..
if exist WindTermCSI2026_Windows.zip del WindTermCSI2026_Windows.zip
powershell -Command "Compress-Archive -Path WindTermCSI2026 -DestinationPath WindTermCSI2026_Windows.zip" >nul 2>&1

echo.
echo ==========================================
echo 构建成功!
echo ==========================================
echo.
echo 输出文件:
echo   目录: %CD%\WindTermCSI2026\
echo   压缩: %CD%\WindTermCSI2026_Windows.zip
echo.
echo 运行方式:
echo   1. 进入 %DEPLOY_DIR%
echo   2. 双击 运行演示.bat 或 运行测试.bat
echo.
echo 功能测试:
echo   printf '\033[?2026hHello\nWorld\033[?2026l\n'
echo.

:: 询问是否运行测试
echo 是否立即运行测试? (Y/N)
set /p RUN_TEST=
if /i "%RUN_TEST%"=="Y" (
    echo.
    %DEPLOY_DIR%\WindTermCSI2026Test.exe
)

cd ..\..
pause
