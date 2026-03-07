#include "../include/handle_data.h"
#include "../include/path_finder.h"
#include "../include/safe.h"
#include "../include/sort_filter.h"
#include "../include/union_find_external.h" 
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <fstream>  
#include <iomanip>

/**
 * @brief 支持命令行交互的网络分析程序
 */
int main(int argc, char* argv[]) {
    // 检查命令行参数
    if (argc < 2) {
        std::cerr << "用法: " << argv[0] << " <csv文件路径>" << std::endl;
        std::cerr << "示例: " << argv[0] << " data/network_data.csv" << std::endl;
        return 1;
    }
    if (argc == 5) {
        std::string command = argv[2];
        if (command == "VISUALIZE_SUBGRAPH") {
            std::string target_ip = argv[3];
            std::string output_file = argv[4];
            
            // 构建CSR图
            CSRGraph graph;
            if (!graph.buildFromCSV(argv[1])) {
                std::cerr << "错误: 无法构建CSR图" << std::endl;
                return 1;
            }
            
            // 提取子图
            SubgraphExtractor extractor(graph);
            SubgraphInfo subgraph = extractor.extractSubgraph(target_ip);
            
            if (subgraph.nodes.empty()) {
                std::cerr << "错误: 未找到IP " << target_ip << " 或子图为空" << std::endl;
                return 1;
            }
            
            // 保存JSON
            std::string json_output = subgraph.toJSON();
            std::ofstream out_file(output_file);
            if (out_file.is_open()) {
                out_file << json_output;
                out_file.close();
                std::cout << "子图提取成功，保存到: " << output_file << std::endl;
                return 0;
            } else {
                std::cerr << "错误: 无法创建输出文件" << std::endl;
                return 1;
            }
        }
    }
    std::string filename = argv[1];
    
    std::cout << "=== 网络分析系统 ===" << std::endl;
    std::cout << "正在加载数据文件: " << filename << std::endl;

    // 创建CSR图对象
    CSRGraph graph;

    // 从CSV文件构建图
    if (!graph.buildFromCSV(filename)) {
        std::cerr << "错误: 无法构建CSR图" << std::endl;
        return -1;
    }

    // 创建分析对象
    PathFinder pathFinder(graph);
    Safe safe(graph);
    SortFilter sortFilter;

    // 子图提取器（在需要时创建）
    SubgraphExtractor* subgraphExtractor = nullptr;

    std::cout << "图加载完成! 顶点数: " << graph.getNumVertices() 
              << ", 边数: " << graph.getNumEdges() << std::endl;
    std::cout << "系统就绪，等待命令..." << std::endl;
    std::cout << "输入 HELP 查看可用命令" << std::endl;

    // 命令行交互循环
    std::string command;
    while (true) {
        std::cout << ">>> ";
        if (!std::getline(std::cin, command)) {
            break;
        }
        
        if (command.empty()) {
            continue;
        }

        // 解析命令
        std::istringstream iss(command);
        std::string cmd;
        iss >> cmd;

        if (cmd == "ANALYZE_IP_RANGE") {
            // IP范围分析命令
            std::string ip1, ip2, ip3;
            if (iss >> ip1 >> ip2 >> ip3) {
                std::cout << "开始分析IP范围违规..." << std::endl;
                std::cout << "源IP: " << ip1 << std::endl;
                std::cout << "禁止范围: " << ip2 << " - " << ip3 << std::endl;
                
                safe.analyzeIPRange(ip1, ip2, ip3);
                std::cout << "ANALYSIS_COMPLETE" << std::endl;
            } else {
                std::cout << "错误: 命令格式不正确" << std::endl;
                std::cout << "正确格式: ANALYZE_IP_RANGE <源IP> <起始IP> <结束IP>" << std::endl;
            }
        }
        else if (cmd == "FIND_SHORTEST_PATH") {
            // 最短路径查找（拥塞程度）
            std::string src_ip, dst_ip;
            if (iss >> src_ip >> dst_ip) {
                std::cout << "查找基于拥塞程度的最短路径: " << src_ip << " -> " << dst_ip << std::endl;
                
                std::vector<int> path = pathFinder.findShortestPath(src_ip, dst_ip);
                if (!path.empty()) {
                    std::cout << "路径: ";
                    for (size_t i = 0; i < path.size(); i++) {
                            std::cout << graph.getSrcIP(path[i]) << " -> ";
                    }
                    std::cout << dst_ip << std::endl;
                } else {
                    std::cout << "未找到路径" << std::endl;
                }
                std::cout << "PATH_FINDING_COMPLETE" << std::endl;
            } else {
                std::cout << "错误: 命令格式不正确" << std::endl;
                std::cout << "正确格式: FIND_SHORTEST_PATH <源IP> <目标IP>" << std::endl;
            }
        }
        else if (cmd == "FIND_SHORTEST_PATH_BY_HOPS") {
            // 最短路径查找（跳数）
            std::string src_ip, dst_ip;
            if (iss >> src_ip >> dst_ip) {
                std::cout << "查找基于跳数的最短路径: " << src_ip << " -> " << dst_ip << std::endl;
                
                std::vector<int> path = pathFinder.findShortestPathByHops(src_ip, dst_ip);
                if (!path.empty()) {
                    std::cout << "路径: ";
                    for (size_t i = 0; i < path.size(); i++) {
                        std::cout << graph.getSrcIP(path[i]);
                        if (i != path.size() - 1) {
                            std::cout << " -> ";
                        }
                    }
                    std::cout << std::endl;
                } else {
                    std::cout << "未找到路径" << std::endl;
                }
                std::cout << "PATH_FINDING_BY_HOPS_COMPLETE" << std::endl;
            } else {
                std::cout << "错误: 命令格式不正确" << std::endl;
                std::cout << "正确格式: FIND_SHORTEST_PATH_BY_HOPS <源IP> <目标IP>" << std::endl;
            }
        }
        else if (cmd == "FIND_STAR_STRUCTURES") {
            // 查找星型结构
            std::cout << "查找星型结构..." << std::endl;
            std::vector<int> star_centers = graph.findStarStructures(); 
            graph.printfStarStructures(star_centers);
            std::cout << "STAR_ANALYSIS_COMPLETE" << std::endl;
        }
        else if (cmd == "GRAPH_INFO") {
            // 显示图信息
            graph.printGraphInfo();
            std::cout << "GRAPH_INFO_COMPLETE" << std::endl;
        }
        else if (cmd == "TRAFFIC_STATISTICS") {
            // 流量统计分析
            std::cout << "开始流量统计分析..." << std::endl;
            
            // 计算总流量
            long long total_data = 0;
            int total_flows = 0;
            for (const auto& edge : graph.getEdgeInfos()) {
                total_data += edge.total_data_size;
                total_flows += edge.flow_count;
            }
            
            std::cout << "总数据量: " << total_data << " bytes" << std::endl;
            std::cout << "总流数: " << total_flows << std::endl;
            std::cout << "平均每流数据量: " << (total_flows > 0 ? total_data / total_flows : 0) << " bytes" << std::endl;
            
            std::cout << "TRAFFIC_STATS_COMPLETE" << std::endl;
        }
        else if (cmd == "HTTPS_STATISTICS") {
            // HTTPS统计分析
            std::cout << "开始HTTPS统计分析..." << std::endl;
            
            // 调用SortFilter类的HTTPS统计功能
            auto https_stats = sortFilter.findHTTPSsrc(graph);
            if (!https_stats.empty()) {
                std::cout << "HTTPS连接统计:" << std::endl;
                for (const auto& stat : https_stats) {
                    std::cout << "IP: " << stat.second << ", 总流量: " << stat.first << " bytes" << std::endl;
                }
            } else {
                std::cout << "未发现HTTPS连接" << std::endl;
            }
            
            std::cout << "HTTPS_STATS_COMPLETE" << std::endl;
        }
        else if (cmd == "UNIDIRECTIONAL_TRAFFIC") {
            // 单向流量统计分析
            std::cout << "开始单向流量统计分析..." << std::endl;
            
            // 调用SortFilter类的单向流量统计功能
            auto unidirectional_stats = sortFilter.findbadsrc(graph);
            if (!unidirectional_stats.empty()) {
                std::cout << "单向流量异常节点:" << std::endl;
                for (const auto& stat : unidirectional_stats) {
                int src_id = graph.getSrcVertexID(stat.second);
                long long out_traffic = 0;

                if (src_id != -1) {
                    for (int edge_id = graph.getOffsets()[src_id]; edge_id < graph.getOffsets()[src_id + 1]; edge_id++) {
                        out_traffic += graph.getEdgeInfos()[edge_id].total_data_size;
                    }
                }

                double ratio = (stat.first > 0) ? (100.0 * out_traffic / stat.first) : 0.0;

    std::cout << "IP: " << stat.second
              << ", 总流量: " << stat.first << " bytes"
              << ", 单向流量占比: " << std::fixed << std::setprecision(2) << ratio << "%"
              << std::endl;
}
            } else {
                std::cout << "未发现单向流量异常" << std::endl;
            }
            
            std::cout << "UNIDIRECTIONAL_STATS_COMPLETE" << std::endl;
        }
        // ========== 新增的子图可视化命令 ==========
        else if (cmd == "VISUALIZE_SUBGRAPH") {
            // 子图可视化命令
            std::string target_ip;
            std::string output_file = "subgraph.json";  // 默认输出文件名
            
            if (iss >> target_ip) {
                // 可选：指定输出文件名
                std::string filename_option;
                if (iss >> filename_option) {
                    output_file = filename_option;
                }
                
                std::cout << "开始提取子图，目标IP: " << target_ip << std::endl;
                
                // 创建子图提取器
                if (subgraphExtractor == nullptr) {
                    subgraphExtractor = new SubgraphExtractor(graph);
                }
                
                // 提取子图
                SubgraphInfo subgraph = subgraphExtractor->extractSubgraph(target_ip);
                
                if (subgraph.nodes.empty()) {
                    std::cout << "错误: 未找到IP " << target_ip << " 或子图为空" << std::endl;
                } else {
                    // 输出统计信息
                    std::cout << "子图统计信息:" << std::endl;
                    std::cout << "  中心IP: " << subgraph.central_ip << std::endl;
                    std::cout << "  节点数: " << subgraph.nodes.size() << std::endl;
                    std::cout << "  边数: " << subgraph.edges.size() << std::endl;
                    
                    // 保存为JSON文件
                    std::string json_output = subgraph.toJSON();
                    std::ofstream out_file(output_file);
                    if (out_file.is_open()) {
                        out_file << json_output;
                        out_file.close();
                        std::cout << "✓ 子图数据已保存到: " << output_file << std::endl;
                        
                        // 提供Python脚本使用说明
                        std::cout << "\n下一步: 运行Python进行可视化" << std::endl;
                        std::cout << "  1. 确保已安装Python库: pip install networkx matplotlib" << std::endl;
                        std::cout << "  2. 运行可视化脚本: python visualize_subgraph.py " << output_file << std::endl;
                    } else {
                        std::cout << "错误: 无法创建输出文件" << std::endl;
                    }
                }
                std::cout << "SUBPGRAPH_EXTRACTION_COMPLETE" << std::endl;
            } else {
                std::cout << "错误: 命令格式不正确" << std::endl;
                std::cout << "正确格式: VISUALIZE_SUBGRAPH <目标IP> [输出文件]" << std::endl;
                std::cout << "  输出文件可选，默认: subgraph.json" << std::endl;
            }
        }
        //生成脚本文件的简单命令，当只有cpp文件的时候可以进行尝试，本项目并没有使用
        else if (cmd == "GENERATE_VIZ_SCRIPT") {
            // 生成Python可视化脚本
            std::cout << "生成Python可视化脚本..." << std::endl;
            
            std::string python_script = R"(#!/usr/bin/env python3
# 网络拓扑可视化脚本
# 使用方法: python visualize_subgraph.py <json文件>

import json
import networkx as nx
import matplotlib.pyplot as plt
import sys
import os

def load_data(filename):
    """加载JSON数据"""
    with open(filename, 'r', encoding='utf-8') as f:
        return json.load(f)

def create_graph(data):
    """创建networkx图"""
    G = nx.DiGraph()
    
    # 添加节点
    for node in data['nodes']:
        G.add_node(node['id'], ip=node['ip'], type=node['type'])
    
    # 添加边
    for edge in data['edges']:
        G.add_edge(edge['source'], edge['target'], 
                  flow_count=edge['flow_count'],
                  data_size=edge['total_data_size'])
    
    return G

def visualize(graph, data, output_png="network.png"):
    """生成可视化"""
    plt.figure(figsize=(12, 8))
    
    # 布局
    pos = nx.spring_layout(graph, k=2, iterations=50, seed=42)
    
    # 节点颜色
    node_colors = []
    for node in graph.nodes():
        node_type = graph.nodes[node]['type']
        if node_type == 'source':
            node_colors.append('lightblue')
        elif node_type == 'destination':
            node_colors.append('lightgreen')
        elif node_type == 'both':
            node_colors.append('orange')
        else:
            node_colors.append('gray')
    
    # 绘制
    nx.draw_networkx_nodes(graph, pos, node_color=node_colors, node_size=500)
    nx.draw_networkx_edges(graph, pos, arrows=True, arrowsize=20)
    nx.draw_networkx_labels(graph, pos, 
                           labels={node: graph.nodes[node]['ip'] for node in graph.nodes()},
                           font_size=10)
    
    plt.title(f"Network Topology - Central IP: {data['info']['central_ip']}")
    plt.axis('off')
    plt.tight_layout()
    plt.savefig(output_png, dpi=300, bbox_inches='tight')
    plt.close()
    
    print(f"可视化图片已保存: {output_png}")

def main():
    if len(sys.argv) < 2:
        print("用法: python visualize_subgraph.py <json文件> [输出图片]")
        sys.exit(1)
    
    json_file = sys.argv[1]
    output_png = sys.argv[2] if len(sys.argv) > 2 else "network.png"
    
    if not os.path.exists(json_file):
        print(f"错误: 文件不存在 {json_file}")
        sys.exit(1)
    
    # 加载数据
    print(f"加载数据: {json_file}")
    data = load_data(json_file)
    
    info = data['info']
    print(f"中心IP: {info['central_ip']}")
    print(f"节点数: {info['total_nodes']}")
    print(f"边数: {info['total_edges']}")
    
    # 创建图
    G = create_graph(data)
    
    # 可视化
    print("生成可视化...")
    visualize(G, data, output_png)
    
    print("完成!")

if __name__ == "__main__":
    main()
)";

            std::string python_filename = "visualize_subgraph.py";
            std::ofstream py_file(python_filename);
            if (py_file.is_open()) {
                py_file << python_script;
                py_file.close();
                std::cout << "✓ Python脚本已生成: " << python_filename << std::endl;
                std::cout << "  运行: python " << python_filename << " subgraph.json" << std::endl;
            } else {
                std::cout << "错误: 无法生成Python脚本" << std::endl;
            }
            std::cout << "VIZ_SCRIPT_GENERATION_COMPLETE" << std::endl;
        }
        // ========== 修改HELP命令 ==========
        else if (cmd == "HELP") {
            // 帮助信息
            std::cout << "=== 可用命令 ===" << std::endl;
            std::cout << "ANALYZE_IP_RANGE <源IP> <起始IP> <结束IP> - 分析IP范围违规" << std::endl;
            std::cout << "FIND_SHORTEST_PATH <源IP> <目标IP> - 基于拥塞程度的最短路径" << std::endl;
            std::cout << "FIND_SHORTEST_PATH_BY_HOPS <源IP> <目标IP> - 基于跳数的最短路径" << std::endl;
            std::cout << "FIND_STAR_STRUCTURES - 查找星型结构" << std::endl;
            std::cout << "TRAFFIC_STATISTICS - 流量统计分析" << std::endl;
            std::cout << "HTTPS_STATISTICS - HTTPS统计分析" << std::endl;
            std::cout << "UNIDIRECTIONAL_TRAFFIC - 单向流量统计分析" << std::endl;
            std::cout << "GRAPH_INFO - 显示图信息" << std::endl;
            std::cout << "VISUALIZE_SUBGRAPH <目标IP> [输出文件] - 提取子图用于可视化" << std::endl;
            std::cout << "GENERATE_VIZ_SCRIPT - 生成Python可视化脚本" << std::endl;
            std::cout << "HELP - 显示帮助信息" << std::endl;
            std::cout << "EXIT - 退出程序" << std::endl;
        }
        else if (cmd == "EXIT") {
            // 退出程序
            std::cout << "退出程序" << std::endl;
            
            // 清理子图提取器
            if (subgraphExtractor != nullptr) {
                delete subgraphExtractor;
                subgraphExtractor = nullptr;
            }
            
            break;
        }
        else {
            std::cout << "未知命令: " << cmd << std::endl;
            std::cout << "输入 HELP 查看可用命令" << std::endl;
        }
    }

    std::cout << "程序结束" << std::endl;
    return 0;
}