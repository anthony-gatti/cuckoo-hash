#include <iostream>
#include <vector>
#include <optional>
#include <functional>
#include <random>
#include <cassert>
#include <chrono>
#include <cstdlib>

template<typename T>
struct Bucket {
    std::optional<T> value;
    int state;
    Bucket() : value(std::nullopt), state(0) {}
};

template<typename T>
class CuckooHash {
private:
    std::vector<Bucket<T>> buckets;
    size_t count;
    size_t capacity;
    double threshold;

    // doubles capacity and reinserts elements
    void rehash() {
        capacity *= 2;
        std::vector<Bucket<T>> oldBuckets = buckets;
        buckets.clear();
        buckets.resize(capacity);
        count = 0;
        for (auto& bucket : oldBuckets) {
            if (bucket.state == 1) {
                add(*(bucket.value));
            }
        }
    }

public:
    CuckooHash(size_t num_buckets = 101, double lf = 0.5)
        : count(0), capacity(num_buckets), threshold(lf) {
        buckets.resize(capacity);
    }
    
    bool add(const T& key) {
        if (contains(key)) {
            return false;
        }

        if (static_cast<double>(count + 1) / capacity > threshold) {
            rehash();
        }

        size_t index = std::hash<T>{}(key) % capacity;

        while (true) { // find the next available bucket
            if (buckets[index].state == 0 || buckets[index].state == -1) {
                buckets[index].value = key;
                buckets[index].state = 1;
                count++;
                return true;
            }
            index = (index + 1) % capacity;
        }

        return false;
    }
    
    bool remove(const T& key) {
        size_t index = std::hash<T>{}(key) % capacity;
        size_t start = index;

        while (buckets[index].state != 0) { // scan to find element
            if (buckets[index].state == 1 && *(buckets[index].value) == key) {
                buckets[index].state = -1;
                count--;
                return true;
            }
            index = (index + 1) % capacity;
            if (index == start) break; // element not found
        }
        return false;
    }
    
    bool contains(const T& key) const {
        size_t index = std::hash<T>{}(key) % capacity;
        size_t start = index;

        while (buckets[index].state != 0) {
            if (buckets[index].state == 1 && *(buckets[index].value) == key) {
                return true;
            }
            index = (index + 1) % capacity;
            if (index == start) break;
        }
        return false;
    }
    
    size_t size() const {
        return count;
    }
    
    void populate(size_t n, int min = 0, int max = 1000) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(min, max);
        for (size_t i = 0; i < n; i++) {
            add(static_cast<T>(dist(gen)));
        }
    }
};

int main() {
    const size_t num_buckets = 1000;
    const size_t num_ops = 10000;
    
    const int num_iter = 50;
    double total_time = 0.0;

    for (int i = 0; i < num_iter; i++) {    
        CuckooHash<int> hashset(num_buckets);
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