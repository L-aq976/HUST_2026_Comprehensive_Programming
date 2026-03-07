#ifndef UNION_FIND_EXTERNAL_H
#define UNION_FIND_EXTERNAL_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include "handle_data.h"

// 简单并查集
class UnionFindExternal {
private:
    std::vector<int> parent;
    std::vector<int> rank;
    
public:
    UnionFindExternal(int n) : parent(n), rank(n, 0) {
        for (int i = 0; i < n; ++i) parent[i] = i;
    }
    
    int find(int x) {
        if (parent[x] != x) parent[x] = find(parent[x]);
        return parent[x];
    }
    
    void unite(int x, int y) {
        int rootX = find(x);
        int rootY = find(y);
        if (rootX != rootY) {
            if (rank[rootX] < rank[rootY]) parent[rootX] = rootY;
            else if (rank[rootX] > rank[rootY]) parent[rootY] = rootX;
            else { parent[rootY] = rootX; rank[rootX]++; }
        }
    }
};

// 子图节点信息
struct SubgraphNode {
    std::string ip;
    int unified_id;
    bool is_source;
    bool is_destination;
    int source_id;  // 在src_ip_list中的ID
    int dest_id;    // 在dest_ip_list中的ID
    
    SubgraphNode() : unified_id(-1), is_source(false), is_destination(false), 
                    source_id(-1), dest_id(-1) {}
};

// 子图边信息
struct SubgraphEdge {
    int source_unified_id;
    int target_unified_id;
    std::string source_ip;
    std::string target_ip;
    int flow_count;
    long long total_data_size;
    float avg_duration;
    
    SubgraphEdge(int src_uid, int tgt_uid, const std::string& src_ip, 
                 const std::string& tgt_ip, int fcount, long long tsize, float avg_dur) :
        source_unified_id(src_uid), target_unified_id(tgt_uid), 
        source_ip(src_ip), target_ip(tgt_ip), flow_count(fcount), 
        total_data_size(tsize), avg_duration(avg_dur) {}
};

// 完整的子图结构
struct SubgraphInfo {
    std::vector<SubgraphNode> nodes;
    std::vector<SubgraphEdge> edges;
    std::string central_ip;
    int component_size;
    
    std::string toJSON() const;
};

// 子图提取器
class SubgraphExtractor {
private:
    const CSRGraph& graph;
    
    // IP到统一ID的映射
    std::unordered_map<std::string, int> ip_to_unified_id;
    std::vector<std::string> unified_id_to_ip;
    std::vector<SubgraphNode> unified_nodes;
    
    UnionFindExternal* uf = nullptr;
    
    // 构建统一映射
    void buildUnifiedMapping();
    
    // 构建并查集
    void buildUnionFind();
    
public:
    SubgraphExtractor(const CSRGraph& g) : graph(g) {}
    ~SubgraphExtractor() { if (uf) delete uf; }
    
    // 提取子图
    SubgraphInfo extractSubgraph(const std::string& target_ip);
};

#endif