@echo off
chcp 65001 >nul
setlocal

echo 启动网络分析系统（双页面）...
cd /d "%~dp0"

if not exist "web_app\uploads" mkdir "web_app\uploads"
if not exist "web_app\static" mkdir "web_app\static"

set "PYTHON_EXE=%~dp0.venv\Scripts\python.exe"
if not exist "%PYTHON_EXE%" (
    echo 未找到虚拟环境 Python：%PYTHON_EXE%
    echo 将尝试使用系统 python...
    set "PYTHON_EXE=python"
)

echo 启动 simple_app (5000)...
start "Simple App - 5000" cmd /k ""%PYTHON_EXE%" "%~dp0web_app\simple_app.py""

echo 启动 visualize_app (5001)...
start "Visualize App - 5001" cmd /k ""%PYTHON_EXE%" "%~dp0web_app\visualize_app.py""

timeout /t 3 /nobreak >nul

echo 打开浏览器页面...
start "" "http://127.0.0.1:5000"
start "" "http://127.0.0.1:5001"

echo.
echo 已启动：
echo - http://127.0.0.1:5000  (simple_app)
echo - http://127.0.0.1:5001  (visualize_app)
echo.
pause