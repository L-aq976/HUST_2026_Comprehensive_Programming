#!/usr/bin/env python3
"""
独立可视化脚本
可以直接运行: python visualize.py <json文件>
"""

import json
import networkx as nx
import matplotlib.pyplot as plt
import argparse
import os
import sys
from pyvis.network import Network

def load_data(filename):
    """加载JSON数据"""
    with open(filename, 'r', encoding='utf-8') as f:
        return json.load(f)

def create_networkx_graph(data):
    """创建networkx图对象"""
    G = nx.DiGraph()
    
    # 添加节点
    for node in data.get("nodes", []):
        G.add_node(node["id"], 
                  ip=node.get("ip", ""),
                  type=node.get("type", "unknown"))
    
    # 添加边
    for edge in data.get("edges", []):
        G.add_edge(edge["source"], edge["target"],
                  flow_count=edge.get("flow_count", 1),
                  data_size=edge.get("total_data_size", 0),
                  avg_duration=edge.get("avg_duration", 0))
    
    return G

def generate_static_image(data, output_file, title=None):
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
    labels = {node: G.nodes[node].get('ip', '') for node in G.nodes()}
    nx.draw_networkx_labels(G, pos, labels, font_size=10, font_weight='bold')
    
    # 标题
    if title is None:
        title = f"Network Topology - Central IP: {data.get('info', {}).get('central_ip', '')}"
    plt.title(title, fontsize=18, fontweight='bold', pad=20)
    
    plt.axis('off')
    plt.tight_layout()
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    plt.close()
    
    print(f"✓ 静态图已保存: {output_file}")
    return output_file

def generate_interactive_html(data, output_file):
    """生成交互式HTML"""
    G = create_networkx_graph(data)
    
    # 使用pyvis
    net = Network(height="750px", width="100%", directed=True, notebook=False)
    
    # 配置物理引擎
    net.barnes_hut(
        gravity=-80000,
        central_gravity=0.3,
        spring_length=250,
        spring_strength=0.001,
        damping=0.09,
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
        
        net.add_node(node, 
                    label=label,
                    title=title,
                    color=color,
                    size=25,
                    font={'size': 12, 'face': 'Arial'})
    
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
                    width=width,
                    smooth={'type': 'dynamic'})
    
    # 保存HTML
    net.save_graph(output_file)
    print(f"✓ 交互式HTML已保存: {output_file}")
    return output_file

def print_statistics(data):
    """打印统计信息"""
    info = data.get('info', {})
    print("\n=== 子图统计信息 ===")
    print(f"中心IP: {info.get('central_ip', 'N/A')}")
    print(f"节点数量: {info.get('total_nodes', 0)}")
    print(f"边数量: {info.get('total_edges', 0)}")
    print(f"连通分量大小: {info.get('component_size', 0)}")
    
    # 节点类型统计
    node_types = {}
    for node in data.get('nodes', []):
        node_type = node.get('type', 'unknown')
        node_types[node_type] = node_types.get(node_type, 0) + 1
    
    print("\n节点类型分布:")
    for node_type, count in node_types.items():
        print(f"  {node_type}: {count}")
    
    # 流量统计
    total_data = 0
    total_flows = 0
    for edge in data.get('edges', []):
        total_data += edge.get('total_data_size', 0)
        total_flows += edge.get('flow_count', 0)
    
    print(f"\n流量统计:")
    print(f"  总数据量: {total_data:,} bytes")
    print(f"  总流数: {total_flows}")
    if total_flows > 0:
        print(f"  平均每流数据量: {total_data/total_flows:,.0f} bytes")

def main():
    parser = argparse.ArgumentParser(description='网络拓扑可视化工具')
    parser.add_argument('json_file', help='JSON子图文件路径')
    parser.add_argument('-o', '--output', help='输出文件前缀（默认使用JSON文件名）')
    parser.add_argument('-t', '--title', help='图表标题')
    parser.add_argument('--png', action='store_true', help='生成PNG图片')
    parser.add_argument('--html', action='store_true', help='生成HTML交互式图')
    parser.add_argument('--all', action='store_true', help='生成所有格式')
    
    args = parser.parse_args()
    
    if not os.path.exists(args.json_file):
        print(f"错误: 文件不存在 {args.json_file}")
        sys.exit(1)
    
    # 加载数据
    print(f"正在加载数据: {args.json_file}")
    data = load_data(args.json_file)
    
    # 打印统计信息
    print_statistics(data)
    
    # 确定输出文件名前缀
    if args.output:
        base_name = args.output
    else:
        base_name = os.path.splitext(os.path.basename(args.json_file))[0]
    
    # 生成可视化
    print("\n正在生成可视化...")
    
    generated_files = []
    
    # 生成PNG
    if args.png or args.all:
        png_file = f"{base_name}.png"
        generate_static_image(data, png_file, args.title)
        generated_files.append(png_file)
    
    # 生成HTML
    if args.html or args.all:
        html_file = f"{base_name}.html"
        generate_interactive_html(data, html_file)
        generated_files.append(html_file)
    
    # 如果没指定格式，默认生成两种
    if not (args.png or args.html or args.all):
        png_file = f"{base_name}.png"
        html_file = f"{base_name}.html"
        generate_static_image(data, png_file, args.title)
        generate_interactive_html(data, html_file)
        generated_files.extend([png_file, html_file])
    
    # 输出使用说明
    print("\n✓ 可视化生成完成！")
    for file in generated_files:
        print(f"  {file}")
    
    if html_file in generated_files:
        print(f"\n在浏览器中打开交互式图:")
        print(f"  file://{os.path.abspath(html_file)}")
        print(f"  或直接双击 {html_file}")

if __name__ == '__main__':
    # 检查依赖
    try:
        import networkx
        import matplotlib
    except ImportError:
        print("错误: 请先安装必要的Python库")
        print("  pip install networkx matplotlib pyvis")
        sys.exit(1)
    
    main()