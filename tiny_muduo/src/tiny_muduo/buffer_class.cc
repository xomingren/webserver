#include "buffer_class.h"

#include <sys/uio.h>//for readv
#include <unistd.h>//for close()

#include "log.h"

using namespace std;

const char Buffer::kCRLF[] = "\r\n";
const size_t Buffer::kCheapPrepend;
const size_t Buffer::kInitialSize;


ssize_t Buffer::ReadFd(FD fd)//if we use epollet, this function should be in a while-loop till read EAGAIN
{
    // saved an ioctl()/FIONREAD call to tell how much to read
    char extrabuf[65536];//64kbytes
    struct iovec vec[2];
    const size_t writable = WritableBytes();
    vec[0].iov_base = Begin() + writerindex_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;
    // when there is enough space in this buffer, don't read into extrabuf.
    // when extrabuf is used, we read 128k-1 bytes at most.
    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    const ssize_t readlength = ::readv(fd, vec, iovcnt);//problemmark
    if (readlength < 0)
    {
        if (errno == ECONNRESET)
        {
            LOG_CRIT << "ECONNREST closed socket fd:" << fd;
            close(fd);
        }
    }
    else if (readlength == 0)
    {
        LOG_INFO << "read 0 closed socket fd:" << fd;
        close(fd);
    }
    else if (static_cast<size_t>(readlength) <= writable)//means extra not use
    {
        writerindex_ += readlength;
        LOG_INFO << "recieved " << readlength << " bytes of data";
    }
    else//use extra,with readlength - writable
    {
        writerindex_ = buffer_.size();
        Append(extrabuf, readlength - writable);
    }
    // if (n == writable + sizeof extrabuf)
    // {
    //   goto line_15 const size_t writable = WritableBytes();;
    // }
    return readlength;
}