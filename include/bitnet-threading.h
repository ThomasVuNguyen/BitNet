#pragma once

#include <thread>
#include <atomic>
#include <vector>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

// Threading configuration for Raspberry Pi 5 (4 cores)
#define BITNET_MAX_THREADS 4
#define BITNET_CACHE_LINE_SIZE 64

// Work stealing queue for better load balancing
template<typename T>
class WorkStealingQueue {
private:
    std::queue<T> tasks;
    mutable std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> finished{false};

public:
    void push(T task) {
        std::lock_guard<std::mutex> lock(mtx);
        tasks.push(task);
        cv.notify_one();
    }

    bool try_pop(T& task) {
        std::lock_guard<std::mutex> lock(mtx);
        if (tasks.empty()) return false;
        task = tasks.front();
        tasks.pop();
        return true;
    }

    void wait_and_pop(T& task) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return !tasks.empty() || finished.load(); });
        if (!tasks.empty()) {
            task = tasks.front();
            tasks.pop();
        }
    }

    void finish() {
        finished.store(true);
        cv.notify_all();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mtx);
        return tasks.empty();
    }
};

// Thread pool optimized for Raspberry Pi 5
class BitNetThreadPool {
private:
    std::vector<std::thread> workers;
    WorkStealingQueue<std::function<void()>> task_queue;
    std::atomic<bool> stop{false};
    std::atomic<int> active_workers{0};
    
    // NUMA awareness for Pi 5
    void set_cpu_affinity(int thread_id) {
#ifdef __linux__
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(thread_id % 4, &cpuset);  // Pi 5 has 4 cores
        
        if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
            // Fallback: use sched_setaffinity
            sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
        }
#endif
    }

public:
    BitNetThreadPool() {
        int num_threads = std::min(BITNET_MAX_THREADS, (int)std::thread::hardware_concurrency());
        
        for (int i = 0; i < num_threads; ++i) {
            workers.emplace_back([this, i]() {
                set_cpu_affinity(i);
                worker_loop();
            });
        }
    }

    ~BitNetThreadPool() {
        stop.store(true);
        task_queue.finish();
        
        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    template<typename F, typename... Args>
    void enqueue(F&& f, Args&&... args) {
        auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        task_queue.push(task);
    }

    void wait_all() {
        while (!task_queue.empty() || active_workers.load() > 0) {
            std::this_thread::yield();
        }
    }

private:
    void worker_loop() {
        while (!stop.load()) {
            std::function<void()> task;
            
            if (task_queue.try_pop(task)) {
                active_workers.fetch_add(1);
                task();
                active_workers.fetch_sub(1);
            } else {
                std::this_thread::yield();
            }
        }
    }
};

// Thread-safe matrix tile for parallel processing
struct MatrixTile {
    int start_row;
    int end_row;
    int start_col;
    int end_col;
    int tile_id;
    
    MatrixTile(int sr, int er, int sc, int ec, int id) 
        : start_row(sr), end_row(er), start_col(sc), end_col(ec), tile_id(id) {}
};

// Optimized tile distribution for Pi 5
class TileDistributor {
private:
    int num_threads;
    std::atomic<int> next_tile{0};
    std::vector<MatrixTile> tiles;
    
public:
    TileDistributor(int rows, int cols, int tile_size, int n_threads) 
        : num_threads(n_threads) {
        
        // Create tiles optimized for Pi 5 cache hierarchy
        int row_tiles = (rows + tile_size - 1) / tile_size;
        int col_tiles = (cols + tile_size - 1) / tile_size;
        
        for (int i = 0; i < row_tiles; ++i) {
            for (int j = 0; j < col_tiles; ++j) {
                int start_row = i * tile_size;
                int end_row = std::min(start_row + tile_size, rows);
                int start_col = j * tile_size;
                int end_col = std::min(start_col + tile_size, cols);
                
                tiles.emplace_back(start_row, end_row, start_col, end_col, 
                                 i * col_tiles + j);
            }
        }
    }
    
    bool get_next_tile(MatrixTile& tile) {
        int tile_id = next_tile.fetch_add(1);
        if (tile_id >= (int)tiles.size()) {
            return false;
        }
        tile = tiles[tile_id];
        return true;
    }
    
    int total_tiles() const { return tiles.size(); }
};

// Memory prefetching for better cache utilization
inline void prefetch_for_read(const void* addr) {
#ifdef __ARM_NEON
    __builtin_prefetch(addr, 0, 3);  // Temporal locality
#endif
}

inline void prefetch_for_write(const void* addr) {
#ifdef __ARM_NEON
    __builtin_prefetch(addr, 1, 3);  // Temporal locality, write
#endif
}

// Cache-aligned memory allocation
template<typename T>
T* aligned_alloc(size_t count) {
    void* ptr = nullptr;
    if (posix_memalign(&ptr, BITNET_CACHE_LINE_SIZE, count * sizeof(T)) != 0) {
        return nullptr;
    }
    return static_cast<T*>(ptr);
}

// Thread-safe progress tracking
class ProgressTracker {
private:
    std::atomic<int> completed_tiles{0};
    int total_tiles;
    std::mutex progress_mtx;
    
public:
    ProgressTracker(int total) : total_tiles(total) {}
    
    void mark_completed() {
        int completed = completed_tiles.fetch_add(1) + 1;
        
        // Optional: Log progress every 10%
        if (completed % (total_tiles / 10 + 1) == 0) {
            std::lock_guard<std::mutex> lock(progress_mtx);
            printf("BitNet threading progress: %d/%d tiles (%.1f%%)\n", 
                   completed, total_tiles, 100.0f * completed / total_tiles);
        }
    }
    
    bool is_complete() const {
        return completed_tiles.load() >= total_tiles;
    }
};

// Global thread pool instance
extern std::unique_ptr<BitNetThreadPool> g_bitnet_thread_pool;

// Initialize threading system
void bitnet_threading_init();

// Cleanup threading system
void bitnet_threading_cleanup();

// Get optimal number of threads for current system
int bitnet_get_optimal_thread_count();
