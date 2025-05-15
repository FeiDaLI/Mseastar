import socket
import time
import threading
import csv
import argparse
import os
from datetime import datetime
import queue

class UDPClientTest:
    def __init__(self, server_ip, server_port, num_clients, num_packets, packet_size, output_csv, server_name):
        self.server_address = (server_ip, server_port)
        self.num_clients = num_clients
        self.num_packets = num_packets
        self.packet_size = packet_size
        self.output_csv = output_csv
        self.server_name = server_name
        self.sent_counts = [0] * num_clients
        self.errors = [0] * num_clients
        self.start_time = 0
        self.end_time = 0
        self.lock = threading.Lock()

    def udp_client(self, client_id):
        """单个 UDP 客户端发送数据包并记录结果"""
        client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            for i in range(self.num_packets):
                message = os.urandom(self.packet_size)
                try:
                    client_socket.sendto(message, self.server_address)
                    with self.lock:
                        self.sent_counts[client_id - 1] += 1
                except Exception as e:
                    with self.lock:
                        self.errors[client_id - 1] += 1
                    print(f"Client {client_id} packet {i + 1} error: {e}")
                time.sleep(0.001)
        except Exception as e:
            print(f"Client {client_id} fatal error: {e}")
        finally:
            client_socket.close()
            print(f"Client {client_id} finished. Sent: {self.sent_counts[client_id - 1]} packets.")

    def write_results(self):
        """将测试结果追加到 CSV 文件"""
        file_exists = os.path.isfile(self.output_csv)
        with open(self.output_csv, 'a', newline='') as csvfile:
            fieldnames = [
                'timestamp', 'server_name', 'server_port', 'num_clients', 'num_packets', 'packet_size',
                'total_packets_sent', 'total_errors', 'success_rate', 'throughput_mbps', 'duration_seconds'
            ]
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

            if not file_exists:
                writer.writeheader()

            total_sent = sum(self.sent_counts)
            total_errors = sum(self.errors)
            expected_packets = self.num_clients * self.num_packets
            success_rate = (total_sent / expected_packets * 100) if expected_packets > 0 else 0
            duration = self.end_time - self.start_time if self.end_time > self.start_time else 0
            throughput_mbps = (total_sent * self.packet_size * 8 / 1_000_000 / duration) if duration > 0 else 0

            writer.writerow({
                'timestamp': datetime.now().isoformat(),
                'server_name': self.server_name,
                'server_port': self.server_address[1],
                'num_clients': self.num_clients,
                'num_packets': self.num_packets,
                'packet_size': self.packet_size,
                'total_packets_sent': total_sent,
                'total_errors': total_errors,
                'success_rate': f"{success_rate:.2f}%",
                'throughput_mbps': f"{throughput_mbps:.2f}",
                'duration_seconds': f"{duration:.2f}"
            })

    def run(self):
        """运行测试"""
        threads = []
        print(f"Starting {self.num_clients} clients to send {self.num_packets} packets "
              f"of size {self.packet_size} bytes to {self.server_address} ({self.server_name})")

        self.start_time = time.time()

        for i in range(self.num_clients):
            client_thread = threading.Thread(
                target=self.udp_client,
                args=(i + 1,)
            )
            threads.append(client_thread)
            client_thread.start()

        for thread in threads:
            thread.join()

        self.end_time = time.time()

        self.write_results()

        print(f"\nTest completed. Total time: {self.end_time - self.start_time:.2f} seconds.")
        print(f"Results appended to {self.output_csv}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="UDP performance testing client")
    parser.add_argument("--server_ip", type=str, default="127.0.0.1", help="Server IP address")
    parser.add_argument("--server_port", type=int, default=10000, help="Server port")
    parser.add_argument("--num_clients", type=int, default=1, help="Number of concurrent clients")
    parser.add_argument("--num_packets", type=int, default=1000, help="Number of packets per client")
    parser.add_argument("--packet_size", type=int, default=1024, help="Packet size in bytes")
    parser.add_argument("--output_csv", type=str, default="udp_test_results.csv", help="Output CSV filename")
    parser.add_argument("--server_name", type=str, default="Unknown", help="Server name (e.g., Mseastar, Node)")
    args = parser.parse_args()

    if args.num_clients < 1 or args.num_packets < 1 or args.packet_size < 1:
        print("Error: num_clients, num_packets, and packet_size must be positive.")
        exit(1)

    test = UDPClientTest(
        args.server_ip, args.server_port, args.num_clients,
        args.num_packets, args.packet_size, args.output_csv, args.server_name
    )
    test.run()