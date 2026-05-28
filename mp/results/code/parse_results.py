import os
import csv
import re

results_dir = 'lab5/results'
output_files = {
    'sequential': 'lab5/results_sequential.csv',
    'variant1': 'lab5/results_variant1.csv',
    'variant2': 'lab5/results_variant2.csv',
    'variant4': 'lab5/results_variant4.csv'
}

header = ['Iteration', 'Threads', 'Size', 'Buckets', 'Gen_Time', 'Split_Time', 'Sort_Time', 'Write_Time', 'Total_Time']

data = {k: [] for k in output_files.keys()}

# Regex to find time lines: ---; gen; split; sort; write; total
time_re = r'---; ([\d.]+); ([\d.]+); ([\d.]+); ([\d.]+); ([\d.]+)'

for filename in sorted(os.listdir(results_dir)):
    if not filename.startswith('result-') or not filename.endswith('.out'):
        continue
    
    iteration = filename.split('-')[1].split('.')[0]
    filepath = os.path.join(results_dir, filename)
    
    with open(filepath, 'r') as f:
        current_threads = "1"
        current_variant = None
        
        for line in f:
            # Detect Threads
            thread_match = re.search(r'=== THREADS: (\d+) ===', line)
            if thread_match:
                current_threads = thread_match.group(1)
                continue
                
            # Detect Sequential
            if '=== SEQUENTIAL ===' in line:
                current_threads = "1"
                current_variant = 'sequential'
                continue
                
            # Detect Variants
            if '--- Variant 1 ---' in line:
                current_variant = 'variant1'
                continue
            if '--- Variant 2 ---' in line:
                current_variant = 'variant2'
                continue
            if '--- Variant 4 ---' in line:
                current_variant = 'variant4'
                continue
            
            # Detect size/buckets for Sequential if variant not yet set
            if current_variant == 'sequential':
                meta_match = re.search(r'Times for: size: (\d+); buckets: (\d+)', line)
                if meta_match:
                    current_size, current_buckets = meta_match.groups()

            # Detect size/buckets/threads for Parallel
            if current_variant in ['variant1', 'variant2', 'variant4']:
                meta_match = re.search(r'Times for.*?: size: (\d+); threads: (\d+); buckets: (\d+)', line)
                if meta_match:
                    current_size, current_threads_inner, current_buckets = meta_match.groups()
                    current_threads = current_threads_inner

            # Match timing data
            if line.startswith('---;'):
                match = re.search(time_re, line)
                if match and current_variant:
                    gen, split, sort, write, total = match.groups()
                    data[current_variant].append([iteration, current_threads, current_size, current_buckets, gen, split, sort, write, total])

for key, filename in output_files.items():
    # Sort data by Threads (as int) then Iteration (as int)
    data[key].sort(key=lambda x: (int(x[1]), int(x[0])))
    with open(filename, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(header)
        writer.writerows(data[key])

print("CSVs generated successfully.")
