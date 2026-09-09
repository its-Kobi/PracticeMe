#pragma once
#include <cstdint>
#include <vector>
#include <optional>
struct Frame { int width; int height; std::vector<uint8_t> data; uint64_t timestampUs; };
class IFrameCapture {
public:
    virtual ~IFrameCapture() = default;
    virtual bool init(int width, int height) = 0;
    virtual std::optional<Frame> capture() = 0;
    virtual void shutdown() = 0;
};
