#pragma once

#include <assert.h>

#include <vector>

#include "define.h"

class Buffer
{
public:
    explicit Buffer(size_t initialSize = kInitialSize)
      : buffer_(kCheapPrepend + initialSize),
        readerindex_(kCheapPrepend),
        writerindex_(kCheapPrepend)
    {
        assert(ReadableBytes() == 0);
        assert(WritableBytes() == initialSize);
        assert(PrependableBytes() == kCheapPrepend);
    }
    Buffer(const Buffer&) = delete;
    Buffer& operator =(const Buffer&) = delete;
    ~Buffer() = default;

    char* Begin()
    { return &*buffer_.begin(); }

    const char* Begin() const
    { return &*buffer_.begin(); }

    const char* Peek() const
    { return Begin() + readerindex_; }

    size_t ReadableBytes() const
    { return writerindex_ - readerindex_; }

    size_t WritableBytes() const
    { return buffer_.size() - writerindex_; }

    size_t PrependableBytes() const
    { return readerindex_; }

    void Retrieve(size_t len)//others get size of 'len' from head of buf_,cut it out
    {
        assert(len <= ReadableBytes());
        if (len < ReadableBytes())
        {
            readerindex_ += len;
        }
        else
        {
            RetrieveAll();
        }
    }

    void RetrieveAll()
    { readerindex_ = kCheapPrepend,writerindex_ = kCheapPrepend; }

    void Append(const std::string& str)
    { Append(str.data(), str.size()); }

    void Append(const void* /*restrict*/ data, size_t len)
    {
        Append(static_cast<const char*>(data), len);
    }

    void Append(const char* /*restrict*/ data, size_t len)
    {
        EnsureWritableBytes(len);
        std::copy(data, data + len, BeginWrite());
        HasWritten(len);
    }

    void EnsureWritableBytes(size_t len)
    {
        if (WritableBytes() < len)
        {
            MakeSpace(len);
        }
        assert(WritableBytes() >= len);
    }

    char* BeginWrite()
    { return Begin() + writerindex_; }

    void HasWritten(size_t len)
    {
        assert(len <= WritableBytes());
        writerindex_ += len;
    }

    void MakeSpace(size_t len)
    {
        if (WritableBytes() + PrependableBytes() < len + kCheapPrepend)
        {
            // FIXME: move readable data
            buffer_.resize(writerindex_ + len);
        }
        else
        {
            // move readable data to the front, make space inside buffer
            assert(kCheapPrepend < readerindex_);
            size_t readable = ReadableBytes();
            std::copy(Begin() + readerindex_,
                Begin() + writerindex_,
                Begin() + kCheapPrepend);
            readerindex_ = kCheapPrepend;
            writerindex_ = readerindex_ + readable;
            assert(readable == ReadableBytes());
        }
    }

    std::string RetrieveAllAsString()
    { return RetrieveAsString(ReadableBytes()); }

    std::string RetrieveAsString(size_t len)
    {
        assert(len <= ReadableBytes());
        std::string result(Peek(), len);
        Retrieve(len);
        return result;
    }

    ssize_t ReadFd(FD fd);
private:
    std::vector<char> buffer_;
    size_t readerindex_;
    size_t writerindex_;
};
