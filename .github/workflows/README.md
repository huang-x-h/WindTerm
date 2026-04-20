# GitHub Actions CI/CD 配置

本目录包含 GitHub Actions 工作流配置，用于自动编译、测试和发布 WindTerm CSI 2026 模块。

## 工作流说明

### 1. Build and Release (`build.yml`)

**触发条件:**
- 推送到 `main` 或 `master` 分支
- 推送版本标签 (如 `v1.0.0`)
- 手动触发 (workflow_dispatch)

**功能:**
- ✅ Windows (MinGW) 构建
- ✅ Linux (Ubuntu 22.04) 构建
- ✅ macOS (最新版) 构建
- ✅ 自动打包和压缩
- ✅ 自动创建 GitHub Release
- ✅ 上传构建产物

### 2. Test and Validation (`test.yml`)

**触发条件:**
- 推送到 `main`, `master`, `develop` 分支
- Pull Request

**功能:**
- ✅ Python 功能模拟测试
- ✅ 静态代码分析
- ✅ 文件完整性检查
- ✅ 多平台构建测试 (矩阵)

## 使用方法

### 方式 1: 自动构建 (推送代码)

每次推送到主分支时，GitHub Actions 会自动:

1. 在 Windows/Linux/macOS 上编译代码
2. 运行测试
3. 打包输出
4. 上传构建产物

查看构建状态:
- 打开 GitHub 仓库
- 点击 "Actions" 标签
- 查看工作流运行状态

### 方式 2: 手动触发构建

1. 打开 GitHub 仓库
2. 点击 "Actions" 标签
3. 选择 "Build and Release" 工作流
4. 点击 "Run workflow"
5. 选择分支，点击 "Run workflow"

### 方式 3: 发布新版本

```bash
# 1. 提交所有更改
git add .
git commit -m "Release v1.0.0"

# 2. 创建标签
git tag v1.0.0

# 3. 推送标签到 GitHub
git push origin v1.0.0
```

推送标签后，GitHub Actions 会自动:
- 在三个平台上构建
- 创建 GitHub Release
- 上传所有构建产物

## 构建产物

每次构建完成后，可以在以下位置下载:

### 1. Actions Artifacts
- 打开 Actions 页面
- 点击具体的工作流运行
- 在 "Artifacts" 部分下载

### 2. GitHub Release (仅标签触发)
- 打开 Releases 页面
- 查看最新发布
- 下载对应平台的包

## 输出文件

| 平台 | 文件名 | 说明 |
|------|--------|------|
| Windows | `WindTermCSI2026_Windows.zip` | 包含 exe 和 Qt 依赖 |
| Linux | `WindTermCSI2026_Linux.tar.gz` | 包含可执行文件和库 |
| macOS | `WindTermCSI2026_macOS.dmg` | 应用安装包 |
| macOS | `WindTermCSI2026_macOS.zip` | 压缩包备选 |

## 配置说明

### 修改 Qt 版本

编辑 `.github/workflows/build.yml`:

```yaml
env:
  QT_VERSION: '6.5.0'  # 修改为你需要的版本
```

### 添加更多平台

编辑 `.github/workflows/build.yml`, 添加新的 job:

```yaml
build-alpine:
  runs-on: ubuntu-latest
  container: alpine:latest
  steps:
    # ... 安装依赖并构建
```

### 自定义构建参数

编辑 `CMakeLists.txt` 或工作流文件中的 cmake 命令:

```yaml
- name: Configure
  run: |
    cmake -B build -S . \
      -DCMAKE_BUILD_TYPE=Release \
      -DOPTION=ON
```

## 故障排除

### 问题 1: Qt 安装失败

**解决:**
- 检查 Qt 版本号是否正确
- 检查 arch 参数是否匹配 (win64_mingw81, clang_64 等)
- 查看 jurplel/install-qt-action 文档

### 问题 2: Windows 编译器找不到

**解决:**
- 确保使用 `shell: cmd` 或 `shell: powershell`
- 检查 MinGW 是否正确安装
- 使用 `egor-tensin/setup-mingw` 安装 MinGW

### 问题 3: macOS 签名问题

**解决:**
- 添加签名配置:
```yaml
- name: Sign app
  run: codesign --force --deep --sign - "WindTermCSI2026Demo.app"
```

### 问题 4: Release 创建失败

**解决:**
- 确保 `secrets.GITHUB_TOKEN` 有写权限
- 检查工作流权限设置:
```yaml
permissions:
  contents: write
```

## 本地测试 Actions

使用 [act](https://github.com/nektos/act) 工具本地测试:

```bash
# 安装 act
brew install act  # macOS
# 或
curl https://raw.githubusercontent.com/nektos/act/master/install.sh | sudo bash

# 运行工作流
act push

# 运行特定工作流
act -W .github/workflows/test.yml

# 运行特定 job
act -j build-linux
```

## 相关文档

- [GitHub Actions 文档](https://docs.github.com/en/actions)
- [install-qt-action](https://github.com/jurplel/install-qt-action)
- [action-gh-release](https://github.com/softprops/action-gh-release)
