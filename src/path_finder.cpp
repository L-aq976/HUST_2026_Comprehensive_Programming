#include "path_finder.h"
#include <queue>
#include <unordered_map>
#include <limits>
#include <iostream>

PathFinder::PathFinder(const CSRGraph& graph) : graph(graph) {}

std::vector<int> PathFinder::findShortestPath(const std::string& src_ip, const std::string& dst_ip) {
    int src_id = graph.getSrcVertexID(src_ip);
    int dst_id = graph.getDestVertexID(dst_ip);
    std::vector<int> path;

    if (src_id == -1 || dst_id == -1) {
        std::cerr << "错误: 无法找到源IP或目标IP对应的顶点ID" << std::endl;
        return path;
    }

    int num_vertices = graph.getNumVertices();
    std::vector<long long> dist(num_vertices, std::numeric_limits<long long>::max());
    std::vector<int> parent(num_vertices, -1);
    std::vector<bool> visited(num_vertices, false);

    std::priority_queue<std::pair<long long, int>,
                        std::vector<std::pair<long long, int>>,
                        std::greater<std::pair<long long, int>>> pq;

    dist[src_id] = 0;
    pq.push({0, src_id});

    // 记录“到目标IP”的最优代价和其前驱源节点
    long long best_target_dist = std::numeric_limits<long long>::max();
    int best_target_parent = -1;

    while (!pq.empty()) {
        auto [current_dist, current] = pq.top();
        pq.pop();

        if (visited[current]) continue;
        visited[current] = true;

        for (int edge_id = graph.getOffsets()[current]; edge_id < graph.getOffsets()[current + 1]; edge_id++) {
            int neighbor_dst_id = graph.getEdges()[edge_id];
            const EdgeInfo& edge_info = graph.getEdgeInfos()[edge_id];

            long long congestion_weight = 0;
            if (edge_info.total_duration > 0) {
                congestion_weight = static_cast<long long>(edge_info.total_data_size / edge_info.total_duration);
            } else {
                congestion_weight = edge_info.total_data_size;
            }

            // 到目标IP的距离应该单独比较
            if (neighbor_dst_id == dst_id) {
                long long candidate = current_dist + congestion_weight;
                if (candidate < best_target_dist) {
                    best_target_dist = candidate;
                    best_target_parent = current;
                }
            }

            // 继续在“可作为src出现的节点”上做Dijkstra扩展
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

    if (best_target_parent == -1) {
        std::cerr << "未找到从 " << src_ip << " 到 " << dst_ip << " 的路径" << std::endl;
        return path;
    }

    // 只返回src索引链路，终点dst_ip在main里单独打印，避免ID体系混用
    int temp = best_target_parent;
    while (temp != -1) {
        path.push_back(temp);
        temp = parent[temp];
    }
    std::reverse(path.begin(), path.end());

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

    std::queue<int> q;
    std::vector<int> parent(graph.getNumVertices(), -1);
    std::vector<bool> visited(graph.getNumVertices(), false);

    q.push(src_id);
    visited[src_id] = true;

    int best_target_parent = -1;

    while (!q.empty()) {
        int current = q.front();
        q.pop();

        // 先检查是否可直接到目标
        for (int edge_id = graph.getOffsets()[current]; edge_id < graph.getOffsets()[current + 1]; edge_id++) {
            int neighbor_dst_id = graph.getEdges()[edge_id];
            if (neighbor_dst_id == dst_id) {
                best_target_parent = current;
                break;
            }
        }
        if (best_target_parent != -1) {
            break;
        }

        for (int edge_id = graph.getOffsets()[current]; edge_id < graph.getOffsets()[current + 1]; edge_id++) {
            int neighbor_dst_id = graph.getEdges()[edge_id];
            std::string neighbor_ip = graph.getDestIP(neighbor_dst_id);
            int neighbor_src_id = graph.getSrcVertexID(neighbor_ip);

            if (neighbor_src_id != -1 && !visited[neighbor_src_id]) {
                visited[neighbor_src_id] = true;
                parent[neighbor_src_id] = current;
                q.push(neighbor_src_id);
            }
        }
    }

    if (best_target_parent == -1) {
        std::cerr << "未找到从 " << src_ip << " 到 " << dst_ip << " 的路径" << std::endl;
        return path;
    }

    int temp = best_target_parent;
    while (temp != -1) {
        path.push_back(temp);
        temp = parent[temp];
    }
    std::reverse(path.begin(), path.end());

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

