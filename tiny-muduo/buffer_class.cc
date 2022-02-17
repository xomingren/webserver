#include "buffer_class.h"

#include<iostream>

#include <sys/uio.h>//for readv
#include <unistd.h>//for close()

using namespace std;

ssize_t Buffer::ReadFd(FD fd)
{
    // saved an ioctl()/FIONREAD call to tell how much to read
    char extrabuf[65536];
    struct iovec vec[2];
    const size_t writable = WritableBytes();
    vec[0].iov_base = Begin() + writerindex_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;
    // when there is enough space in this buffer, don't read into extrabuf.
    // when extrabuf is used, we read 128k-1 bytes at most.
    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    const ssize_t readlength = ::readv(fd, vec, iovcnt);
    if (readlength < 0)
    {
        if (errno == ECONNRESET)
        {
            cout << "ECONNREST closed socket fd:" << fd << endl;
            close(fd);
        }
    }
    else if (readlength == 0)
    {
        cout << "read 0 closed socket fd:" << fd << endl;
        close(fd);
    }
    else if (static_cast<size_t>(readlength) <= writable)
    {
        writerindex_ += readlength;
        cout << "recieved " << readlength << " bytes of data" << endl;
    }
    else
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