#include "handle_data.h"

// EdgeInfo 构造函数的实现
EdgeInfo::EdgeInfo() : flow_count(0), total_data_size(0), total_duration(0.0f), src_port(0), dst_port(0) {}

EdgeInfo::EdgeInfo(int count, int size, float dur, int src_port, int dst_port) 
    : flow_count(count), total_data_size(size), total_duration(dur), src_port(src_port), dst_port(dst_port) {}

// CSRGraph 私有方法的实现
int CSRGraph::src_get_or_createID(const std::string& ip) {
    auto it = src_ip_to_id.find(ip);
    if (it != src_ip_to_id.end()) {
        return it->second;
    }
    
    int new_id = static_cast<int>(src_ip_list.size());
    src_ip_list.push_back(ip);
    src_ip_to_id[ip] = new_id;
    return new_id;
}

int CSRGraph::dest_get_or_createID(const std::string& ip) {
    auto it = dest_ip_to_id.find(ip);
    if (it != dest_ip_to_id.end()) {
        return it->second;
    }
    
    int new_id = static_cast<int>(dest_ip_list.size());
    dest_ip_list.push_back(ip);
    dest_ip_to_id[ip] = new_id;
    return new_id;
}

std::vector<FlowRecord> CSRGraph::parseCSV(const std::string& filename) {
    std::vector<FlowRecord> flows;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "错误: 无法打开文件 " << filename << std::endl;
        return flows;
    }
    
    std::string line;
    int line_num = 0;
    
    // 读取文件
    while (std::getline(file, line)) {
        line_num++;
        
        // 跳过标题行
        if (line_num == 1) continue;
        
        // 跳过空行
        if (line.empty()) continue;
        
        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> tokens;
        
        // 分割逗号分隔的值
        while (std::getline(ss, token, ',')) {
            tokens.push_back(token);
        }
        
        // 确保有足够的字段
        if (tokens.size() >= 7) {
            FlowRecord record;
            record.source_ip = tokens[0];
            record.dest_ip = tokens[1];
            record.protocol = tokens[2];
            
            // 转换端口和数值
            try {
                record.src_port = std::stoi(tokens[3]);
                record.dst_port = std::stoi(tokens[4]);
                record.data_size = std::stoi(tokens[5]);
                record.duration = std::stof(tokens[6]);
            } catch (const std::exception& e) {
                std::cerr << "警告: 行 " << line_num << " 数据格式错误" << std::endl;
                continue;
            }
            
            flows.push_back(record);
        } else {
            std::cerr << "警告: 行 " << line_num << " 字段数量不足" << std::endl;
        }
    }
    
    std::cout << "成功解析 " << flows.size() << " 条流量记录" << std::endl;
    return flows;
}

// CSRGraph 公共方法的实现
bool CSRGraph::buildFromCSV(const std::string& filename) {
    // 1. 解析CSV文件
    std::vector<FlowRecord> flows = parseCSV(filename);
    if (flows.empty()) {
        std::cerr << "错误: 没有可处理的流量记录" << std::endl;
        return false;
    }
    
    // 2. 清空现有数据
    offsets.clear();
    edges.clear();
    edge_infos.clear();
    src_ip_list.clear();
    src_ip_to_id.clear();
    dest_ip_list.clear();
    dest_ip_to_id.clear();
    
    // 3. 第一步分别收集src节点和dest节点的ID
    std::cout << "第一步: 收集IP并统计出度..." << std::endl;
    
    for (const auto& flow : flows) {
        int src_id = src_get_or_createID(flow.source_ip);
        int dst_id = dest_get_or_createID(flow.dest_ip);
    }
    
    // 4. 第二步：构建邻接列表
    std::cout << "第二步: 构建邻接列表..." << std::endl;
    
    // 邻接列表：每个顶点有一个列表，存储(目标ID, 流信息)
    std::vector<std::vector<std::pair<int, EdgeInfo>>> adjacency(src_ip_list.size());
    
    for (const auto& flow : flows) {
        int src_id = src_ip_to_id[flow.source_ip];
        int dst_id = dest_ip_to_id[flow.dest_ip];
        
        // 创建边信息
        EdgeInfo edge(1, flow.data_size, flow.duration, flow.src_port, flow.dst_port);
        
        // 添加到邻接列表
        adjacency[src_id].push_back(std::make_pair(dst_id, edge));
    }
    
    // 5. 第三步：对每个顶点的邻居排序并合并相同目标
    std::cout << "第三步: 排序和合并..." << std::endl;
    
    // 先计算每个顶点的唯一邻居数量
    std::vector<int> unique_out_degree(src_ip_list.size(), 0);
    
    for (int src_id = 0; src_id < src_ip_list.size(); src_id++) {
        if (!adjacency[src_id].empty()) {
            // 按目标ID排序
            std::sort(adjacency[src_id].begin(), adjacency[src_id].end(),
                     [](const auto& a, const auto& b) {
                         return a.first < b.first;
                     });
            
            // 合并相同目标并计数
            int unique_count = 0;
            for (size_t i = 0; i < adjacency[src_id].size(); i++) {
                if (i == 0 || adjacency[src_id][i].first != adjacency[src_id][i-1].first) {
                    unique_count++;
                }
            }
            
            unique_out_degree[src_id] = unique_count;
        }
    }
    
    // 6. 第四步：构建CSR数组
    std::cout << "第四步: 构建CSR数组..." << std::endl;
    
    // 6.1 构建offsets数组
    offsets.resize(src_ip_list.size() + 1, 0); //offsets[0] = 0
    for (int i = 0; i < src_ip_list.size(); i++) {
        offsets[i + 1] = offsets[i] + unique_out_degree[i];
    }
    
    int total_edges = offsets[src_ip_list.size()];
    std::cout << "总边数: " << total_edges << std::endl;
    
    // 6.2 分配edges和edge_infos数组
    edges.resize(total_edges);
    edge_infos.resize(total_edges);
    
    // 6.3 填充CSR数组
    int num_vertices = static_cast<int>(src_ip_list.size());
    for (int src_id = 0; src_id < num_vertices; src_id++) {
        int edge_start = offsets[src_id]; // 该顶点的边在edges数组中的起始索引
        int edge_index = 0; // 该顶点的边在edges数组中的当前索引
        
        for (size_t j = 0; j < adjacency[src_id].size(); j++) {
            int dst_id = adjacency[src_id][j].first;
            const EdgeInfo& curr_edge = adjacency[src_id][j].second;
            
            if (j == 0 || dst_id != adjacency[src_id][j-1].first) {
                // 新边
                edges[edge_start + edge_index] = dst_id;
                edge_infos[edge_start + edge_index] = curr_edge;
                edge_index++;
            } else {
                // 合并到现有边，累加流量统计
                int idx = edge_start + edge_index - 1;
                edge_infos[idx].flow_count += curr_edge.flow_count;
                edge_infos[idx].total_data_size += curr_edge.total_data_size;
                edge_infos[idx].total_duration += curr_edge.total_duration;
            }
        }
    }
    
    std::cout << "CSR构建完成!" << std::endl;
    std::cout << "顶点数: " << src_ip_list.size() << ", 边数: " << total_edges << std::endl;
    
    return true;
}

// 其他方法实现
const std::vector<int>& CSRGraph::getOffsets() const { return offsets; }
const std::vector<int>& CSRGraph::getEdges() const { return edges; }
const std::vector<EdgeInfo>& CSRGraph::getEdgeInfos() const { return edge_infos; }

const std::vector<std::string>& CSRGraph::getSrcIPList() const { return src_ip_list; }
const std::unordered_map<std::string, int>& CSRGraph::getSrcIPToIDMap() const { return src_ip_to_id; }

const std::vector<std::string>& CSRGraph::getDestIPList() const { return dest_ip_list; }
const std::unordered_map<std::string, int>& CSRGraph::getDestIPToIDMap() const { return dest_ip_to_id; }

int CSRGraph::getNumVertices() const { return static_cast<int>(src_ip_list.size()); }
int CSRGraph::getNumEdges() const { return static_cast<int>(edges.size()); }
int CSRGraph::getNumDestVertices() const { return static_cast<int>(dest_ip_list.size()); }

int CSRGraph::getSrcVertexID(const std::string& ip) const {
    auto it = src_ip_to_id.find(ip);
    return (it != src_ip_to_id.end()) ? it->second : -1;
}

int CSRGraph::getDestVertexID(const std::string& ip) const {
    auto it = dest_ip_to_id.find(ip);
    return (it != dest_ip_to_id.end()) ? it->second : -1;
}

std::string CSRGraph::getSrcIP(int vertex_id) const {
    if (vertex_id >= 0 && vertex_id < src_ip_list.size()) {
        return src_ip_list[vertex_id];
    }
    return "";
}

std::string CSRGraph::getDestIP(int vertex_id) const {
    if (vertex_id >= 0 && vertex_id < dest_ip_list.size()) {
        return dest_ip_list[vertex_id];
    }
    return "";
}

int CSRGraph::getNeighborCount(int vertex_id) const {
    if (vertex_id < 0 || vertex_id >= offsets.size() - 1) {
        return 0;
    }
    return offsets[vertex_id + 1] - offsets[vertex_id];
}

bool CSRGraph::getNeighborInfo(int vertex_id, int neighbor_index, 
                              int& dst_id, EdgeInfo& edge_info) const {
    if (vertex_id < 0 || vertex_id >= offsets.size() - 1) {
        return false;
    }
    
    int start = offsets[vertex_id];
    int end = offsets[vertex_id + 1];
    
    if (neighbor_index < 0 || neighbor_index >= (end - start)) {
        return false;
    }
    
    int edge_idx = start + neighbor_index;
    dst_id = edges[edge_idx];
    edge_info = edge_infos[edge_idx];
    
    return true;
}

void CSRGraph::printGraphInfo() const {
    std::cout << "\n=== CSR图信息 ===" << std::endl;
    std::cout << "顶点数: " << getNumVertices() << std::endl;
    std::cout << "边数: " << getNumEdges() << std::endl;
    
    // 计算平均出度
    int total_neighbors = 0;
    int num_vertices = getNumVertices();
    for (int i = 0; i < num_vertices; i++) {
        total_neighbors += getNeighborCount(i);
    }
    
    float avg_out_degree = static_cast<float>(total_neighbors) / getNumVertices();
    std::cout << "平均出度: " << avg_out_degree << std::endl;
    
    // 统计总流量
    long long total_data = 0;
    int total_flows = 0;
    for (const auto& edge : edge_infos) {
        total_data += edge.total_data_size;
        total_flows += edge.flow_count;
    }
    
    std::cout << "总流数: " << total_flows << std::endl;
    std::cout << "总数据量: " << total_data << " bytes" << std::endl;
}
// 获取目的IP对应的顶点ID数目（目的IP对应的入度）
int CSRGraph::getDestVertexIDCount(int dest_vertex_id) const {
    int count = 0;
    for (const auto& edge : edge_infos) {
        if (edge.dst_port == dest_vertex_id) {
            count++;
        }
    }
    return count;
}


//星型结构实现
std::vector<int> CSRGraph::findStarStructures() {
    std::vector<int> star_centers;
    for (int vertex_id = 0; vertex_id < getNumVertices(); vertex_id++) {
        int neighbor_count = getNeighborCount(vertex_id);
        
        if (neighbor_count >= 20) {
            //首先每个邻居都必须只和中心节点建立链接，即邻居的目的顶点ID必须是中心节点的ID
                bool is_star_center = true;
            //    for (int i = 0; i < neighbor_count; i++) {
            //         int dst_id;
            //         EdgeInfo edge_info;
            //         //入度必须是1
            //         // int desidcount = getDestVertexIDCount(edges[offsets[vertex_id]+i]);
            //         getNeighborInfo(vertex_id, i, dst_id, edge_info);
            //         if (dst_id != vertex_id ) {
            //             is_star_center = false;
            //             break;
            //         }
            //     }
            
            if (is_star_center) {
                star_centers.push_back(vertex_id);
            }
        }
    }
    
    return star_centers;
}

//该功能要求找到图中的所有星型结构，并以“中心节点地址：相连节点1，相邻节点2......” 的格式输出。
void CSRGraph::printfStarStructures(const std::vector<int>& star_centers) {
    std::cout << "\n=== 星型结构中心节点 ===" << std::endl;
    for (int center_id : star_centers) {
        std::cout << "中心节点ID: " << center_id 
                  << ", IP: " << getSrcIP(center_id) 
                  << ", 邻居数量: " << getNeighborCount(center_id) << std::endl;
        std::cout << "相连节点: "<< std::endl;
        for (int i = 0; i < getNeighborCount(center_id); i++) {
            int dst_id;
            EdgeInfo edge_info;
            getNeighborInfo(center_id, i, dst_id, edge_info);
            std::cout << getDestIP(dst_id) << " "<< std::endl;
        }
        std::cout << std::endl;
    }
}