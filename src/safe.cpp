#include"safe.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

Safe::Safe(const CSRGraph& graph) : graph(graph) {}

// 将IP地址转换为32位整数，用于范围比较
uint32_t ipToUint32(const std::string& ip) {
    std::istringstream iss(ip);
    std::string segment;
    uint32_t result = 0;
    
    for (int i = 0; i < 4; i++) {
        if (!std::getline(iss, segment, '.')) {
            return 0;
        }
        result = (result << 8) + std::stoi(segment);
    }
    return result;
}

// 检查IP地址是否在指定范围内
bool isInIPRange(const std::string& ip, const std::string& start_ip, const std::string& end_ip) {
    uint32_t ip_val = ipToUint32(ip);
    uint32_t start_val = ipToUint32(start_ip);
    uint32_t end_val = ipToUint32(end_ip);
    
    return ip_val >= start_val && ip_val <= end_val;
}

//该功能输入3个IP地址，分别是地址1，地址2，地址3。
//其中地址2和地址3构成一个地址范围，表示地址1禁止\允许与该范围内的地址建立会话
//并输出所有违反该规则的会话
void Safe::analyzeIPRange(const std::string& ip1, const std::string& ip2, const std::string& ip3) {
    std::cout << "正在分析IP范围..." << std::endl;
    
    // 验证IP地址格式
    if (ip1.empty() || ip2.empty() || ip3.empty()) {
        std::cout << "错误: IP地址不能为空" << std::endl;
        return;
    }
    
    // 检查IP1是否存在于图中
    int src_vertex_id = graph.getSrcVertexID(ip1);
    if (src_vertex_id == -1) {
        std::cout << "警告: 源IP地址 " << ip1 << " 不在图中" << std::endl;
        return;
    }
    
    std::cout << "安全规则: IP " << ip1 << " 禁止与范围 [" << ip2 << " - " << ip3 << "] 内的地址建立会话" << std::endl;
    std::cout << "正在检查违规会话..." << std::endl;
    
    int violation_count = 0;
    
    // 获取IP1的所有邻居（会话）
    int neighbor_count = graph.getNeighborCount(src_vertex_id);
    
    for (int i = 0; i < neighbor_count; i++) {
        int dst_vertex_id;
        EdgeInfo edge_info;
        
        if (graph.getNeighborInfo(src_vertex_id, i, dst_vertex_id, edge_info)) {
            std::string dest_ip = graph.getDestIP(dst_vertex_id);
            
            // 检查目标IP是否在禁止范围内
            if (isInIPRange(dest_ip, ip2, ip3)) {
                violation_count++;
                
                std::cout << "违规会话 " << violation_count << ": " << std::endl;
                std::cout << "  源IP: " << ip1 << std::endl;
                std::cout << "  目标IP: " << dest_ip << " (在禁止范围内)" << std::endl;
                std::cout << "  会话统计:" << std::endl;
                std::cout << "    流数量: " << edge_info.flow_count << std::endl;
                std::cout << "    总数据量: " << edge_info.total_data_size << " bytes" << std::endl;
                std::cout << "    总时长: " << std::fixed << std::setprecision(2) << edge_info.total_duration << " 秒" << std::endl;
                std::cout << "    源端口: " << edge_info.src_port << std::endl;
                std::cout << "    目标端口: " << edge_info.dst_port << std::endl;
                std::cout << std::endl;
            }
        }
    }
    
    if (violation_count == 0) {
        std::cout << "未发现违规会话。IP " << ip1 << " 与范围 [" << ip2 << " - " << ip3 << "] 内的地址没有建立会话。" << std::endl;
    } else {
        std::cout << "共发现 " << violation_count << " 个违规会话。" << std::endl;
    }
}
