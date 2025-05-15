import matplotlib.pyplot as plt
import pandas as pd

# Data
data = {
    'server_name': ['Node', 'Node', 'Node', 'Node', 'Node', 'Node', 'Node', 'Node', 'Node', 'Node', 'Node', 'Node', 'Node', 'Node', 'Node', 'Node', 'Mseastar', 'Mseastar', 'Mseastar', 'Mseastar', 'Mseastar', 'Mseastar', 'Mseastar', 'Mseastar', 'Mseastar', 'Mseastar', 'Mseastar', 'Mseastar', 'Mseastar', 'Mseastar', 'Mseastar', 'Mseastar'],
    'server_port': [41234, 41234, 41234, 41234, 41234, 41234, 41234, 41234, 41234, 41234, 41234, 41234, 41234, 41234, 41234, 41234, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000],
    'num_clients': [10, 10, 10, 10, 20, 20, 20, 20, 50, 50, 50, 50, 100, 100, 100, 100, 10, 10, 10, 10, 20, 20, 20, 20, 50, 50, 50, 50, 100, 100, 100, 100],
    'num_packets': [1000, 1000, 10000, 10000, 1000, 1000, 10000, 10000, 1000, 1000, 10000, 10000, 1000, 1000, 10000, 10000, 1000, 1000, 10000, 10000, 1000, 1000, 10000, 10000, 1000, 1000, 10000, 10000, 1000, 1000, 10000, 10000],
    'packet_size': [1024, 2048, 1024, 2048, 1024, 2048, 1024, 2048, 1024, 2048, 1024, 2048, 1024, 2048, 1024, 2048, 1024, 2048, 1024, 2048, 1024, 2048, 1024, 2048, 1024, 2048, 1024, 2048, 1024, 2048, 1024, 2048],
    'total_packets_sent': [10000, 10000, 100000, 100000, 20000, 20000, 200000, 200000, 50000, 50000, 500000, 500000, 100000, 100000, 1000000, 1000000, 10000, 10000, 100000, 100000, 20000, 20000, 200000, 200000, 50000, 50000, 500000, 500000, 100000, 100000, 1000000, 1000000],
    'total_errors': [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
    'success_rate': ['100.00%', '100.00%', '100.00%', '100.00%', '100.00%', '100.00%', '100.00%', '100.00%', '100.00%', '100.00%', '100.00%', '100.00%', '100.00%', '100.00%', '100.00%', '100.00%', '100.00%', '100.00%', '100.00%', '100.00%', '100.00%', '100.00%', '100.00%', '100.00%', '100.00%', '100.00%', '100.00%', '100.00%', '100.00%', '100.00%', '100.00%', '100.00%'],
    'throughput_mbps': [69.63, 138.23, 70.21, 139.08, 146.01, 290.83, 146.27, 290.74, 376.33, 752.29, 379.06, 756.52, 557.45, 1141.67, 571.49, 1160.8, 75.9, 151.62, 75.92, 152.15, 151.17, 302.73, 151.45, 300.96, 362.48, 732.55, 374.99, 739.35, 452.15, 914.47, 473.31, 925.32],
    'duration_seconds': [1.18, 1.19, 11.67, 11.78, 1.12, 1.13, 11.2, 11.27, 1.09, 1.09, 10.81, 10.83, 1.47, 1.44, 14.33, 14.11, 1.08, 1.08, 10.79, 10.77, 1.08, 1.08, 10.82, 10.89, 1.13, 1.12, 10.92, 11.08, 1.81, 1.79, 17.31, 17.71]
}
df = pd.DataFrame(data)

# Set IEEE style: Times New Roman, compact size
plt.rcParams['font.family'] = 'Times New Roman'
plt.rcParams['font.size'] = 8
plt.rcParams['axes.titlesize'] = 8
plt.rcParams['axes.labelsize'] = 8
plt.rcParams['xtick.labelsize'] = 8
plt.rcParams['ytick.labelsize'] = 8
plt.rcParams['legend.fontsize'] = 8

# Create figure with IEEE half-column width (3.5 inches)
fig, ax = plt.subplots(figsize=(3.5, 2.5))

# Group data by server_name, num_clients, packet_size, averaging throughput
grouped = df.groupby(['server_name', 'num_clients', 'packet_size'])['throughput_mbps'].mean().reset_index()

# Create a pivot table to ensure proper alignment of bars
pivot = grouped.pivot_table(
    values='throughput_mbps',
    index=['num_clients', 'server_name'],
    columns='packet_size'
).reset_index()

# Plot bars manually to control colors and hatches
bar_width = 0.2
clients = pivot['num_clients'].unique()
x = range(len(clients))

# Define vibrant colors for Node and Mseastar
colors = {
    ('Node', 1024): '#1f77b4',    # Blue for Node 1024
    ('Node', 2048): '#4a90e2',    # Light blue for Node 2048
    ('Mseastar', 1024): '#d62728', # Red for Mseastar 1024
    ('Mseastar', 2048): '#ff7f0e'  # Orange for Mseastar 2048
}
hatches = {'Node': '', 'Mseastar': '//'}

# Plot bars for each combination
for i, server in enumerate(['Node', 'Mseastar']):
    for j, packet_size in enumerate([1024, 2048]):
        subset = pivot[pivot['server_name'] == server]
        ax.bar(
            [pos + (i * 2 + j) * bar_width for pos in x],
            subset[packet_size],
            width=bar_width,
            color=colors[(server, packet_size)],
            hatch=hatches[server],
            label=f'{server}, {packet_size} Bytes'
        )

# Customize plot
ax.set_xlabel('Number of Clients')
ax.set_ylabel('Throughput (Mbps)')
ax.set_title('Throughput: Node vs. Mseastar')
ax.set_xticks([pos + 1.5 * bar_width for pos in x])
ax.set_xticklabels(clients)
ax.grid(True, axis='y', linestyle='--', alpha=0.7)
ax.legend(title='Server and Packet Size', loc='upper left', frameon=True)

# Tight layout for compact spacing
plt.tight_layout()

# Save as high-resolution PNG
plt.savefig('throughput_comparison.png', dpi=300, bbox_inches='tight')