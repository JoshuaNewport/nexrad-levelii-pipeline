#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include "levelii/BackgroundFrameFetcher.h"
#include "levelii/ThreadPool.h"

int main() {
    const int num_threads = 32;
    const int num_buffers = 64;
    const int num_tasks = 100;
    
    std::cout << "Starting deadlock simulation with " << num_threads << " threads and " << num_buffers << " buffers..." << std::endl;
    
    auto buffer_pool = std::make_shared<BufferPool>(num_buffers, 1024);
    ThreadPool pool(num_threads);
    
    std::atomic<int> completed_tasks{0};
    
    for (int i = 0; i < num_tasks; ++i) {
        pool.enqueue([&, i]() {
            // Simulate process_discovery_batch
            
            // 1. Acquire raw_data and decompressed_data
            ScopedBuffer raw_data(buffer_pool);
            ScopedBuffer decompressed_data(buffer_pool);
            
            if (!raw_data.valid() || !decompressed_data.valid()) {
                std::cerr << "Failed to acquire initial buffers" << std::endl;
                return;
            }
            
            // Simulate processing
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            // 2. THE FIX: Release early
            raw_data.reset();
            decompressed_data.reset();
            
            // 3. Simulate processing products that need more buffers
            for (int p = 0; p < 3; ++p) {
                ScopedBuffer vol_grid(buffer_pool);
                if (!vol_grid.valid()) {
                    std::cerr << "Failed to acquire vol_grid buffer" << std::endl;
                    return;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                // vol_grid released here via RAII
            }
            
            completed_tasks++;
        });
    }
    
    // Wait with timeout
    auto start = std::chrono::steady_clock::now();
    while (completed_tasks < num_tasks) {
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(10)) {
            std::cerr << "❌ DEADLOCK DETECTED! Only " << completed_tasks << "/" << num_tasks << " tasks completed." << std::endl;
            return 1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "✅ Simulation completed successfully! No deadlock occurred." << std::endl;
    return 0;
}
