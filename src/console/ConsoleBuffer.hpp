#pragma once

// Thread-safe store for one server's console history. Written to from a
// background ConsoleController thread, read from the UI thread when
// repainting the console view.

#include <mutex>
#include <string>
#include <vector>

namespace console
{

class ConsoleBuffer
{
public:
    explicit ConsoleBuffer(size_t maxLines = 5000)
        : maxLines_(maxLines)
    {
    }

    void Append(const std::wstring& line)
    {
        std::scoped_lock lock(mutex_);
        lines_.push_back(line);
        if (lines_.size() > maxLines_)
        {
            // Drop the oldest lines in one shot rather than one at a time,
            // so this stays O(1) amortized instead of O(n) per line once
            // the buffer is full.
            const size_t excess = lines_.size() - maxLines_;
            lines_.erase(lines_.begin(), lines_.begin() + static_cast<long>(excess));
        }
    }

    [[nodiscard]] std::vector<std::wstring> GetAll() const
    {
        std::scoped_lock lock(mutex_);
        return lines_;
    }

    void Clear()
    {
        std::scoped_lock lock(mutex_);
        lines_.clear();
    }

private:
    mutable std::mutex mutex_;
    std::vector<std::wstring> lines_;
    size_t maxLines_;
};

} // namespace console
