// ①要求筛选出包含HTTPS连接（HTTPS 是基于 TCP 协议 的加密 HTTP 通信。
// 协议类型（Protocol）必须为 6（代表 TCP 协议）；目标端口（DstPort）必须为 443）的节点，
// 并对这些节点进行按照流量大小进行排序，这样我们可以专注于HTTPS的相关的流量，从而忽略其他协议的影响。
// ②要求筛选单向流量占比>80%的节点，
// 该节点的发出流量占该节点总流量的80%以上的节点，因为这类节点很可能是在进行端口扫描操作，可能是恶意节点。
// 并对这些节点按照流量大小进行排序，并输出{总流量，发出流量占比}。
#ifndef SORT_FILTER_H
#define SORT_FILTER_H

#include "handle_data.h"

class SortFilter {
public:
    std::vector<std::pair<long long, std::string>> traffic_total; // 存储每个顶点的总流量和对应的IP地址
    std::vector<std::pair<long long, std::string>> badsrc;        // 存储单向流量占比>80%的节点的总流量和对应的IP地址
    std::vector<std::pair<long long, std::string>> sortedhttpssrc;      // 存储包含HTTPS连接的节点的总流量和对应的IP地址
    // 构造函数
    SortFilter();
    SortFilter(CSRGraph& graph);

    void sortVerticesByTraffic(CSRGraph& graph);

    void printTopNVertices(const std::vector<std::pair<long long, std::string>>& sorted_vertices, int N);

    std::vector<std::pair<long long, std::string>> findbadsrc(CSRGraph& graph); // 筛选单向流量占比>80%的节点，并返回这些节点的总流量和发出流量占比

    void printBadSrc(const std::vector<std::pair<long long, std::string>>& badsrc); // 打印单向流量占比>80%的节点的总流量和发出流量占比

    std::vector<std::pair<long long, std::string>> findHTTPSsrc(CSRGraph& graph); // 筛选包含HTTPS连接的节点，并返回这些节点的总流量和对应的IP地址

    void printHTTPSsrc(const std::vector<std::pair<long long, std::string>>& sortedhttpssrc); // 打印包含HTTPS连接的节点的总流量和对应的IP地址
};

#endif // SORT_FILTER_H