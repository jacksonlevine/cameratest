#include <vector>
#include <atomic>

struct RingBuffer {
    std::atomic<int> readHead;
    std::atomic<int> writeHead;
    std::atomic<int> count;
    static const size_t maxBuffers = 128;
    static const size_t bufferSize = 480;
    std::vector<std::vector<float>> buffers;
    RingBuffer& operator=(RingBuffer&& other) noexcept;
    RingBuffer(const RingBuffer& other);
    RingBuffer();
    void write(float *data, size_t amount);
    void readOneBuffer(float *out);
};