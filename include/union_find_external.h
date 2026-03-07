// union_find_external.h
#ifndef UNION_FIND_EXTERNAL_H
#define UNION_FIND_EXTERNAL_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "handle_data.h"

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

// 子图节点信息结构
struct SubgraphNode {
    std::string ip;        // IP地址
    int unified_id;        // 统一ID
    bool is_source;        // 是否是源IP
    bool is_destination;   // 是否是目的IP
    int source_id;         // 在源IP列表中的ID（如果不是源IP则为-1）
    int dest_id;           // 在目的IP列表中的ID（如果不是目的IP则为-1）
    
    SubgraphNode() : unified_id(-1), is_source(false), is_destination(false), 
                    source_id(-1), dest_id(-1) {}
    
    SubgraphNode(const std::string& ip, int uid) : 
        ip(ip), unified_id(uid), is_source(false), is_destination(false), 
        source_id(-1), dest_id(-1) {}
};

// 子图边信息结构
struct SubgraphEdge {
    int source_unified_id;    // 源节点统一ID
    int target_unified_id;    // 目标节点统一ID
    std::string source_ip;    // 源IP
    std::string target_ip;    // 目标IP
    int flow_count;           // 流数量
    long long total_data_size; // 总数据量
    float avg_duration;       // 平均持续时间
    
    SubgraphEdge(int src_uid, int tgt_uid, const std::string& src_ip, 
                 const std::string& tgt_ip, int fcount, long long tsize, float avg_dur) :
        source_unified_id(src_uid), target_unified_id(tgt_uid), 
        source_ip(src_ip), target_ip(tgt_ip), flow_count(fcount), 
        total_data_size(tsize), avg_duration(avg_dur) {}
};

// 完整的子图结构
struct SubgraphInfo {
    std::vector<SubgraphNode> nodes;    // 所有节点
    std::vector<SubgraphEdge> edges;    // 所有边
    std::string central_ip;             // 中心IP（查询的目标IP）
    int component_size;                 // 连通分量大小
    
    // 转换为JSON字符串
    std::string toJSON() const;
    
    // 清空
    void clear() {
        nodes.clear();
        edges.clear();
        central_ip = "";
        component_size = 0;
    }
};

// 子图提取器类
class SubgraphExtractor {
private:
    // IP到统一ID的映射
    std::unordered_map<std::string, int> ip_to_unified_id;
    std::vector<std::string> unified_id_to_ip;
    
    // 统一节点信息
    std::vector<SubgraphNode> unified_nodes;
    
    // 并查集
    UnionFindExternal* uf = nullptr;
    
    // 引用CSRGraph对象
    const CSRGraph& graph;
    
    // 构建统一节点映射
    void buildUnifiedMapping();
    
    // 构建并查集
    void buildUnionFind();
    
public:
    // 构造函数
    SubgraphExtractor(const CSRGraph& g) : graph(g) {}
    
    // 析构函数
    ~SubgraphExtractor() {
        if (uf != nullptr) delete uf;
    }
    
    // 提取指定IP所在的子图
    SubgraphInfo extractSubgraph(const std::string& target_ip);
    
    // 获取所有连通分量
    std::vector<std::vector<std::string>> getAllComponents();
    
    // 获取连通分量信息
    std::string getComponentInfo(const std::string& target_ip);
    
    // 获取IP的统一ID
    int getUnifiedID(const std::string& ip) const;
    
    // 获取统一节点信息
    const SubgraphNode* getUnifiedNode(int unified_id) const;
    const SubgraphNode* getUnifiedNode(const std::string& ip) const;
    
    // 获取并查集
    UnionFindExternal* getUnionFind() { return uf; }
};

#endif // UNION_FIND_EXTERNAL_H