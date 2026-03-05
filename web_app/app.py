from flask import Flask, render_template, request, jsonify
import os
import subprocess
import threading
import queue
from datetime import datetime

app = Flask(__name__)
app.config['UPLOAD_FOLDER'] = 'uploads'
app.config['MAX_CONTENT_LENGTH'] = 16 * 1024 * 1024

class CppAnalyzer:
    def __init__(self):
        self.process = None
        self.output_queue = queue.Queue()
        
    def start(self, csv_file):
        """启动C++分析程序"""
        try:
            cpp_program = os.path.join(os.path.dirname(__file__), '..', 'bin', 'main.exe')
            if not os.path.exists(cpp_program):
                return False
                
            self.process = subprocess.Popen(
                [cpp_program, csv_file],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                bufsize=1
            )
            
            threading.Thread(target=self._read_output, daemon=True).start()
            return True
        except Exception as e:
            print(f"启动失败: {e}")
            return False
    
    def _read_output(self):
        while self.process and self.process.poll() is None:
            line = self.process.stdout.readline()
            if line:
                self.output_queue.put(line.strip())
    
    def send_command(self, command):
        if self.process and self.process.poll() is None:
            self.process.stdin.write(command + '\n')
            self.process.stdin.flush()
            return True
        return False
    
    def get_output(self, timeout=5):
        try:
            return self.output_queue.get(timeout=timeout)
        except queue.Empty:
            return None

analyzer = CppAnalyzer()

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/api/upload', methods=['POST'])
def upload_file():
    if 'file' not in request.files:
        return jsonify({'error': '没有文件'}), 400
        
    file = request.files['file']
    if file.filename == '':
        return jsonify({'error': '没有文件'}), 400
        
    if file and file.filename.endswith('.csv'):
        filename = f"upload_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
        upload_dir = os.path.join(app.root_path, 'uploads')
        os.makedirs(upload_dir, exist_ok=True)
        filepath = os.path.join(upload_dir, filename)
        file.save(filepath)
        
        if analyzer.start(filepath):
            return jsonify({'success': True, 'filename': filename})
        return jsonify({'error': '启动失败'}), 500
        
    return jsonify({'error': '需要CSV文件'}), 400

@app.route('/api/analyze/ip_range', methods=['POST'])
def analyze_ip_range():
    data = request.json
    ip1 = data.get('ip1', '')
    ip2 = data.get('ip2', '')
    ip3 = data.get('ip3', '')
    
    if not all([ip1, ip2, ip3]):
        return jsonify({'error': 'IP不能为空'}), 400
        
    command = f"ANALYZE_IP_RANGE {ip1} {ip2} {ip3}"
    if analyzer.send_command(command):
        result = analyzer.get_output(timeout=10)
        return jsonify({'success': True, 'result': result})
        
    return jsonify({'error': '分析失败'}), 500

@app.route('/api/status')
def get_status():
    if analyzer.process and analyzer.process.poll() is None:
        return jsonify({'status': 'running'})
    return jsonify({'status': 'stopped'})

def main():
    app.run(debug=True, host='0.0.0.0', port=5000)

if __name__ == '__main__':
    main()