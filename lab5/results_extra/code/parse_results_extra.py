import os
import csv
import re

results_dir = "lab5/results_extra"
output_files = {
    "variant2": "lab5/results_extra_variant2.csv",
    "variant2aligned": "lab5/results_extra_variant2_aligned.csv",
    "variant4": "lab5/results_extra_variant4.csv",
    "variant4aligned": "lab5/results_extra_variant4_aligned.csv",
}

header = [
    "Iteration",
    "Threads",
    "Size",
    "Buckets",
    "Gen_Time",
    "Split_Time",
    "Sort_Time",
    "Write_Time",
    "Total_Time",
]

data = {k: [] for k in output_files.keys()}

# Regex to find time lines: ---; gen; split; sort; write; total
time_re = r"---; ([\d.]+); ([\d.]+); ([\d.]+); ([\d.]+); ([\d.]+)"

for filename in sorted(os.listdir(results_dir)):
    if not filename.startswith("result_extra-") or not filename.endswith(".out"):
        continue

    iteration = filename.split("-")[1].split(".")[0]
    filepath = os.path.join(results_dir, filename)

    with open(filepath, "r") as f:
        current_threads = "1"
        current_variant = None

        for line in f:
            # Detect Threads
            thread_match = re.search(r"=== THREADS: (\d+) ===", line)
            if thread_match:
                current_threads = thread_match.group(1)
                continue

            # Detect Variant 2
            if "--- Variant 2 ---" in line:
                current_variant = "variant2"
                continue

            # Detect Variant 2 Aligned
            if "--- Variant 2 ALIGNED ---" in line:
                current_variant = "variant2aligned"
                continue
            if "--- Variant 4 ---" in line:
                current_variant = "variant4"
                continue
            if "--- Variant 4 ALIGNED ---" in line:
                current_variant = "variant4aligned"
                continue

            meta_match = re.search(
                r"Times for.*?: size: (\d+); threads: (\d+); buckets: (\d+)", line
            )
            if meta_match:
                current_size, current_threads_inner, current_buckets = (
                    meta_match.groups()
                )
                current_threads = current_threads_inner

            # Match timing data
            if line.startswith("---;"):
                match = re.search(time_re, line)
                if match and current_variant:
                    gen, split, sort, write, total = match.groups()
                    data[current_variant].append(
                        [
                            iteration,
                            current_threads,
                            current_size,
                            current_buckets,
                            gen,
                            split,
                            sort,
                            write,
                            total,
                        ]
                    )

for key, filename in output_files.items():
    # Sort data by Threads (as int) then Iteration (as int)
    data[key].sort(key=lambda x: (int(x[1]), int(x[0])))
    with open(filename, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(header)
        writer.writerows(data[key])

print("CSVs generated successfully.")
