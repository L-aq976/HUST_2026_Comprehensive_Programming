#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
PCAP转CSV转换器
自动将data目录中的pcap文件转换为与项目相同格式的CSV文件
"""

import os
import sys
import csv
import glob
from datetime import datetime
import argparse

try:
    from scapy.all import rdpcap
    from scapy.layers.inet import IP, TCP, UDP, ICMP
except ImportError:
    print("错误: 需要安装scapy库，请运行: pip install scapy")
    sys.exit(1)

def get_csv_header():
    """获取CSV文件的表头格式"""
    return ["Source", "Destination", "Protocol", "SrcPort", "DstPort", "DataSize", "Duration"]

def protocol_to_number(protocol_name):
    """将协议名称转换为数字表示"""
    protocol_map = {
        'TCP': 6,
        'UDP': 17,
        'ICMP': 1,
        'HTTP': 6,  # HTTP基于TCP
        'HTTPS': 6,  # HTTPS基于TCP
        'DNS': 17,   # DNS通常基于UDP
        'FTP': 6,
        'SSH': 6,
        'TELNET': 6
    }
    return protocol_map.get(protocol_name.upper(), 0)

def extract_packet_info(packet):
    """从pcap包中提取所需信息"""
    try:
        # 检查是否为IP包
        if not packet.haslayer(IP):
            return None  # 非IP包跳过
        
        ip_layer = packet[IP]
        src_ip = ip_layer.src
        dst_ip = ip_layer.dst
        
        # 获取协议信息
        protocol_num = 0
        src_port = '0'
        dst_port = '0'
        
        if packet.haslayer(TCP):
            protocol_num = 6  # TCP
            tcp_layer = packet[TCP]
            src_port = str(tcp_layer.sport)
            dst_port = str(tcp_layer.dport)
        elif packet.haslayer(UDP):
            protocol_num = 17  # UDP
            udp_layer = packet[UDP]
            src_port = str(udp_layer.sport)
            dst_port = str(udp_layer.dport)
        elif packet.haslayer(ICMP):
            protocol_num = 1  # ICMP
        else:
            return None  # 不支持的协议
        
        # 获取数据大小
        data_size = len(packet)
        
        # 获取时间戳
        timestamp = float(packet.time)
        
        # 转换为CSV格式
        return {
            'Source': src_ip,
            'Destination': dst_ip,
            'Protocol': protocol_num,
            'SrcPort': src_port,
            'DstPort': dst_port,
            'DataSize': data_size,
            'Timestamp': timestamp
        }
        
    except Exception:
        return None

def get_bidirectional_session_key(packet_info):
    """构造双向会话键，使A->B与B->A归并为同一条会话"""
    endpoint_a = (packet_info['Source'], packet_info['SrcPort'])
    endpoint_b = (packet_info['Destination'], packet_info['DstPort'])

    if endpoint_a <= endpoint_b:
        src_ep, dst_ep = endpoint_a, endpoint_b
    else:
        src_ep, dst_ep = endpoint_b, endpoint_a

    return (
        src_ep[0],
        dst_ep[0],
        packet_info['Protocol'],
        src_ep[1],
        dst_ep[1]
    )

def process_pcap_file(pcap_file_path, csv_file_path):
    """处理单个pcap文件并转换为CSV"""
    print(f"正在处理: {pcap_file_path}")

    # 按双向会话聚合并计算会话持续时间
    flows = {}
    
    try:
        # 使用scapy读取pcap文件
        packets = rdpcap(pcap_file_path)

        for packet in packets:
            packet_info = extract_packet_info(packet)
            if packet_info:
                flow_key = get_bidirectional_session_key(packet_info)

                if flow_key not in flows:
                    flows[flow_key] = {
                        # 使用标准化端点顺序输出，避免同一会话方向不一致
                        'source': flow_key[0],
                        'destination': flow_key[1],
                        'protocol': flow_key[2],
                        'src_port': flow_key[3],
                        'dst_port': flow_key[4],
                        'total_data_size': 0,
                        'first_timestamp': packet_info['Timestamp'],
                        'last_timestamp': packet_info['Timestamp'],
                        'packet_count': 0,
                    }

                flow = flows[flow_key]
                flow['total_data_size'] += packet_info['DataSize']
                flow['packet_count'] += 1
                if packet_info['Timestamp'] < flow['first_timestamp']:
                    flow['first_timestamp'] = packet_info['Timestamp']
                if packet_info['Timestamp'] > flow['last_timestamp']:
                    flow['last_timestamp'] = packet_info['Timestamp']

        if not flows:
            print(f"警告: {pcap_file_path} 中没有找到有效的IP包")
            return False

        # 写入CSV文件
        with open(csv_file_path, 'w', newline='', encoding='utf-8') as csvfile:
            writer = csv.writer(csvfile)
            writer.writerow(get_csv_header())

            for flow in flows.values():
                duration = flow['last_timestamp'] - flow['first_timestamp']
                row = [
                    flow['source'],
                    flow['destination'],
                    flow['protocol'],
                    flow['src_port'],
                    flow['dst_port'],
                    flow['total_data_size'],
                    f"{duration:.3f}"  # 每条流的持续时间
                ]
                writer.writerow(row)

        total_packets = sum(item['packet_count'] for item in flows.values())
        print(f"成功转换: {pcap_file_path} -> {csv_file_path}")
        print(f"转换了 {total_packets} 个数据包，聚合为 {len(flows)} 条流")
        return True
        
    except Exception as e:
        print(f"处理文件 {pcap_file_path} 时出错: {str(e)}")
        return False

def find_pcap_files(data_dir):
    """在data目录中查找pcap文件"""
    pcap_patterns = [
        os.path.join(data_dir, "*.pcap"),
        os.path.join(data_dir, "*.pcapng"),
        os.path.join(data_dir, "*.cap")
    ]
    
    pcap_files = []
    for pattern in pcap_patterns:
        pcap_files.extend(glob.glob(pattern))
    
    return pcap_files

def create_data_directory():
    """创建data目录（如果不存在）"""
    data_dir = os.path.join(os.path.dirname(__file__), "data")
    if not os.path.exists(data_dir):
        os.makedirs(data_dir)
        print(f"创建目录: {data_dir}")
    return data_dir

def main():
    """主函数"""
    parser = argparse.ArgumentParser(description='PCAP转CSV转换器')
    parser.add_argument('--data-dir', default='data', help='数据目录路径（默认: data）')
    parser.add_argument('--force', action='store_true', help='强制重新转换已存在的CSV文件')
    args = parser.parse_args()
    
    # 创建或确认data目录存在
    data_dir = create_data_directory()
    
    # 查找pcap文件
    pcap_files = find_pcap_files(data_dir)
    
    if not pcap_files:
        print(f"在目录 {data_dir} 中没有找到pcap文件")
        print("支持的文件格式: .pcap, .pcapng, .cap")
        return
    
    print(f"找到 {len(pcap_files)} 个pcap文件")
    
    # 处理每个pcap文件
    successful_conversions = 0
    
    for pcap_file in pcap_files:
        # 生成对应的CSV文件名
        base_name = os.path.splitext(os.path.basename(pcap_file))[0]
        csv_file = os.path.join(data_dir, f"{base_name}.csv")
        
        # 检查是否已存在CSV文件
        if os.path.exists(csv_file) and not args.force:
            print(f"跳过已存在的文件: {csv_file} (使用 --force 重新转换)")
            continue
        
        # 转换文件
        if process_pcap_file(pcap_file, csv_file):
            successful_conversions += 1
    
    print(f"\n转换完成: {successful_conversions}/{len(pcap_files)} 个文件成功转换")

if __name__ == "__main__":
    main()