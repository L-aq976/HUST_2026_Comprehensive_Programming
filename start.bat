@echo off
echo 启动网络分析系统...

if not exist "web_app" (
    echo 创建目录结构...
    mkdir web_app
    mkdir web_app\templates
    mkdir web_app\uploads
)

if not exist ".venv" (
    echo 创建虚拟环境...
    uv venv
)

echo 激活虚拟环境...
call .venv\Scripts\activate.bat

echo 安装依赖...
uv sync

echo 启动Web服务器...
set FLASK_APP=web_app.app
set FLASK_ENV=development
flask run --host=0.0.0.0 --port=5000

pause