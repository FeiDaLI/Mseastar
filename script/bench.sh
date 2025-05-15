

server_port = (10000,41234)
num_clients = (10,20,....)
num_packets = (1000,....)
packet_size = (1024,....)
csv = (Mseastar.csv,node.csv)

python udp_tester.py --server_ip 127.0.0.1 --server_port {} --num_clients {} --num_packets {} --packet_size {} --output_csv {}.csv
