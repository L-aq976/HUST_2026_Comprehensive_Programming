@echo off
echo 启动网络分析系统...

cd /d "%~dp0"

if not exist "web_app" mkdir web_app
if not exist "web_app\templates" mkdir web_app\templates
if not exist "web_app\uploads" mkdir web_app\uploads

call .venv\Scripts\activate.bat

set FLASK_APP=web_app.simple_app
set FLASK_ENV=development
python -m flask run --host=0.0.0.0 --port=5000

pause