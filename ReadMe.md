# DsComplete

DsComplete 是一个独立构建的 Qt Creator 插件，通过 DeepSeek Fill-in-the-Middle API 提供行内代码补全。

插件只实现代码补全，不包含聊天、Agent、仓库索引或跨文件检索功能。

## 功能

- 自动请求行内补全，可配置触发延迟。
- 手动请求当前光标位置的补全。
- 使用 Qt Creator 原生幽灵文本显示补全结果。
- 复用 Qt Creator 的完整、单词和单行接受动作。
- 支持兼容接口返回多个候选时循环切换。
- 使用系统凭据存储保存 API Key。
- 支持配置 API 地址、模型、上下文长度和最大输出 token。
- 光标或文档变化后取消请求并丢弃过期响应。
- 已有其他插件的 suggestion 时主动退让，不覆盖 Copilot 等插件的结果。

## 当前限制

- 默认使用非流式 FIM 请求。
- 不自动重试失败或限流请求。
- 不提供项目级开关。安装版 Qt Creator SDK 未包含 ProjectExplorer 所需的 QtTaskTree 开发头，因此当前仅提供全局开关。
- 插件需要用户先在 Qt Creator 的插件管理界面启用，然后再在设置中启用补全。

## 目录结构

| 文件 | 用途 |
| --- | --- |
| `dscompleteplugin.cpp` | 插件入口、动作和状态栏开关 |
| `dscompleteclient.cpp` | 编辑器监听、请求调度和补全展示 |
| `dscompleteprotocol.cpp` | FIM URL、payload、上下文和响应解析 |
| `dscompletesettings.cpp` | 全局设置和 API Key 凭据存储 |
| `tst_dscompleteprotocol.cpp` | 协议层单元测试 |
| `package.ps1` | 验证并生成插件 ZIP 和 SHA-256 校验文件 |
| `CMakeLists.txt` | 独立 CMake 构建入口 |
| `dscomplete.qbs` | 与 CMake 同步的 qbs 描述 |

## 前置条件

当前配置针对以下环境验证：

- Windows x64
- Qt Creator SDK：`C:\Qt\Tools\qtcreator`
- Qt：`C:\Qt\6.8.3\msvc2022_64`
- MSVC x64 工具链
- CMake 和 Ninja

安装版 Qt Creator 的 CMake package 位于：

```text
C:\Qt\Tools\qtcreator\lib\cmake\QtCreator
```

如安装位置不同，可通过 `QtCreator_DIR` 和 `CMAKE_PREFIX_PATH` 覆盖。

## 配置与构建

在 PowerShell 中载入 MSVC 环境：

```powershell
. "C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1" `
    -Arch amd64 -HostArch amd64
```

配置工程并启用测试：

```powershell
cmake -S . -B build-ninja-msvc -G Ninja `
    -DCMAKE_BUILD_TYPE=RelWithDebInfo `
    -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/msvc2022_64" `
    -DQtCreator_DIR="C:/Qt/Tools/qtcreator/lib/cmake/QtCreator" `
    -DWITH_TESTS=ON
```

编译：

```powershell
cmake --build build-ninja-msvc
```

运行测试：

```powershell
ctest --test-dir build-ninja-msvc --output-on-failure -C RelWithDebInfo
```

构建产物默认位于：

```text
build-ninja-msvc/lib/qtcreator/plugins/DsComplete.dll
```

## 自动打包

在 MSVC Developer PowerShell 中运行：

```powershell
./package.ps1
```

脚本默认执行以下操作：

1. 以 `RelWithDebInfo` 配置编译 `build-ninja-msvc`。
2. 使用 `qtplugininfo` 验证插件 ID、版本和构建类型。
3. 拒绝打包 Debug DLL。
4. 生成只包含根目录 `DsComplete.dll` 的安装 ZIP。
5. 生成对应的 SHA-256 校验文件。

默认产物位于：

```text
dist/DsComplete-<version>-windows-<architecture>.zip
dist/DsComplete-<version>-windows-<architecture>.zip.sha256
```

如果已经完成编译，可以跳过构建：

```powershell
./package.ps1 -SkipBuild
```

使用其他构建目录或 Qt Creator 安装目录：

```powershell
./package.ps1 `
    -BuildDir "D:/path/to/build" `
    -QtCreatorDir "C:/Qt/Tools/qtcreator" `
    -Configuration Release
```

只有在明确需要测试 Debug 插件时才使用 `-AllowDebug`。

## 运行未安装插件

可通过临时插件路径启动 Qt Creator：

```powershell
& "C:/Qt/Tools/qtcreator/bin/qtcreator.exe" `
    -pluginpath "${PWD}/build-ninja-msvc/lib/qtcreator/plugins"
```

也可构建 `RunQtCreator` 目标：

```powershell
cmake --build build-ninja-msvc --target RunQtCreator
```

不要在未确认目标版本和插件 ABI 前直接复制 DLL 到 Qt Creator 安装目录。

## 配置插件

1. 在 Qt Creator 的插件管理界面启用 **DeepSeek Completion**，然后重启 Qt Creator。
2. 打开 **Preferences > AI > DeepSeek Completion**。
3. 填写 API Key。
4. 保持默认 Base URL，或填写兼容的 HTTPS FIM 服务地址。
5. 配置模型、上下文长度、最大输出 token 和自动请求延迟。
6. 启用 **DeepSeek Completion**；如需自动补全，再启用自动请求。

默认模型为 `deepseek-chat`，默认请求路径为 `/beta/completions`。

## 补全行为

自动请求只会在以下条件满足时执行：

- 插件和自动补全均已启用。
- 当前编辑器可写。
- 当前只有一个光标且没有选区。
- 光标位于本次文本修改范围内。
- 当前没有其他可见 suggestion。
- API Key 和 endpoint 有效。

请求期间移动光标、继续编辑或关闭编辑器会使旧结果失效。返回结果只有在文档 revision 和光标位置仍匹配时才会显示。

## 安全与隐私

补全请求会将光标前后的代码片段发送到配置的 API endpoint。启用前应确认有权向该服务发送相关代码。

- API Key 使用 `Core::SecretAspect` 保存。
- API Key 和 Authorization header 不会写入日志。
- 默认只允许 HTTPS endpoint。
- HTTP 仅允许 localhost 或 loopback 地址。
- 插件不忽略 TLS 证书错误。
- 响应体大小限制为 1 MiB。

## 测试范围

协议测试覆盖：

- 前缀和后缀截断。
- FIM payload 构造。
- 请求 URL 规范化。
- HTTPS 和 loopback HTTP 校验。
- 单个及多个候选解析。
- 无效 JSON 拒绝。

涉及编辑器生命周期和网络取消的行为目前通过实现中的 revision、光标和请求 ID 校验保护，后续可加入本地假 HTTP 服务集成测试。
