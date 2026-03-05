#include "path_finder.h"
#include <queue>
#include <unordered_map>
#include <limits>
#include <iostream>

PathFinder::PathFinder(const CSRGraph& graph) : graph(graph) {}

std::vector<int> PathFinder::findShortestPath(const std::string& src_ip, const std::string& dst_ip) {
    int src_id = graph.getSrcVertexID(src_ip); // 获取源IP的索引
    int dst_id = graph.getDestVertexID(dst_ip); // 获取目标IP的索引
    std::vector<int> path;

    if (src_id == -1 || dst_id == -1) {
        std::cerr << "错误: 无法找到源IP或目标IP对应的顶点ID" << std::endl;
        return path;
    }

    // 使用Dijkstra算法，权重为拥塞程度（流量/持续时间）
    int num_vertices = graph.getNumVertices();
    std::vector<long long> dist(num_vertices, std::numeric_limits<long long>::max());
    std::vector<int> parent(num_vertices, -1);
    std::vector<bool> visited(num_vertices, false);
    
    // 使用优先队列（最小堆）
    std::priority_queue<std::pair<long long, int>, 
                        std::vector<std::pair<long long, int>>, 
                        std::greater<std::pair<long long, int>>> pq;

    dist[src_id] = 0;
    pq.push({0, src_id});

    while (!pq.empty()) {
        auto [current_dist, current] = pq.top();
        pq.pop();

        if (visited[current]) continue;
        visited[current] = true;

        // 检查当前源IP是否直接连接到目标IP
        for (int edge_id = graph.getOffsets()[current]; edge_id < graph.getOffsets()[current + 1]; edge_id++) {
            int neighbor_dst_id = graph.getEdges()[edge_id];
            
            // 计算拥塞程度权重：流量/持续时间
            const EdgeInfo& edge_info = graph.getEdgeInfos()[edge_id];
            long long congestion_weight = 0;
            if (edge_info.total_duration > 0) {
                congestion_weight = edge_info.total_data_size / edge_info.total_duration;
            } else {
                congestion_weight = edge_info.total_data_size; // 如果持续时间为0，使用流量作为权重
            }
            
            if (neighbor_dst_id == dst_id) {
                // 找到能够直接到达目标IP的路径
                if (current_dist + congestion_weight < dist[current]) {
                    dist[current] = current_dist + congestion_weight;
                    parent[current] = current; // 自环，表示直接连接
                }
                
                // 构建路径：从源IP到当前源IP，然后到目标IP
                int temp = current;
                while (temp != -1) {
                    path.push_back(temp);
                    temp = parent[temp];
                }
                std::reverse(path.begin(), path.end());
                
                // 在路径末尾添加目标IP（使用目标IP索引系统的ID）
                path.push_back(dst_id);
                
                return path;
            }
            
            // 将目标IP转换回源IP索引系统（如果存在）
            std::string neighbor_ip = graph.getDestIP(neighbor_dst_id);
            int neighbor_src_id = graph.getSrcVertexID(neighbor_ip);
            
            if (neighbor_src_id != -1) {
                long long new_dist = current_dist + congestion_weight;
                if (new_dist < dist[neighbor_src_id]) {
                    dist[neighbor_src_id] = new_dist;
                    parent[neighbor_src_id] = current;
                    pq.push({new_dist, neighbor_src_id});
                }
            }
        }
    }

    std::cerr << "未找到从 " << src_ip << " 到 " << dst_ip << " 的路径" << std::endl;
    return path;
}


std::vector<int> PathFinder::findShortestPathByHops(const std::string& src_ip, const std::string& dst_ip) {
    int src_id = graph.getSrcVertexID(src_ip);
    int dst_id = graph.getDestVertexID(dst_ip);
    std::vector<int> path;

    if (src_id == -1 || dst_id == -1) {
        std::cerr << "错误: 无法找到源IP或目的IP对应的顶点ID" << std::endl;
        return path;
    }

    // BFS 实现最小跳数路径查找
    std::queue<int> q;
    std::vector<int> parent(graph.getNumVertices(), -1); // 使用源IP索引系统的大小
    std::vector<bool> visited(graph.getNumVertices(), false); // 使用源IP索引系统的大小

    q.push(src_id);
    visited[src_id] = true;

    while (!q.empty()) {
        int current = q.front();
        q.pop();

        // 检查当前源IP是否直接连接到目标IP
        for (int edge_id = graph.getOffsets()[current]; edge_id < graph.getOffsets()[current + 1]; edge_id++) {
            int neighbor_dst_id = graph.getEdges()[edge_id];
            if (neighbor_dst_id == dst_id) {
                // 找到能够直接到达目标IP的源IP
                
                // 构建路径：从源IP到当前源IP，然后到目标IP
                int temp = current;
                while (temp != -1) {
                    path.push_back(temp);
                    temp = parent[temp];
                }
                std::reverse(path.begin(), path.end());
                
                // 在路径末尾添加目标IP（使用目标IP索引系统的ID）
                path.push_back(dst_id);
                
                return path;
            }
        }

        // 继续搜索邻居
        for (int edge_id = graph.getOffsets()[current]; edge_id < graph.getOffsets()[current + 1]; edge_id++) {
            int neighbor_dst_id = graph.getEdges()[edge_id];
            
            // 将目标IP转换回源IP索引系统（如果存在）
            std::string neighbor_ip = graph.getDestIP(neighbor_dst_id);
            int neighbor_src_id = graph.getSrcVertexID(neighbor_ip);
            
            if (neighbor_src_id != -1 && !visited[neighbor_src_id]) {
                visited[neighbor_src_id] = true;
                parent[neighbor_src_id] = current;
                q.push(neighbor_src_id);
            }
        }
    }

    std::cerr << "未找到从 " << src_ip << " 到 " << dst_ip << " 的路径" << std::endl;
    return path;
}
void PathFinder::printPath(const std::vector<int>& path) {
    if (path.empty()) {
        std::cout << "路径为空" << std::endl;
        return;
    }
    for (size_t i = 0; i < path.size(); i++) {
        std::cout << graph.getSrcIP(path[i]);
        if (i != path.size() - 1) {
            std::cout << " -> ";
        }
    }
    std::cout << std::endl;
}

