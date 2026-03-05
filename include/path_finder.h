#ifndef PATH_FINDER_H
#define PATH_FINDER_H

#include "handle_data.h" // 引入CSRGraph类的定义
#include <vector>
#include <string>

class PathFinder {
private:
    const CSRGraph& graph; // 引用CSRGraph对象
    
    
public:
    // 构造函数
    PathFinder(const CSRGraph& graph);

    // 按照拥塞程度查找两个节点之间的最短路径，拥塞程度可以简单的计算为节点之间的流量除以会话持续时间​
    std::vector<int> findShortestPath(const std::string& src_ip, const std::string& dst_ip);

    // 按照节点跳数寻找最小跳数路径
    std::vector<int> findShortestPathByHops(const std::string& src_ip, const std::string& dst_ip);

    // 打印路径信息 以IP1 -> IP2 -> ...... ->IPn的格式输出路径
    void printPath(const std::vector<int>& path);
};

#endif // PATH_FINDER_H