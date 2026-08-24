#include "Systems/BackgroundJobQueue.h"

namespace StudioCore {

BackgroundJobQueue::BackgroundJobQueue() = default;

BackgroundJobQueue::~BackgroundJobQueue() {
    Cancel();
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void BackgroundJobQueue::Cancel() {
    m_cancel = true;
}

bool BackgroundJobQueue::IsRunning() const {
    return m_running;
}

float BackgroundJobQueue::GetProgress() const {
    return m_progress;
}

bool BackgroundJobQueue::HasResults() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return !m_running && !m_results.empty();
}

std::vector<std::shared_ptr<SpriteDefinition>> BackgroundJobQueue::ConsumeResults() {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto res = std::move(m_results);
    m_results.clear();
    return res;
}

}