import csv
import matplotlib.pyplot as plt

# Dictionary to store results grouped by program.
data = {}

with open("results.csv", "r") as csvfile:
    reader = csv.DictReader(csvfile)
    for row in reader:
        prog = row["Program"]
        threads = float(row["Threads"])
        avg_time = float(row["AverageExecutionTime"])
        if prog not in data:
            data[prog] = {"threads": [], "times": []}
        data[prog]["threads"].append(threads)
        data[prog]["times"].append(avg_time)

plt.figure()
for prog, d in data.items():
    # Sort the data by thread count.
    sorted_data = sorted(zip(d["threads"], d["times"]), key=lambda x: x[0])
    threads_sorted, times_sorted = zip(*sorted_data)
    plt.plot(threads_sorted, times_sorted, marker="o", linestyle="-", label=prog)

plt.xscale("log")
plt.xlabel("Threads (log scale)")
plt.ylabel("Average Execution Time (microseconds)")
plt.title("Benchmark: Average Execution Time vs Threads")
plt.legend()
plt.grid(True)
plt.savefig("results_plot.png")
plt.show()