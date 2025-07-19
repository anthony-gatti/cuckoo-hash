#include <iostream>
#include <vector>
#include <random>
#include <atomic>
#include <shared_mutex>
#include <chrono>
#include <thread>
#include <optional>
#include <mutex>

constexpr size_t MAX_MIGRATIONS = 32;

size_t h1(int key, size_t capacity) {
    return std::hash<int>{}(key) % capacity;
}

size_t h2(int key, size_t capacity) {
    return (std::hash<int>{}(~key)) % capacity;
}

template<typename T>
struct Bucket {
    std::atomic<T> key;
    std::atomic<bool> valid;
    std::shared_mutex lock;
    Bucket() : key(0), valid(false) {}
};

template <typename T>
class CuckooHash {
private:
    std::vector<Bucket<T>> table1;
    std::vector<Bucket<T>> table2;
    std::atomic<size_t> count;
    size_t capacity;
    std::mutex resize_mutex;

public:
    CuckooHash(size_t num_buckets) : table1(num_buckets), table2(num_buckets), count(0), capacity(num_buckets) {}

    bool add(const T& key_input) {
        if(contains(key_input)) return false;
        T key = key_input;
        for (size_t attempt = 0; attempt < MAX_MIGRATIONS; ++attempt) {
            size_t i1 = h1(key, capacity);
            {
                std::unique_lock lock1(table1[i1].lock);
                if (!table1[i1].valid.load()) {
                    table1[i1].key.store(key);
                    table1[i1].valid.store(true);
                    count++;
                    return true;
                }
            }

            size_t i2 = h2(key, capacity);
            {
                std::unique_lock lock2(table2[i2].lock);
                if (!table2[i2].valid.load()) {
                    table2[i2].key.store(key);
                    table2[i2].valid.store(true);
                    count++;
                    return true;
                }
            }

            {
                std::unique_lock lock1(table1[i1].lock);
                T displaced = table1[i1].key.load();
                table1[i1].key.store(key);
                table1[i1].valid.store(true);
                key = displaced;
            }
        }
        resize();
        return add(key);
    }

    bool remove(const T& key) {
        if(!contains(key)) return false;
        size_t i1 = h1(key, capacity);
        {
            std::unique_lock lock1(table1[i1].lock);
            if (table1[i1].valid && table1[i1].key == key) {
                table1[i1].valid = false;
                count--;
                return true;
            }
        }

        size_t i2 = h2(key, capacity);
        {
            std::unique_lock lock2(table2[i2].lock);
            if (table2[i2].valid && table2[i2].key == key) {
                table2[i2].valid = false;
                count--;
                return true;
            }
        }

        return false;
    }

    bool contains(const T& key) {
        size_t i1 = h1(key, capacity);
        size_t i2 = h2(key, capacity);

        T k1 = table1[i1].key.load(std::memory_order_acquire);
        bool v1 = table1[i1].valid.load(std::memory_order_acquire);
        if (v1 && k1 == key) return true;

        T k2 = table2[i2].key.load(std::memory_order_acquire);
        bool v2 = table2[i2].valid.load(std::memory_order_acquire);
        return v2 && k2 == key;
    }

    void resize() {
        std::lock_guard<std::mutex> guard(resize_mutex);
    
        size_t old_capacity = capacity;
        capacity *= 2;
    
        std::vector<Bucket<T>> new_table1(capacity);
        std::vector<Bucket<T>> new_table2(capacity);
    
        for (size_t i = 0; i < old_capacity; ++i) {
            if (table1[i].valid.load()) {
                reinsert(table1[i].key.load(), new_table1, new_table2);
            }
            if (table2[i].valid.load()) {
                reinsert(table2[i].key.load(), new_table1, new_table2);
            }
        }
    
        table1 = std::move(new_table1);
        table2 = std::move(new_table2);
    }

    void reinsert(T key, std::vector<Bucket<T>>& t1, std::vector<Bucket<T>>& t2) {
        for (size_t attempt = 0; attempt < MAX_MIGRATIONS; ++attempt) {
            size_t i1 = h1(key, capacity);
            if (!t1[i1].valid.load()) {
                t1[i1].key.store(key);
                t1[i1].valid.store(true);
                return;
            }
    
            size_t i2 = h2(key, capacity);
            if (!t2[i2].valid.load()) {
                t2[i2].key.store(key);
                t2[i2].valid.store(true);
                return;
            }
    
            int temp = t1[i1].key.load();
            t1[i1].key.store(key);
            key = temp;
        }
    }

    size_t size() const {
        return count.load();
    }

    void populate(size_t n, int min = 0, int max = 1000) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(min, max);
        for (size_t i = 0; i < n; ++i) {
            add(static_cast<T>(dist(gen)));
        }
    }
};

int main(int argc, char* argv[]) {
    const size_t num_buckets = 1000;
    const size_t num_ops = 10000;

    size_t num_threads = 1;
    if (argc >= 2) {
        num_threads = std::stoul(argv[1]);
    }

    const size_t ops_per_thread = num_ops / num_threads;
    const int num_iter = 50;
    double total_time = 0.0;

    for (int i = 0; i < num_iter; i++) {
        CuckooHash<int> hashset(num_buckets);
        hashset.populate(100);

        std::vector<std::thread> threads;
        auto start = std::chrono::high_resolution_clock::now();
        for (size_t t = 0; t < num_threads; t++) {
            threads.emplace_back([&hashset, ops_per_thread]() {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> opDist(1, 100);
                std::uniform_int_distribution<> keyDist(0, 1000);
                for (size_t i = 0; i < ops_per_thread; i++) {
                    int op = opDist(gen);
                    int key = keyDist(gen);
                    if (op <= 80) {
                        hashset.contains(key);
                    } else if (op <= 90) {
                        hashset.add(key);
                    } else {
                        hashset.remove(key);
                    }
                }
            });
        }
        for (auto& th : threads) {
            th.join();
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::micro> duration = end - start;
        total_time += duration.count();
    }
    double avg_time = total_time / num_iter;
    std::cout << "Average execution time (microseconds): " << avg_time << std::endl;

    return 0;
}
