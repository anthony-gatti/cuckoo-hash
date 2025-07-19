#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <functional>

constexpr size_t MAX_MIGRATIONS = 32;

size_t h1(int key, size_t capacity) {
    return std::hash<int>{}(key) % capacity;
}

size_t h2(int key, size_t capacity) {
    return (std::hash<int>{}(~key)) % capacity;
}

struct Bucket {
    int key;
    bool valid;
    Bucket() : key(0), valid(false) {}
};

class CuckooHash {
private:
    std::vector<Bucket> table1;
    std::vector<Bucket> table2;
    size_t count;
    size_t capacity;

    void resize() {
        std::vector<Bucket> old1 = table1;
        std::vector<Bucket> old2 = table2;

        capacity *= 2;
        table1.clear(); table1.resize(capacity);
        table2.clear(); table2.resize(capacity);
        count = 0;

        for (const auto& b : old1) {
            if (b.valid) add(b.key);
        }
        for (const auto& b : old2) {
            if (b.valid) add(b.key);
        }
    }

public:
    CuckooHash(size_t num_buckets) : table1(num_buckets), table2(num_buckets), count(0), capacity(num_buckets) {}

    bool add(int key) {
        if (contains(key)) return false;
        for (size_t attempt = 0; attempt < MAX_MIGRATIONS; ++attempt) {
            size_t i1 = h1(key, capacity);
            if (!table1[i1].valid) {
                table1[i1].key = key;
                table1[i1].valid = true;
                count++;
                return true;
            }

            size_t i2 = h2(key, capacity);
            if (!table2[i2].valid) {
                table2[i2].key = key;
                table2[i2].valid = true;
                count++;
                return true;
            }

            std::swap(key, table1[i1].key);
        }

        resize();
        return add(key);
    }

    bool remove(int key) {
        if(!contains(key)) return false;
        size_t i1 = h1(key, capacity);
        if (table1[i1].valid && table1[i1].key == key) {
            table1[i1].valid = false;
            count--;
            return true;
        }

        size_t i2 = h2(key, capacity);
        if (table2[i2].valid && table2[i2].key == key) {
            table2[i2].valid = false;
            count--;
            return true;
        }

        return false;
    }

    bool contains(int key) const {
        size_t i1 = h1(key, capacity);
        size_t i2 = h2(key, capacity);
        return (table1[i1].valid && table1[i1].key == key) ||
               (table2[i2].valid && table2[i2].key == key);
    }

    size_t size() const {
        return count;
    }

    void populate(size_t n, int min = 0, int max = 1000) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(min, max);
        for (size_t i = 0; i < n; ++i) {
            add(dist(gen));
        }
    }
};

int main() {
    const size_t num_buckets = 1000;
    const size_t num_ops = 10000;
    
    const int num_iter = 50;
    double total_time = 0.0;

    for (int i = 0; i < num_iter; i++) {    
        CuckooHash hashset(num_buckets);
        hashset.populate(100);
        size_t expected_size = hashset.size();
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> opDist(1, 100);
        std::uniform_int_distribution<> keyDist(0, 1000);
                
        auto start = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < num_ops; i++) {
            int op = opDist(gen);
            int key = keyDist(gen);
            if (op <= 80) {
                hashset.contains(key);
            } else if (op <= 90) {
                bool added = hashset.add(key);
                if (added) {
                    expected_size++;
                }
            } else {
                bool removed = hashset.remove(key);
                if (removed) {
                    expected_size--;
                }
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::micro> duration = end - start;
        total_time += duration.count();
        
        // std::cout << "Expected size: " << expected_size << std::endl;
        // std::cout << "Actual size: " << hashSet.size() << std::endl;
    }
    double avg_time = total_time / num_iter;
    std::cout << "Average execution time (microseconds): " << avg_time << std::endl;
        
    return 0;
}