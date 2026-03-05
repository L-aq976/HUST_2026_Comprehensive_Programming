#include "sort_filter.h"

SortFilter::SortFilter(CSRGraph& graph) {
    // 构造函数可以在这里进行一些初始化操作，如果需要的话
    sortVerticesByTraffic(graph); // 在构造函数中调用排序方法，预先计算流量总和
    badsrc = findbadsrc(graph); // 在构造函数中调用筛选方法，预先计算单向流量占比>80%的节点
    sortedhttpssrc = findHTTPSsrc(graph); // 在构造函数中调用筛选方法，预先计算包含HTTPS连接的节点
}

SortFilter::SortFilter() {
    // 默认构造函数
    traffic_total.clear();
    badsrc.clear();
}
void SortFilter::sortVerticesByTraffic(CSRGraph& graph) {
    //1.设置一个数组统计src中每个顶点的流量
    std::vector<long long> traffic(graph.getNumVertices(), 0);
    for(int src_id = 0; src_id < graph.getNumVertices(); src_id++){
        //2.遍历src_id的出边，统计流量
        int count = graph.offsets[src_id + 1] - graph.offsets[src_id];
        for(int edge_id = graph.offsets[src_id]; edge_id < graph.offsets[src_id + 1]; edge_id++){
            traffic[src_id] += graph.edge_infos[edge_id].total_data_size;
        }
    }
    //3.设置一个数组统计des中每个顶点的流量
    std::vector<long long> traffic_des(graph.getNumDestVertices(), 0);
    int ptr_offset = 0;
    int ptr_edge = 0;
    while(ptr_offset < graph.getNumDestVertices()){
        int count = graph.offsets[ptr_offset + 1] - graph.offsets[ptr_offset];
        for(int i = 0; i < count; i++){
            traffic_des[graph.edges[ptr_edge]] += graph.edge_infos[ptr_edge].total_data_size;
            ptr_edge++;
        }
        ptr_offset++;
    }
    //4.将src和des的流量合并
    //先将src的流量合并到traffic_total
    traffic_total.resize(graph.getNumVertices());
    for(int i=0;i<graph.getNumVertices();i++){
        if(graph.dest_ip_to_id.find(graph.src_ip_list[i]) != graph.dest_ip_to_id.end()){
            traffic_total[i].first = traffic[i] + traffic_des[graph.dest_ip_to_id[graph.src_ip_list[i]]];
            traffic_total[i].second = graph.src_ip_list[i];
        }
        else{
            traffic_total[i].first = traffic[i];
            traffic_total[i].second = graph.src_ip_list[i];
        }
    }
    //继续追加des的流量，其中要是前面没有的IP才追加
    for(int i=0;i<graph.getNumDestVertices();i++){
        if(graph.src_ip_to_id.find(graph.dest_ip_list[i]) == graph.src_ip_to_id.end()){
            traffic_total.push_back(std::make_pair(traffic_des[i], graph.dest_ip_list[i]));
        }
    }
    //5.按照流量排序,时间复杂度O(nlogn)
    std::sort(traffic_total.begin(), traffic_total.end(), [](const auto& a, const auto& b) {    
        return a.first > b.first;  
    });
}

void SortFilter::printTopNVertices(const std::vector<std::pair<long long,std::string>>& sorted_vertices, int N) {
    std::cout << "\n=== 流量排名前 " << N << " 的顶点 ===" << std::endl;
    for (int i = 0; i < N && i < sorted_vertices.size(); i++) {
        std::cout << "IP: " << sorted_vertices[i].second 
                  << ", 总流量: " << sorted_vertices[i].first << " bytes" << std::endl;
    }

}

std::vector<std::pair<long long, std::string>> SortFilter::findbadsrc(CSRGraph& graph) {
    std::vector<std::pair<long long, std::string>> badsrc;
    if(traffic_total.empty()){
        sortVerticesByTraffic(graph);
    }
    for(int i=0;i<traffic_total.size();i++){
        //找到对应的IP地址在src中的ID
        int src_id = graph.getSrcVertexID(traffic_total[i].second);
        if(src_id != -1){
            long long out_traffic = 0;
            for(int edge_id = graph.offsets[src_id]; edge_id < graph.offsets[src_id + 1]; edge_id++){
                out_traffic += graph.edge_infos[edge_id].total_data_size;
            }
            if(out_traffic > 0.8 * traffic_total[i].first){
                badsrc.push_back(std::make_pair(traffic_total[i].first, traffic_total[i].second));
            }
        }
    }
    //按照流量排序
    std::sort(badsrc.begin(), badsrc.end(), [](const auto& a, const auto& b) {    
        return a.first > b.first;  
    });
    return badsrc;
}

void SortFilter::printBadSrc(const std::vector<std::pair<long long, std::string>>& badsrc) {
    std::cout << "\n=== 单向流量占比>80%的节点 ===" << std::endl;
    for (const auto& entry : badsrc) {
        std::cout << "IP: " << entry.second 
                  << ", 总流量: " << entry.first << " bytes" << std::endl;
    }
}

std::vector<std::pair<long long, std::string>> SortFilter::findHTTPSsrc(CSRGraph& graph) {
    std::vector<std::pair<long long, std::string>> httpssrc;
    for(int src_id = 0; src_id < graph.getNumVertices(); src_id++){
        bool has_https = false;
        long long total_traffic = 0;
        for(int edge_id = graph.offsets[src_id]; edge_id < graph.offsets[src_id + 1]; edge_id++){
            const EdgeInfo& edge_info = graph.edge_infos[edge_id];
            total_traffic += edge_info.total_data_size;
            if(edge_info.dst_port == 443 && edge_info.src_port != 443){
                has_https = true;
            }
        }
        if(has_https){
            httpssrc.push_back(std::make_pair(total_traffic, graph.src_ip_list[src_id]));
        }
    }
    //按照流量排序
    std::sort(httpssrc.begin(), httpssrc.end(), [](const auto& a, const auto& b) {    
        return a.first > b.first;  
    });
    return httpssrc;
}

void SortFilter::printHTTPSsrc(const std::vector<std::pair<long long, std::string>>& sortedhttpssrc) {
    if(sortedhttpssrc.empty()){
        std::cout << "\n=== 没有包含HTTPS连接的节点 ===" << std::endl;
        return;
    }
    std::cout << "\n=== 包含HTTPS连接的节点 ===" << std::endl;
    for (const auto& entry : sortedhttpssrc) {
        std::cout << "IP: " << entry.second 
                  << ", 总流量: " << entry.first << " bytes" << std::endl;
    }
}