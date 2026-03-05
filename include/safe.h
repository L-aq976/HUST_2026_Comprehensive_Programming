#ifndef SAFE_H
#define SAFE_H
#include "handle_data.h"
class Safe {
    private:
        const CSRGraph& graph; // 引用CSRGraph对象
    public:
        // 构造函数
        Safe(const CSRGraph& graph);

        //该功能输入3个IP地址，分别是地址1，地址2，地址3。
        //其中地址2和地址3构成一个地址范围，表示地址1禁止\允许与该范围内的地址建立会话

        void analyzeIPRange(const std::string& ip1, const std::string& ip2, const std::string& ip3);
};
#endif // SAFE_H
