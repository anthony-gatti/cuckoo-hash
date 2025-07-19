#include <iostream>
#include <vector>
#include <optional>
#include <functional>
#include <random>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <mutex>
#include <thread>

template<typename T>
struct Bucket { // states: 0 = empty, 1 = occupied, -1 = deleted
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
    
    size_t num_stripes;
    size_t stripe_size;
    mutable std::vector<std::mutex> locks; // mutable allows const methods to lock
    
    // doubles capacity and reinserts all elements
    void rehash() {
        // acquire all stripe locks
        lockAllStripes();
        capacity *= 2;
        stripe_size = (capacity + num_stripes - 1) / num_stripes;
        std::vector<Bucket<T>> oldBuckets = buckets;
        buckets.clear();
        buckets.resize(capacity);
        count = 0;

        for (auto& bucket : oldBuckets) {
            if (bucket.state == 1) {
                rehashAdd(*(bucket.value));
            }
        }
        unlockAllStripes();
    }
    
    // helper to acquire all stripe locks
    void lockAllStripes() const { 
        for (size_t i = 0; i < num_stripes; i++) {
            locks[i].lock();
        }
    }
    
    // helper to release all stripe locks
    void unlockAllStripes() const { 
        for (size_t i = 0; i < num_stripes; i++) {
            locks[i].unlock();
        }
    }

    void rehashAdd(const T& key) {
        size_t index = std::hash<T>{}(key) % capacity;
        size_t start = index;
        while (buckets[index].state != 0) {
            index = (index + 1) % capacity;
            if (index == start) break;
        }
        buckets[index].value = key;
        buckets[index].state = 1;
        count++;
    }
    
public:
    CuckooHash(size_t num_buckets = 101, double lf = 0.5, size_t stripes = 8)
        : count(0), capacity(num_buckets), threshold(lf), num_stripes(stripes) {
        buckets.resize(capacity);
        locks = std::vector<std::mutex>(num_stripes);
        stripe_size = (capacity + num_stripes - 1) / num_stripes;
    }
    
    bool add(const T& key) {
        size_t index = std::hash<T>{}(key) % capacity;
        size_t stripe = index / stripe_size;
        bool result = false;
        {
            std::unique_lock<std::mutex> lock(locks[stripe]);
            size_t start = index;
            while (buckets[index].state != 0) {
                if (buckets[index].state == 1 && *(buckets[index].value) == key) {
                    return false; 
                }
                index = (index + 1) % capacity;
                if (index / stripe_size != stripe) { // moved into new stripe, fallback
                    lock.unlock();
                    lockAllStripes();
                    index = std::hash<T>{}(key) % capacity;
                    size_t start_all = index;
                    while (buckets[index].state != 0) {
                        if (buckets[index].state == 1 && *(buckets[index].value) == key) {
                            unlockAllStripes();
                            return false;
                        }
                        index = (index + 1) % capacity;
                        if (index == start_all) break;
                    }
                    buckets[index].value = key;
                    buckets[index].state = 1;
                    count++;
                    unlockAllStripes();
                    if (static_cast<double>(count) / capacity > threshold) {
                        rehash();
                    }
                    return true;
                }
                if (index == start) break;
            }

            buckets[index].value = key;
            buckets[index].state = 1;
            count++;
            result = true;
        } 
        if (static_cast<double>(count) / capacity > threshold) {
            rehash();
        }
        return result; 
    }
    
    bool remove(const T& key) {
        size_t index = std::hash<T>{}(key) % capacity;
        size_t stripe = index / stripe_size;
        {
            std::unique_lock<std::mutex> lock(locks[stripe]);
            size_t start = index;
            while (buckets[index].state != 0) {
                if (buckets[index].state == 1 && *(buckets[index].value) == key) {
                    buckets[index].state = -1; // mark as deleted.
                    count--;
                    return true;
                }
                index = (index + 1) % capacity;
                if (index / stripe_size != stripe) { // moved into new stripe, fallback
                    lock.unlock();
                    lockAllStripes();
                    index = std::hash<T>{}(key) % capacity;
                    size_t start_all = index;
                    bool found = false;
                    while (buckets[index].state != 0) {
                        if (buckets[index].state == 1 && *(buckets[index].value) == key) {
                            buckets[index].state = -1;
                            count--;
                            found = true;
                            break;
                        }
                        index = (index + 1) % capacity;
                        if (index == start_all) break;
                    }
                    unlockAllStripes();
                    return found;
                }
                if (index == start) break;
            }
            return false;
        }
    }
    
    bool contains(const T& key) const {
        size_t index = std::hash<T>{}(key) % capacity;
        size_t stripe = index / stripe_size;
        {
            std::unique_lock<std::mutex> lock(const_cast<std::mutex&>(locks[stripe]));
            size_t start = index;
            while (buckets[index].state != 0) {
                if (buckets[index].state == 1 && *(buckets[index].value) == key) {
                    return true;
                }
                index = (index + 1) % capacity;
                if (index / stripe_size != stripe) { // moved into new stripe, fallback
                    lock.unlock();
                    lockAllStripes();
                    index = std::hash<T>{}(key) % capacity;
                    size_t start_all = index;
                    bool found = false;
                    while (buckets[index].state != 0) {
                        if (buckets[index].state == 1 && *(buckets[index].value) == key) {
                            found = true;
                            break;
                        }
                        index = (index + 1) % capacity;
                        if (index == start_all) break;
                    }
                    unlockAllStripes();
                    return found;
                }
                if (index == start) break;
            }
            return false;
        }
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

int main(int argc, char* argv[]) {
    const size_t num_buckets = 1000;
    const size_t num_ops = 10000;
    
    size_t num_threads = 1;
    if (argc >= 2) {
        num_threads = std::stoul(argv[1]);
    }

    const size_t ops_per_thread = num_ops / num_threads;

    const int num_iter = 10;
    double total_time = 0.0;
    
    for (int i = 0; i < num_iter; i++) {  
        CuckooHash<int> hashset(num_buckets, 0.5, 8);
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