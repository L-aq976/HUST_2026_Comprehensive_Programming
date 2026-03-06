#include "../include/handle_data.h"
#include "../include/path_finder.h"
#include "../include/safe.h"
#include "../include/sort_filter.h"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

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
                        std::cout << graph.getSrcIP(path[i]);
                        if (i != path.size() - 1) {
                            std::cout << " -> ";
                        }
                    }
                    std::cout << std::endl;
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
                    std::cout << "IP: " << stat.second << ", 总流量: " << stat.first << " bytes" << std::endl;
                }
            } else {
                std::cout << "未发现单向流量异常" << std::endl;
            }
            
            std::cout << "UNIDIRECTIONAL_STATS_COMPLETE" << std::endl;
        }
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
            std::cout << "HELP - 显示帮助信息" << std::endl;
            std::cout << "EXIT - 退出程序" << std::endl;
        }
        else if (cmd == "EXIT") {
            // 退出程序
            std::cout << "退出程序" << std::endl;
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