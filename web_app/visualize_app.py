# visualize_app.py
from flask import Flask, render_template, request, jsonify, send_file
import subprocess
import json
import networkx as nx
import matplotlib
matplotlib.use('Agg')   # 必须在 import pyplot 之前
import matplotlib.pyplot as plt
import tempfile
import os
import sys
import time
import shutil
from werkzeug.utils import secure_filename
from pathlib import Path

plt.rcParams['font.sans-serif'] = ['Microsoft YaHei', 'SimHei', 'DejaVu Sans']
plt.rcParams['axes.unicode_minus'] = False

BASE_DIR = Path(__file__).resolve().parent  # web_app目录

app = Flask(
    __name__,
    static_folder=str(BASE_DIR / "static"),
    template_folder=str(BASE_DIR / "templates"),
)

app.config['UPLOAD_FOLDER'] = str(BASE_DIR / "uploads")
app.config['STATIC_FOLDER'] = app.static_folder
app.config['MAX_CONTENT_LENGTH'] = 16 * 1024 * 1024  # 16MB


# 确保目录存在
Path(app.config['UPLOAD_FOLDER']).mkdir(parents=True, exist_ok=True)
Path(app.config['STATIC_FOLDER']).mkdir(parents=True, exist_ok=True)

# 假设C++程序的路径
CPP_PROGRAM = "main"  # 修改为您的C++程序实际路径

def find_cpp_program():
    """查找C++程序"""
    possible_paths = [
        "main",
        "../main",
        "main.exe",
        "../main.exe",
        os.path.join(os.path.dirname(__file__), "..", "main"),
        os.path.join(os.path.dirname(__file__), "..", "bin", "main.exe")
    ]
    
    for path in possible_paths:
        if os.path.exists(path):
            return os.path.abspath(path)
    
    return None

def run_cpp_extract_subgraph(csv_file, target_ip, output_json):
    """调用C++程序提取子图"""
    cpp_program = find_cpp_program()
    if not cpp_program:
        return False, "找不到C++分析程序"
    
    if not os.path.exists(csv_file):
        return False, f"CSV文件不存在: {csv_file}"
    
    try:
        # 构建完整的命令
        command = [cpp_program, csv_file, "VISUALIZE_SUBGRAPH", target_ip, output_json]
        
        # 使用subprocess运行
        result = subprocess.run(
            command,
            capture_output=True,
            text=True,
            timeout=30
        )
        
        if result.returncode != 0:
            return False, f"C++程序错误: {result.stderr}"
        
        return True, "子图提取成功"
        
    except subprocess.TimeoutExpired:
        return False, "C++程序执行超时"
    except Exception as e:
        return False, f"执行C++程序失败: {str(e)}"

def load_subgraph_data(json_file):
    """加载子图JSON数据"""
    with open(json_file, 'r', encoding='utf-8') as f:
        return json.load(f)

def create_networkx_graph(data):
    """创建networkx图对象"""
    G = nx.DiGraph()
    
    # 添加节点
    for node in data.get("nodes", []):
        node_id = node["id"]
        G.add_node(node_id, 
                  ip=node.get("ip", ""),
                  type=node.get("type", "unknown"),
                  label=f"{node.get('ip', '')} ({node.get('type', '')})")
    
    # 添加边
    for edge in data.get("edges", []):
        G.add_edge(edge["source"], edge["target"],
                  flow_count=edge.get("flow_count", 1),
                  data_size=edge.get("total_data_size", 0),
                  avg_duration=edge.get("avg_duration", 0),
                  label=f"{edge.get('flow_count', 0)} flows")
    
    return G

def generate_static_image(data, output_path):
    """生成静态PNG图片"""
    G = create_networkx_graph(data)
    
    plt.figure(figsize=(16, 12))
    
    # 布局
    pos = nx.spring_layout(G, k=2, iterations=100, seed=42)
    
    # 节点颜色和大小
    node_colors = []
    node_sizes = []
    for node in G.nodes():
        node_type = G.nodes[node].get('type', 'unknown')
        if node_type == 'source':
            node_colors.append('#6baed6')
            node_sizes.append(800)
        elif node_type == 'destination':
            node_colors.append('#74c476')
            node_sizes.append(800)
        elif node_type == 'both':
            node_colors.append('#fd8d3c')
            node_sizes.append(1000)
        else:
            node_colors.append('#cccccc')
            node_sizes.append(600)
    
    # 绘制节点
    nx.draw_networkx_nodes(G, pos, 
                          node_color=node_colors,
                          node_size=node_sizes,
                          alpha=0.9,
                          edgecolors='black',
                          linewidths=2)
    
    # 绘制边
    edge_widths = [1 + G[u][v].get('flow_count', 0) * 0.1 for u, v in G.edges()]
    nx.draw_networkx_edges(G, pos,
                          width=edge_widths,
                          alpha=0.6,
                          edge_color='gray',
                          arrows=True,
                          arrowsize=20)
    
    # 标签
    nx.draw_networkx_labels(G, pos,
                           labels={node: G.nodes[node].get('ip', '') for node in G.nodes()},
                           font_size=10)
    
    # 标题
    title = f"网络拓扑图 - 中心IP: {data.get('info', {}).get('central_ip', '')}"
    plt.title(title, fontsize=18, fontweight='bold', pad=20)
    
    plt.axis('off')
    plt.tight_layout()
    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    plt.close()
    
    return output_path

def generate_interactive_html(data, output_path):
    """生成交互式HTML（使用pyvis）"""

    G = create_networkx_graph(data)
    central_ip = data.get('info', {}).get('central_ip', '')
    try:
        from pyvis.network import Network
    except ImportError:
        # 如果pyvis不可用，生成简单的HTML
        return generate_simple_html(data, output_path)
    
    G = create_networkx_graph(data)
    
    # 创建网络
    net = Network(height="750px", width="100%", directed=True, notebook=False)
    
    # 配置物理引擎
    net.barnes_hut(
        gravity=-30000,
        central_gravity=0.5,
        spring_length=100,
        spring_strength=0.01,
        damping=0.12,
        overlap=1
    )
    
    # 添加节点
    for node in G.nodes():
        node_data = G.nodes[node]
        node_type = node_data.get('type', 'unknown')
        
        # 设置颜色
        color = {
            'source': '#6baed6',
            'destination': '#74c476',
            'both': '#fd8d3c',
            'unknown': '#cccccc'
        }.get(node_type, '#cccccc')
        
        # 设置标签
        label = node_data.get('ip', str(node))
        title = f"IP: {node_data.get('ip', '')}<br>Type: {node_type}"
        node_ip = node_data.get('ip', '')
        is_central = (node_ip == central_ip)

        if is_central:
                color = {
                    'background': '#FFD700',   # gold
                    'border': '#B8860B'        # dark goldenrod
                }
                size = 55
        else:
                color = {
                    'source': '#6baed6',
                    'destination': '#74c476',
                    'both': '#fd8d3c',
                    'unknown': '#cccccc'
                }.get(node_type, '#cccccc')
                size = 45
        
        net.add_node(node, 
                    label=label,
                    title=title,
                    color=color,
                    size=45,
                    font={'size': 12})
    
    # 添加边
    for u, v, edge_data in G.edges(data=True):
        flow_count = edge_data.get('flow_count', 0)
        data_size = edge_data.get('data_size', 0)
        
        # 边宽基于流数量
        width = 1 + flow_count * 0.5
        
        # 边标签
        title = (f"从: {G.nodes[u].get('ip', '')}<br>"
                 f"到: {G.nodes[v].get('ip', '')}<br>"
                 f"流数量: {flow_count}<br>"
                 f"总数据量: {data_size:,} bytes")
        
        if 'avg_duration' in edge_data:
            title += f"<br>平均时长: {edge_data['avg_duration']:.2f}s"
        
        net.add_edge(u, v, 
                    title=title,
                    width=width)
    
    # 保存HTML
    net.save_graph(output_path)
    return output_path

def generate_simple_html(data, output_path):
    """生成简单的交互式HTML（不使用pyvis）"""
    G = create_networkx_graph(data)
    central_ip = data.get('info', {}).get('central_ip', '')
    
    # 创建节点和边的数据
    nodes = []
    edges = []
    
    for node in G.nodes():
        node_data = G.nodes[node]
        nodes.append({
            'id': node,
            'label': node_data.get('ip', str(node)),
            'type': node_data.get('type', 'unknown')
        })
    
    for u, v, edge_data in G.edges(data=True):
        edges.append({
            'from': u,
            'to': v,
            'label': f"{edge_data.get('flow_count', 0)} flows"
        })
    
    # 简单的HTML模板
    html_template = """
    <!DOCTYPE html>
    <html>
    <head>
        <title>网络拓扑图</title>
        <script src="https://unpkg.com/vis-network/standalone/umd/vis-network.min.js"></script>
        <style>
            body { margin: 0; padding: 20px; font-family: Arial; }
            #network { width: 100%; height: 90vh; border: 1px solid #ddd; }
            .legend { margin: 20px 0; }
            .legend-item { display: inline-block; margin-right: 20px; }
            .color-box { width: 20px; height: 20px; display: inline-block; margin-right: 5px; }
        </style>
    </head>
    <body>
        <h2>网络拓扑图 - 中心IP: {central_ip}</h2>
        
        <div class="legend">
            <div class="legend-item"><div class="color-box" style="background:#6baed6"></div>源节点</div>
            <div class="legend-item"><div class="color-box" style="background:#74c476"></div>目的节点</div>
            <div class="legend-item"><div class="color-box" style="background:#fd8d3c"></div>既是源又是目的</div>
        </div>
        
        <div id="network"></div>
        
        <script>
            var nodes = new vis.DataSet({nodes_data});
            var edges = new vis.DataSet({edges_data});
            
            var container = document.getElementById('network');
            var data = {{
                nodes: nodes,
                edges: edges
            }};
            var options = {{
                nodes: {{
                    shape: 'dot',
                    size: 20,
                    font: {{
                        size: 12,
                        color: '#000'
                    }},
                    borderWidth: 2
                }},
                edges: {{
                    width: 2,
                    arrows: {{
                        to: {{ enabled: true, scaleFactor: 0.5 }}
                    }},
                    smooth: {{
                        type: 'continuous'
                    }}
                }},
                physics: {{
                    enabled: true,
                    stabilization: {{
                        iterations: 100
                    }}
                }},
                interaction: {{
                    hover: true,
                    tooltipDelay: 200
                }}
            }};
            
            // 根据节点类型设置颜色
            nodes.forEach(function(node) {{
                if (node.type === 'source') {{
                    node.color = {{ background: '#6baed6', border: '#2b7cbb' }};
                }} else if (node.type === 'destination') {{
                    node.color = {{ background: '#74c476', border: '#4cae4c' }};
                }} else if (node.type === 'both') {{
                    node.color = {{ background: '#fd8d3c', border: '#e6550d' }};
                }} else {{
                    node.color = {{ background: '#cccccc', border: '#999999' }};
                }}
            }});
            
            var network = new vis.Network(container, data, options);
            
            // 添加点击事件
            network.on("click", function(params) {{
                if (params.nodes.length > 0) {{
                    var nodeId = params.nodes[0];
                    var node = nodes.get(nodeId);
                    alert("IP: " + node.label + "\\n类型: " + node.type);
                }}
            }});
        </script>
    </body>
    </html>
    """
    
    # 填充模板
    html_content = html_template.format(
        central_ip=data.get('info', {}).get('central_ip', ''),
        nodes_data=json.dumps(nodes),
        edges_data=json.dumps(edges)
    )
    
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(html_content)
    
    return output_path

@app.route('/')
def index():
    return render_template('visualize_index.html')

@app.route('/api/upload_csv', methods=['POST'])
def upload_csv():
    """上传CSV数据文件"""
    if 'file' not in request.files:
        return jsonify({'error': '没有选择文件'}), 400
    
    file = request.files['file']
    if file.filename == '':
        return jsonify({'error': '没有选择文件'}), 400
    
    if file and (file.filename.endswith('.csv') or file.filename.endswith('.CSV')):
        filename = secure_filename(f"csv_{int(time.time())}_{file.filename}")
        filepath = os.path.join(app.config['UPLOAD_FOLDER'], filename)
        file.save(filepath)
        
        return jsonify({
            'success': True,
            'filename': filename,
            'filepath': filepath
        })
    
    return jsonify({'error': '请上传CSV文件'}), 400

@app.route('/api/upload_json', methods=['POST'])
def upload_json():
    """上传JSON子图文件"""
    if 'file' not in request.files:
        return jsonify({'error': '没有选择文件'}), 400
    
    file = request.files['file']
    if file.filename == '':
        return jsonify({'error': '没有选择文件'}), 400
    
    if file and file.filename.endswith('.json'):
        filename = secure_filename(f"json_{int(time.time())}_{file.filename}")
        filepath = os.path.join(app.config['UPLOAD_FOLDER'], filename)
        file.save(filepath)
        
        # 验证JSON格式
        try:
            data = load_subgraph_data(filepath)
            return jsonify({
                'success': True, 
                'filename': filename,
                'filepath': filepath,
                'info': data.get('info', {}),
                'message': '文件上传成功'
            })
        except Exception as e:
            return jsonify({'error': f'无效的JSON文件: {str(e)}'}), 400
    
    return jsonify({'error': '请上传JSON文件'}), 400

@app.route('/api/extract_subgraph', methods=['POST'])
def api_extract_subgraph():
    """从CSV文件提取指定IP的子图"""
    data = request.json
    csv_filename = data.get('csv_filename', '')
    target_ip = data.get('target_ip', '')
    
    if not csv_filename or not target_ip:
        return jsonify({'error': '请提供CSV文件名和目标IP'}), 400
    
    # 获取CSV文件路径
    csv_filepath = os.path.join(app.config['UPLOAD_FOLDER'], csv_filename)
    if not os.path.exists(csv_filepath):
        return jsonify({'error': f'CSV文件不存在: {csv_filename}'}), 404
    
    # 生成输出JSON文件名
    json_filename = f"subgraph_{target_ip}_{int(time.time())}.json"
    json_filepath = os.path.join(app.config['UPLOAD_FOLDER'], json_filename)
    
    # 调用C++程序提取子图
    print(f"调用C++提取子图: CSV={csv_filepath}, IP={target_ip}")
    success, message = run_cpp_extract_subgraph(csv_filepath, target_ip, json_filepath)
    
    if not success:
        return jsonify({'error': message}), 500
    
    # 加载提取的JSON数据
    try:
        subgraph_data = load_subgraph_data(json_filepath)
        return jsonify({
            'success': True,
            'json_filename': json_filename,
            'json_filepath': json_filepath,
            'info': subgraph_data.get('info', {}),
            'message': message
        })
    except Exception as e:
        return jsonify({'error': f'子图提取成功但加载失败: {str(e)}'}), 500

@app.route('/api/generate_visualization', methods=['POST'])
def api_generate_visualization():
    """生成可视化"""
    data = request.json
    json_filename = data.get('json_filename', '')
    viz_type = data.get('type', 'png')
    
    if not json_filename:
        return jsonify({'error': '未指定JSON文件'}), 400
    
    # 获取JSON文件路径
    json_filepath = os.path.join(app.config['UPLOAD_FOLDER'], json_filename)
    if not os.path.exists(json_filepath):
        return jsonify({'error': f'JSON文件不存在: {json_filename}'}), 404
    
    try:
        # 加载数据
        subgraph_data = load_subgraph_data(json_filepath)
        central_ip = subgraph_data.get('info', {}).get('central_ip', 'unknown')
        timestamp = int(time.time())
        
        if viz_type == 'png':
            # 生成静态图片
            output_filename = f"network_{central_ip}_{timestamp}.png"
            output_path = os.path.join(app.config['STATIC_FOLDER'], output_filename)
            generate_static_image(subgraph_data, output_path)
            
            return jsonify({
                'success': True,
                'url': f'/static/{output_filename}',
                'filename': output_filename,
                'type': 'png'
            })
            
        elif viz_type == 'html':
            # 生成交互式HTML
            output_filename = f"network_{central_ip}_{timestamp}.html"
            output_path = os.path.join(app.config['STATIC_FOLDER'], output_filename)
            generate_interactive_html(subgraph_data, output_path)
            
            return jsonify({
                'success': True,
                'url': f'/static/{output_filename}',
                'filename': output_filename,
                'type': 'html'
            })
            
        else:
            return jsonify({'error': f'不支持的格式: {viz_type}'}), 400
            
    except Exception as e:
        return jsonify({'error': f'生成可视化失败: {str(e)}'}), 500

@app.route('/api/get_uploaded_files', methods=['GET'])
def get_uploaded_files():
    """获取已上传的文件列表"""
    csv_files = []
    json_files = []
    
    for filename in os.listdir(app.config['UPLOAD_FOLDER']):
        if filename.endswith('.csv'):
            csv_files.append(filename)
        elif filename.endswith('.json'):
            json_files.append(filename)
    
    return jsonify({
        'csv_files': csv_files,
        'json_files': json_files
    })
@app.route('/test')
def test():
    return "Flask应用运行正常"

@app.route('/test/static')
def test_static():
    """测试静态文件访问"""
    try:
        # 创建一个测试文件
        test_file = os.path.join(app.config['STATIC_FOLDER'], 'test.txt')
        with open(test_file, 'w') as f:
            f.write('测试文件')
        return f"静态文件夹: {app.config['STATIC_FOLDER']}<br>文件已创建: {test_file}"
    except Exception as e:
        return f"错误: {str(e)}"

if __name__ == '__main__':
    print("=== 网络拓扑可视化服务 ===")
    print("访问地址: http://localhost:5001")
    print("\n使用说明:")
    print("1. 上传CSV文件")
    print("2. 输入目标IP提取子图")
    print("3. 生成可视化")
    print("\n或直接上传JSON子图文件")
    
    debug_mode = os.getenv('FLASK_DEBUG', '0').lower() in ('1', 'true', 'yes', 'on')
    host = os.getenv('APP_HOST', '0.0.0.0')
    port = int(os.getenv('VISUALIZE_APP_PORT', '5001'))
    app.run(debug=debug_mode, host=host, port=port)