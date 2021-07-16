#include "eventloop _class.h"

#include <iostream>

#include <vector>

using namespace std;

EventLoop::EventLoop()
	:quit_(false),
	 poller_(new Epoll())
{
}

void EventLoop::Loop()
{
    while (!quit_)
    {
        cout << "epoll waiting..." << endl;
        vector<Channel*> channels;
        poller_->Poll(&channels);

        for (const auto& it : channels)
        {
            //choose different callback
            //1.if channel is acceptchannel ,use OnAccept() and the socketfd returned to generate a new tcpconnection in tcpserver
            //2.if channel is tcpconnection, use OnEvent() to decode compute encode etc
            it->HandleEvent();
        }
    }
}

void EventLoop::Update(Channel* channel)
{
    poller_->Update(channel);
}
