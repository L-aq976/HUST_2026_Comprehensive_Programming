# 项目部署说明（不打包）

本文档说明如何在一台新 Windows 机器上部署并运行本项目。

## 1. 环境要求

- Windows 10/11
- Python 3.11+（建议使用项目内 `.venv` 或自行创建虚拟环境）
- MinGW g++（用于编译 C++ 后端）
- 浏览器（Chrome/Edge）

## 2. 获取代码

将项目目录放到本地，例如：

- `C:\project\network-analyzer`

## 3. Python 环境与依赖

在项目根目录打开 PowerShell，执行：

```powershell
# 如已有 .venv 可跳过创建
python -m venv .venv

# 激活虚拟环境
.\.venv\Scripts\Activate.ps1

# 安装依赖
python -m pip install --upgrade pip
pip install -r requirements.txt
```

## 4. 编译 C++ 后端

确保编译后得到：

- `bin/main.exe`

可用方式：

```powershell
# 方式1：使用你当前任务里的 g++ 参数（推荐）
# 在 VS Code 中运行任务: Build C++ Project

# 方式2：命令行（示例，按本机 g++ 路径调整）
g++ -std=c++17 -g -I.\include .\src\main.cpp .\src\handle_data.cpp .\src\path_finder.cpp .\src\sort_filter.cpp -o .\bin\main.exe
```

## 5. 启动服务

在项目根目录直接双击：

- `start.bat`

它会启动两个服务并自动打开页面：

- `http://127.0.0.1:5000`（simple_app）
- `http://127.0.0.1:5001`（visualize_app）

## 6. 常见问题

### 6.1 页面打不开

- 看启动窗口是否报错。
- 检查 5000/5001 端口是否被占用。
- 确认 `bin/main.exe` 存在。

### 6.2 报错找不到 Flask 或其他 Python 包

- 重新激活虚拟环境：`.\.venv\Scripts\Activate.ps1`
- 再安装依赖：`pip install -r requirements.txt`

### 6.3 报错找不到 C++ 程序

- 确认 `bin/main.exe` 已编译成功。
- 确认项目目录结构未被改动。

## 7. 更新部署（新代码覆盖）

每次更新代码后，建议执行：

```powershell
.\.venv\Scripts\Activate.ps1
pip install -r requirements.txt
# 重新编译 C++
```

然后重新运行 `start.bat`。
