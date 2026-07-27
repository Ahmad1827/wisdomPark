#pragma once
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <memory>

namespace StudioCore {

class SpriteDefinition;

class BackgroundJobQueue {
public:
    BackgroundJobQueue();
    ~BackgroundJobQueue();

    template<typename F>
    void StartJob(F&& jobFunction) {
        if (m_running) return;
        m_running = true;
        m_cancel = false;
        m_progress = 0.0f;
        m_results.clear();

        if (m_thread.joinable()) m_thread.join();

        m_thread = std::thread([this, job = std::forward<F>(jobFunction)]() {
            auto res = job(m_progress, m_cancel);
            std::lock_guard<std::mutex> lock(m_mutex);
            m_results = std::move(res);
            m_running = false;
        });
    }

    void Cancel();
    bool IsRunning() const;
    float GetProgress() const;
    bool HasResults();
    std::vector<std::shared_ptr<SpriteDefinition>> ConsumeResults();

private:
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_cancel{false};
    std::atomic<float> m_progress{0.0f};
    
    std::mutex m_mutex;
    std::vector<std::shared_ptr<SpriteDefinition>> m_results;
};

}