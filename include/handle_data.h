#ifndef HANDLE_DATA_H
#define HANDLE_DATA_H

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

// 流记录结构体
struct FlowRecord {
    std::string source_ip;
    std::string dest_ip;
    std::string protocol;
    int src_port;
    int dst_port;
    int data_size;
    float duration;
};

// 边信息结构体
struct EdgeInfo {
    int flow_count;           // 相同源目的对的流数量
    long long total_data_size;  // 总数据量
    float total_duration;     // 总时长
    int src_port; // 源端口
    int dst_port; // 目的端口
    
    EdgeInfo();  // 声明默认构造函数
    
    EdgeInfo(int count, int size, float dur, int src_port, int dst_port);  // 声明带参数构造函数
};

// CSR图结构
class CSRGraph {
private:
    // 核心CSR数组
    std::vector<int> offsets;          // 顶点偏移数组，长度 = 顶点数 + 1
    std::vector<int> edges;            // 边数组，存储目标顶点ID
    std::vector<EdgeInfo> edge_infos;  // 边属性数组，与edges一一对应
    
    // 顶点映射
    std::vector<std::string> src_ip_list;  
    std::unordered_map<std::string, int> src_ip_to_id;  
    std::vector<std::string> dest_ip_list;  
    std::unordered_map<std::string, int> dest_ip_to_id;  
    
    // 获取或创建顶点ID
    int src_get_or_createID(const std::string& ip);
    
    // 获取或创建目标顶点ID
    int dest_get_or_createID(const std::string& ip);

    // 解析CSV文件
    std::vector<FlowRecord> parseCSV(const std::string& filename);

    friend class SortFilter; // 声明SortFilter为友元类，可以访问私有成员
    
public:
    // 从CSV文件构建CSR
    bool buildFromCSV(const std::string& filename);
    
    // 获取CSR数组的访问方法
    const std::vector<int>& getOffsets() const;
    const std::vector<int>& getEdges() const;
    const std::vector<EdgeInfo>& getEdgeInfos() const;
    
    // 获取源IP相关方法
    const std::vector<std::string>& getSrcIPList() const;
    const std::unordered_map<std::string, int>& getSrcIPToIDMap() const;
    
    // 获取目的IP相关方法
    const std::vector<std::string>& getDestIPList() const;
    const std::unordered_map<std::string, int>& getDestIPToIDMap() const;

    // 获取顶点数（源IP数量）
    int getNumVertices() const;

    //获取目的IP数量
    int getNumDestVertices() const;
    
    // 获取边数
    int getNumEdges() const;
    
    // 获取源IP对应的顶点ID
    int getSrcVertexID(const std::string& ip) const;
    
    // 获取目的IP对应的顶点ID
    int getDestVertexID(const std::string& ip) const;
    
    // 获取顶点ID对应的源IP
    std::string getSrcIP(int vertex_id) const;
    
    // 获取顶点ID对应的目的IP
    std::string getDestIP(int vertex_id) const;
    
    // 获取顶点i的邻居数量
    int getNeighborCount(int vertex_id) const;
    
    // 获取顶点i的第j个邻居的信息
    bool getNeighborInfo(int vertex_id, int neighbor_index, 
                        int& dst_id, EdgeInfo& edge_info) const;

    //获取目的ip对应的顶点ID数目
    int getDestVertexIDCount(int dest_vertex_id) const;
    
    // 打印图的基本信息
    void printGraphInfo() const;

public:
    //找到星型结构 定义：如果中心节点和20个或更多的节点相连，且这些节点只和中心节点建立链接，这样的结构认为是星型结构。
    //vector存储中心节点的ID
    std::vector<int> findStarStructures();

    void printfStarStructures(const std::vector<int>& star_centers);
};

#endif // HANDLE_DATA_H