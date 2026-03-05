#include "../include/handle_data.h"
#include "../include/path_finder.h"
#include "../include/safe.h"
#include <iostream>
#include <string>

/**
 * @brief 主函数 - 演示CSRGraph类的使用
 */
int main() {
    std::cout << "=== PathFinder 测试程序 ===" << std::endl;

    // 创建CSR图对象
    CSRGraph graph;

    // 从CSV文件构建图
    std::string filename = "data/processed_data_balanced.csv"; // 请确保该文件存在并且路径正确
    if (!graph.buildFromCSV(filename)) {
        std::cerr << "错误: 无法构建CSR图" << std::endl;
        return -1;
    }

    // 创建PathFinder对象
    PathFinder pathFinder(graph);

    // 测试 findShortestPath 功能
    std::string src_ip = "112.65.219.203";
    std::string dst_ip = "182.129.59.184";
    std::cout << "\n测试基于拥塞程度的最短路径查找:" << std::endl;
    std::vector<int> shortestPath = pathFinder.findShortestPath(src_ip, dst_ip);
    if (!shortestPath.empty()) {
        std::cout << "路径: ";
        for (size_t i = 0; i < shortestPath.size(); i++) {
            std::cout << graph.getSrcIP(shortestPath[i]);
            if (i != shortestPath.size() - 1) {
                std::cout << " -> ";
            }
        }
        std::cout << std::endl;
    } else {
        std::cout << "未找到路径" << std::endl;
    }

    // 测试 findShortestPathByHops 功能
    std::cout << "\n测试基于跳数的最短路径查找:" << std::endl;
    std::vector<int> shortestPathByHops = pathFinder.findShortestPathByHops(src_ip, dst_ip);
    if (!shortestPathByHops.empty()) {
        std::cout << "路径: ";
        for (size_t i = 0; i < shortestPathByHops.size(); i++) {
            std::cout << graph.getSrcIP(shortestPathByHops[i]);
            if (i != shortestPathByHops.size() - 1) {
                std::cout << " -> ";
            }
        }
        std::cout << std::endl;
    } else {
        std::cout << "未找到路径" << std::endl;
    }

    // 创建Safe对象
    Safe safe(graph);
    // 分析IP范围：检查IP1是否与IP2-IP3范围内的地址建立了会话
    safe.analyzeIPRange("183.94.22.71", "27.44.123.4", "27.44.123.255");

    // 查找星型结构
    std::vector<int> star_centers = graph.findStarStructures(); 
    graph.printfStarStructures(star_centers);


    // 调试代码：将邻接列表内容写入文件
    std::ofstream debug_file("debug_output.txt");
    if (debug_file.is_open()) {
        debug_file << "邻接列表内容:" << std::endl;
        for (size_t i = 0; i < graph.getOffsets().size() - 1; ++i) {
            debug_file << "顶点 " << graph.getSrcIP(i) << " 的邻居:" << std::endl;
            for (int j = graph.getOffsets()[i]; j < graph.getOffsets()[i + 1]; ++j) {
                int neighbor = graph.getEdges()[j];
                const EdgeInfo& edge = graph.getEdgeInfos()[j];
                debug_file << "  -> " << graph.getDestIP(neighbor)
                           << " (数据大小: " << edge.total_data_size
                           << ", 持续时间: " << edge.total_duration << ")" << std::endl;
            }
        }
        debug_file.close();
    } else {
        std::cerr << "无法打开调试输出文件" << std::endl;
    }

    return 0;
}