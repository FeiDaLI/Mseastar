import subprocess
import itertools
import os
import argparse

def run_udp_tests(output_csv):
    # Define parameter sets
    server_configs = [
        {'port': 10000, 'server_name': 'Mseastar'},
        # {'port': 41234, 'server_name': 'Node'}
    ]
    num_clients = [10, 20, 50, 100]
    num_packets = [1000, 10000]
    packet_sizes = [1024, 2048]
    server_ip = '127.0.0.1'

    # Generate all combinations
    total_tests = len(server_configs) * len(num_clients) * len(num_packets) * len(packet_sizes)
    print(f"Running {total_tests} tests, saving to {output_csv}...")

    test_count = 1
    for config, clients, packets, size in itertools.product(server_configs, num_clients, num_packets, packet_sizes):
        port = config['port']
        server_name = config['server_name']

        # Build command
        command = [
            'python', 'udp_tester.py',
            '--server_ip', server_ip,
            '--server_port', str(port),
            '--num_clients', str(clients),
            '--num_packets', str(packets),
            '--packet_size', str(size),
            '--output_csv', output_csv,
            '--server_name', server_name
        ]

        # Print test info
        print(f"\nTest {test_count}/{total_tests}: {server_name} (port {port})")
        print(f"  Clients: {clients}, Packets: {packets}, Packet Size: {size}, Output: {output_csv}")
        print(f"  Command: {' '.join(command)}")

        # Run the command
        try:
            result = subprocess.run(command, check=True, capture_output=True, text=True)
            print(f"  Success. Output:\n{result.stdout}")
        except subprocess.CalledProcessError as e:
            print(f"  Error running test: {e}")
            print(f"  Stderr: {e.stderr}")
        test_count += 1
    print(f"\nAll tests completed. Results saved to {output_csv}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run multiple UDP performance tests")
    parser.add_argument("--output_csv", type=str, default="all_results.csv", help="Single CSV file for all test results")
    args = parser.parse_args()

    # Verify udp_tester.py exists
    if not os.path.exists('udp_tester.py'):
        print("Error: udp_tester.py not found in the current directory.")
        exit(1)

    run_udp_tests(args.output_csv)