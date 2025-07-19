import subprocess
import csv

# Define thread counts to test.
thread_counts = [1, 2, 4, 8, 16]
# Define the programs to test.
programs = ["./cuckoo_seq", "./cuckoo_seq_v2", "./cuckoo_con", "./cuckoo_con_v2", "./cuckoo_trans"]

results = []

for prog in programs:
    for threads in thread_counts:
        result = subprocess.run([prog, str(threads)], capture_output=True, text=True)
        output = result.stdout.strip()
        # Look for the line with the average execution time.
        for line in output.splitlines():
            if "Average execution time" in line:
                parts = line.split(":")
                if len(parts) >= 2:
                    avg_time = parts[1].strip()
                    results.append((prog, threads, float(avg_time)))
                break

# Write the results to a CSV file.
with open("results.csv", "w", newline="") as csvfile:
    writer = csv.writer(csvfile)
    writer.writerow(["Program", "Threads", "AverageExecutionTime"])
    for prog, threads, avg_time in results:
        writer.writerow([prog, threads, avg_time])
        
print("Benchmark results saved to results.csv")