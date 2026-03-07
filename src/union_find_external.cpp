// union_find_external.cpp
#include "union_find_external.h"
#include <sstream>
#include <algorithm>
#include <iostream>

// 构建统一节点映射
void SubgraphExtractor::buildUnifiedMapping() {
    if (!unified_nodes.empty()) return; // 已经构建过
    
    // 清空现有映射和节点信息
    ip_to_unified_id.clear();
    unified_id_to_ip.clear();
    unified_nodes.clear();
    
    // 第一步：收集所有源IP
    const auto& src_ip_list = graph.getSrcIPList();
    for (int i = 0; i < src_ip_list.size(); ++i) {
        const std::string& ip = src_ip_list[i];
        auto it = ip_to_unified_id.find(ip);
        
        if (it == ip_to_unified_id.end()) {
            // 新IP
            int unified_id = unified_nodes.size();
            ip_to_unified_id[ip] = unified_id;
            unified_id_to_ip.push_back(ip);
            
            SubgraphNode node(ip, unified_id);
            node.is_source = true;
            node.source_id = i;
            unified_nodes.push_back(node);
        } else {
            // IP已存在，更新源IP信息
            int unified_id = it->second;
            unified_nodes[unified_id].is_source = true;
            unified_nodes[unified_id].source_id = i;
        }
    }
    
    // 第二步：收集所有目的IP
    const auto& dest_ip_list = graph.getDestIPList();
    for (int i = 0; i < dest_ip_list.size(); ++i) {
        const std::string& ip = dest_ip_list[i];
        auto it = ip_to_unified_id.find(ip);
        
        if (it == ip_to_unified_id.end()) {
            // 新IP
            int unified_id = unified_nodes.size();
            ip_to_unified_id[ip] = unified_id;
            unified_id_to_ip.push_back(ip);
            
            SubgraphNode node(ip, unified_id);
            node.is_destination = true;
            node.dest_id = i;
            unified_nodes.push_back(node);
        } else {
            // IP已存在，更新目的IP信息
            int unified_id = it->second;
            unified_nodes[unified_id].is_destination = true;
            unified_nodes[unified_id].dest_id = i;
        }
    }
    
    std::cout << "构建统一节点映射完成，总节点数: " << unified_nodes.size() << std::endl;
    std::cout << "源IP数量: " << src_ip_list.size() 
              << ", 目的IP数量: " << dest_ip_list.size() << std::endl;
}

// 构建并查集
void SubgraphExtractor::buildUnionFind() {
    if (uf != nullptr) return; // 已经构建过
    
    // 先构建统一映射
    buildUnifiedMapping();
    
    // 创建并查集
    uf = new UnionFindExternal(unified_nodes.size());
    
    // 遍历CSR图中的所有边，合并源节点和目标节点
    int num_vertices = graph.getNumVertices();
    const auto& offsets = graph.getOffsets();
    const auto& edges = graph.getEdges();
    
    for (int src_local_id = 0; src_local_id < num_vertices; ++src_local_id) {
        const std::string& src_ip = graph.getSrcIP(src_local_id);
        
        // 获取源IP的统一ID
        auto src_it = ip_to_unified_id.find(src_ip);
        if (src_it == ip_to_unified_id.end()) {
            continue; // 应该不会发生
        }
        int src_unified_id = src_it->second;
        
        // 遍历源节点的所有出边
        int start = offsets[src_local_id];
        int end = offsets[src_local_id + 1];
        
        for (int j = start; j < end; ++j) {
            int dest_local_id = edges[j];
            const std::string& dest_ip = graph.getDestIP(dest_local_id);
            
            // 获取目的IP的统一ID
            auto dest_it = ip_to_unified_id.find(dest_ip);
            if (dest_it == ip_to_unified_id.end()) {
                continue; // 应该不会发生
            }
            int dest_unified_id = dest_it->second;
            
            // 合并源节点和目的节点
            uf->unite(src_unified_id, dest_unified_id);
        }
    }
    
    std::cout << "构建并查集完成，总节点数: " << unified_nodes.size() << std::endl;
}

// 提取指定IP所在的子图
SubgraphInfo SubgraphExtractor::extractSubgraph(const std::string& target_ip) {
    SubgraphInfo subgraph;
    subgraph.central_ip = target_ip;
    
    // 构建并查集
    buildUnionFind();
    
    // 检查目标IP是否存在
    auto target_it = ip_to_unified_id.find(target_ip);
    if (target_it == ip_to_unified_id.end()) {
        std::cerr << "错误: IP " << target_ip << " 不存在" << std::endl;
        return subgraph;
    }
    
    int target_unified_id = target_it->second;
    int target_component = uf->find(target_unified_id);
    
    // 收集连通分量中的所有节点
    std::vector<int> component_nodes;
    for (int i = 0; i < unified_nodes.size(); ++i) {
        if (uf->find(i) == target_component) {
            component_nodes.push_back(i);
        }
    }
    
    subgraph.component_size = component_nodes.size();
    
    // 将组件节点添加到子图
    for (int unified_id : component_nodes) {
        subgraph.nodes.push_back(unified_nodes[unified_id]);
    }
    
    // 收集子图中的边
    // 首先构建IP集合用于快速查找
    std::unordered_set<std::string> component_ips;
    for (int unified_id : component_nodes) {
        component_ips.insert(unified_id_to_ip[unified_id]);
    }
    
    // 遍历CSR图的所有边
    int num_vertices = graph.getNumVertices();
    const auto& offsets = graph.getOffsets();
    const auto& edges = graph.getEdges();
    const auto& edge_infos = graph.getEdgeInfos();
    
    for (int src_local_id = 0; src_local_id < num_vertices; ++src_local_id) {
        const std::string& src_ip = graph.getSrcIP(src_local_id);
        
        // 如果源IP不在子图中，跳过
        if (component_ips.find(src_ip) == component_ips.end()) {
            continue;
        }
        
        int src_unified_id = ip_to_unified_id[src_ip];
        int start = offsets[src_local_id];
        int end = offsets[src_local_id + 1];
        
        for (int j = start; j < end; ++j) {
            int dest_local_id = edges[j];
            const std::string& dest_ip = graph.getDestIP(dest_local_id);
            
            // 如果目的IP不在子图中，跳过
            if (component_ips.find(dest_ip) == component_ips.end()) {
                continue;
            }
            
            int dest_unified_id = ip_to_unified_id[dest_ip];
            const EdgeInfo& edge = edge_infos[j];
            
            // 计算平均持续时间
            float avg_duration = edge.flow_count > 0 ? 
                edge.total_duration / edge.flow_count : 0.0f;
            
            // 添加边
            subgraph.edges.emplace_back(
                src_unified_id, dest_unified_id,
                src_ip, dest_ip,
                edge.flow_count,
                edge.total_data_size,
                avg_duration
            );
        }
    }
    
    std::cout << "提取子图完成: 节点数=" << subgraph.nodes.size() 
              << ", 边数=" << subgraph.edges.size() << std::endl;
    
    return subgraph;
}

// 获取所有连通分量
std::vector<std::vector<std::string>> SubgraphExtractor::getAllComponents() {
    buildUnionFind();
    
    // 使用map按连通分量根节点分组
    std::unordered_map<int, std::vector<std::string>> component_map;
    
    for (int i = 0; i < unified_nodes.size(); ++i) {
        int root = uf->find(i);
        component_map[root].push_back(unified_id_to_ip[i]);
    }
    
    // 转换为结果
    std::vector<std::vector<std::string>> result;
    for (const auto& pair : component_map) {
        result.push_back(pair.second);
    }
    
    return result;
}

// 获取连通分量信息
std::string SubgraphExtractor::getComponentInfo(const std::string& target_ip) {
    buildUnionFind();
    
    auto target_it = ip_to_unified_id.find(target_ip);
    if (target_it == ip_to_unified_id.end()) {
        return "{\"error\": \"IP not found\"}";
    }
    
    int target_unified_id = target_it->second;
    int component_id = uf->find(target_unified_id);
    
    // 计算组件大小
    int component_size = 0;
    for (int i = 0; i < unified_nodes.size(); ++i) {
        if (uf->find(i) == component_id) {
            component_size++;
        }
    }
    
    std::stringstream json;
    json << "{";
    json << "\"target_ip\":\"" << target_ip << "\",";
    json << "\"component_id\":" << component_id << ",";
    json << "\"component_size\":" << component_size << ",";
    json << "\"unified_id\":" << target_unified_id << ",";
    json << "\"is_source\":" << (unified_nodes[target_unified_id].is_source ? "true" : "false") << ",";
    json << "\"is_destination\":" << (unified_nodes[target_unified_id].is_destination ? "true" : "false");
    
    // 如果是源IP，添加出度信息
    if (unified_nodes[target_unified_id].is_source) {
        int src_local_id = unified_nodes[target_unified_id].source_id;
        int degree = graph.getNeighborCount(src_local_id);
        json << ",\"out_degree\":" << degree;
    }
    
    json << "}";
    std::cout << "获取组件信息完成: " << json.str() << std::endl;
    return json.str();
}

// 获取IP的统一ID
int SubgraphExtractor::getUnifiedID(const std::string& ip) const {
    auto it = ip_to_unified_id.find(ip);
    if (it != ip_to_unified_id.end()) {
        return it->second;
    }
    return -1;
}

// 获取统一节点信息
const SubgraphNode* SubgraphExtractor::getUnifiedNode(int unified_id) const {
    if (unified_id >= 0 && unified_id < unified_nodes.size()) {
        return &unified_nodes[unified_id];
    }
    return nullptr;
}

const SubgraphNode* SubgraphExtractor::getUnifiedNode(const std::string& ip) const {
    int id = getUnifiedID(ip);
    if (id != -1) {
        return getUnifiedNode(id);
    }
    return nullptr;
}

// SubgraphInfo转换为JSON
std::string SubgraphInfo::toJSON() const {
    std::stringstream json;
    json << "{\"nodes\":[";
    
    // 添加节点
    bool first_node = true;
    for (const auto& node : nodes) {
        if (!first_node) json << ",";
        first_node = false;
        
        json << "{";
        json << "\"id\":" << node.unified_id << ",";
        json << "\"ip\":\"" << node.ip << "\",";
        
        // 确定节点类型
        std::string type = "";
        if (node.is_source && node.is_destination) type = "both";
        else if (node.is_source) type = "source";
        else if (node.is_destination) type = "destination";
        else type = "unknown";
        
        json << "\"type\":\"" << type << "\"";
        json << "}";
    }
    
    json << "],\"edges\":[";
    
    // 添加边
    bool first_edge = true;
    for (const auto& edge : edges) {
        if (!first_edge) json << ",";
        first_edge = false;
        
        json << "{";
        json << "\"source\":" << edge.source_unified_id << ",";
        json << "\"target\":" << edge.target_unified_id << ",";
        json << "\"source_ip\":\"" << edge.source_ip << "\",";
        json << "\"target_ip\":\"" << edge.target_ip << "\",";
        json << "\"flow_count\":" << edge.flow_count << ",";
        json << "\"total_data_size\":" << edge.total_data_size << ",";
        json << "\"avg_duration\":" << edge.avg_duration;
        json << "}";
    }
    
    json << "],\"info\":{";
    json << "\"total_nodes\":" << nodes.size() << ",";
    json << "\"total_edges\":" << edges.size() << ",";
    json << "\"central_ip\":\"" << central_ip << "\",";
    json << "\"component_size\":" << component_size;
    json << "}}";
    
    return json.str();
}