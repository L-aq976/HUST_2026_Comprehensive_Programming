#include "../include/handle_data.h"
#include "../include/path_finder.h"
#include "../include/safe.h"
#include <iostream>
#include <string>

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
            // 最短路径查找
            std::string src_ip, dst_ip;
            if (iss >> src_ip >> dst_ip) {
                std::cout << "查找最短路径: " << src_ip << " -> " << dst_ip << std::endl;
                
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
        }
        else if (cmd == "HELP") {
            // 帮助信息
            std::cout << "=== 可用命令 ===" << std::endl;
            std::cout << "ANALYZE_IP_RANGE <源IP> <起始IP> <结束IP> - 分析IP范围违规" << std::endl;
            std::cout << "FIND_SHORTEST_PATH <源IP> <目标IP> - 查找最短路径" << std::endl;
            std::cout << "FIND_STAR_STRUCTURES - 查找星型结构" << std::endl;
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