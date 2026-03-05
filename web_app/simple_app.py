from flask import Flask, render_template, request, jsonify
import os
import subprocess
import threading
import queue
import time
import json

app = Flask(__name__)
app.config['UPLOAD_FOLDER'] = 'uploads'

class SimpleCppInterface:
    """简单的C++程序接口"""
    
    def __init__(self):
        self.process = None
        self.is_running = False
        self.output_queue = queue.Queue()
        
    def start_analysis(self, csv_file_path):
        """启动C++分析程序"""
        try:
            cpp_program = self._find_cpp_program()
            if not cpp_program:
                return False, "找不到C++分析程序"
            
            if not os.path.exists(csv_file_path):
                return False, f"CSV文件不存在: {csv_file_path}"
            
            self.process = subprocess.Popen(
                [cpp_program, csv_file_path],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                bufsize=1
            )
            
            self.is_running = True
            threading.Thread(target=self._read_output, daemon=True).start()
            
            if self._wait_for_ready():
                return True, "C++分析程序启动成功"
            else:
                return False, "C++程序启动超时"
            
        except Exception as e:
            return False, f"启动C++程序失败: {str(e)}"
    
    def _find_cpp_program(self):
        """查找C++程序"""
        possible_paths = [
            os.path.join(os.path.dirname(__file__), '..', 'main.exe'),
            os.path.join(os.path.dirname(__file__), '..', 'bin', 'main.exe')
        ]
        for path in possible_paths:
            if os.path.exists(path):
                return path
        return None
    
    def _read_output(self):
        """读取C++程序输出"""
        while self.is_running and self.process and self.process.poll() is None:
            try:
                line = self.process.stdout.readline()
                if line:
                    line = line.strip()
                    if line:
                        print(f"[C++] {line}")
                        self.output_queue.put(line)
            except:
                break
    
    def _wait_for_ready(self, timeout=10):
        """等待程序就绪"""
        start_time = time.time()
        while time.time() - start_time < timeout:
            try:
                output = self.output_queue.get(timeout=1)
                if "系统就绪" in output:
                    return True
            except queue.Empty:
                continue
        return False
    
    def send_command(self, command, timeout=30):
        """向C++程序发送命令"""
        if not self.is_running or not self.process:
            return False, "程序未运行"
        
        try:
            print(f"[Python] 发送命令: {command}")
            self.process.stdin.write(command + '\n')
            self.process.stdin.flush()
            
            # 等待命令完成
            start_time = time.time()
            completion_markers = {
                "ANALYZE_IP_RANGE": "ANALYSIS_COMPLETE",
                "FIND_SHORTEST_PATH": "PATH_FINDING_COMPLETE", 
                "FIND_STAR_STRUCTURES": "STAR_ANALYSIS_COMPLETE",
                "TRAFFIC_STATISTICS": "TRAFFIC_STATS_COMPLETE",
                "HTTPS_STATISTICS": "HTTPS_STATS_COMPLETE",
                "UNIDIRECTIONAL_TRAFFIC": "UNIDIRECTIONAL_STATS_COMPLETE"
            }
            
            marker = completion_markers.get(command.split()[0], "COMPLETE")
            results = []
            
            while time.time() - start_time < timeout:
                try:
                    output = self.output_queue.get(timeout=1)
                    results.append(output)
                    
                    if marker in output:
                        return True, "\n".join(results)
                        
                except queue.Empty:
                    continue
            
            return False, f"命令执行超时: {command}"
                
        except Exception as e:
            return False, f"发送命令失败: {str(e)}"
    
    def stop(self):
        """停止程序"""
        if self.process:
            try:
                self.send_command("EXIT", timeout=5)
                time.sleep(2)
            except:
                pass
            finally:
                self.process.terminate()
                self.process.wait()
        self.is_running = False

# 全局C++接口实例
cpp_interface = SimpleCppInterface()

@app.route('/')
def index():
    return render_template('simple_index.html')

@app.route('/api/upload', methods=['POST'])
def upload_file():
    if 'file' not in request.files:
        return jsonify({'error': '没有选择文件'}), 400
    
    file = request.files['file']
    if file.filename == '':
        return jsonify({'error': '没有选择文件'}), 400
    
    if file and file.filename.endswith('.csv'):
        filename = f"upload_{int(time.time())}.csv"
        upload_dir = os.path.join(app.root_path, 'uploads')
        os.makedirs(upload_dir, exist_ok=True)
        filepath = os.path.join(upload_dir, filename)
        file.save(filepath)
        
        if cpp_interface.is_running:
            cpp_interface.stop()
            time.sleep(2)
        
        success, message = cpp_interface.start_analysis(filepath)
        
        if success:
            return jsonify({'success': True, 'filename': filename, 'message': message})
        else:
            return jsonify({'error': message}), 500
    
    return jsonify({'error': '请上传CSV文件'}), 400

@app.route('/api/analyze/ip_range', methods=['POST'])
def analyze_ip_range():
    """IP范围分析"""
    data = request.json
    ip1 = data.get('ip1', '').strip()
    ip2 = data.get('ip2', '').strip()
    ip3 = data.get('ip3', '').strip()
    
    if not all([ip1, ip2, ip3]):
        return jsonify({'error': 'IP地址不能为空'}), 400
    
    command = f"ANALYZE_IP_RANGE {ip1} {ip2} {ip3}"
    success, result = cpp_interface.send_command(command)
    
    if success:
        return jsonify({'success': True, 'result': result})
    else:
        return jsonify({'error': result}), 500

@app.route('/api/analyze/shortest_path', methods=['POST'])
def analyze_shortest_path():
    """最短路径分析"""
    data = request.json
    src_ip = data.get('src_ip', '').strip()
    dst_ip = data.get('dst_ip', '').strip()
    
    if not all([src_ip, dst_ip]):
        return jsonify({'error': '源IP和目标IP不能为空'}), 400
    
    command = f"FIND_SHORTEST_PATH {src_ip} {dst_ip}"
    success, result = cpp_interface.send_command(command)
    
    if success:
        return jsonify({'success': True, 'result': result})
    else:
        return jsonify({'error': result}), 500

@app.route('/api/analyze/star_structures', methods=['POST'])
def analyze_star_structures():
    """星型结构分析"""
    command = "FIND_STAR_STRUCTURES"
    success, result = cpp_interface.send_command(command)
    
    if success:
        return jsonify({'success': True, 'result': result})
    else:
        return jsonify({'error': result}), 500

@app.route('/api/analyze/traffic_statistics', methods=['POST'])
def analyze_traffic_statistics():
    """流量统计分析"""
    command = "TRAFFIC_STATISTICS"
    success, result = cpp_interface.send_command(command)
    
    if success:
        return jsonify({'success': True, 'result': result})
    else:
        return jsonify({'error': result}), 500

@app.route('/api/analyze/https_statistics', methods=['POST'])
def analyze_https_statistics():
    """HTTPS统计分析"""
    command = "HTTPS_STATISTICS"
    success, result = cpp_interface.send_command(command)
    
    if success:
        return jsonify({'success': True, 'result': result})
    else:
        return jsonify({'error': result}), 500

@app.route('/api/analyze/unidirectional_traffic', methods=['POST'])
def analyze_unidirectional_traffic():
    """单向流量统计分析"""
    command = "UNIDIRECTIONAL_TRAFFIC"
    success, result = cpp_interface.send_command(command)
    
    if success:
        return jsonify({'success': True, 'result': result})
    else:
        return jsonify({'error': result}), 500

@app.route('/api/graph_info')
def get_graph_info():
    """获取图信息"""
    command = "GRAPH_INFO"
    success, result = cpp_interface.send_command(command)
    
    if success:
        return jsonify({'success': True, 'result': result})
    else:
        return jsonify({'error': result}), 500

@app.route('/api/status')
def get_status():
    """获取程序状态"""
    status = 'running' if cpp_interface.is_running else 'stopped'
    return jsonify({'status': status})

@app.route('/api/stop')
def stop_analysis():
    """停止分析程序"""
    cpp_interface.stop()
    return jsonify({'success': True, 'message': '分析程序已停止'})

def main():
    app.run(debug=True, host='0.0.0.0', port=5000)

if __name__ == '__main__':
    main()