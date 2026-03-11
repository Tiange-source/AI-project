#include "network/Poller.h"
#include "network/Channel.h"
#include "utils/Logger.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>

namespace gomoku {

Poller::Poller() : epollfd_(::epoll_create1(EPOLL_CLOEXEC)), events_(kInitEventListSize) {
    if (epollfd_ < 0) {
        LOG_ERROR("Poller::Poller failed");
    }
}

Poller::~Poller() {
    ::close(epollfd_);
}

int Poller::poll(int timeoutMs, ChannelList& activeChannels) {
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    
    int savedErrno = errno;
    if (numEvents > 0) {
        // LOG_DEBUG(std::to_string(numEvents) + " events happened");
        fillActiveChannels(numEvents, activeChannels);
    } else if (numEvents == 0) {
        // LOG_DEBUG("nothing happened");
    } else {
        if (savedErrno != EINTR) {
            errno = savedErrno;
            LOG_ERROR("Poller::poll() error");
        }
    }
    
    return numEvents;
}

void Poller::fillActiveChannels(int numEvents, ChannelList& activeChannels) {
    for (int i = 0; i < numEvents; ++i) {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->setRevents(events_[i].events);
        activeChannels.push_back(channel);
    }
}

void Poller::updateChannel(Channel* channel) {
    int fd = channel->fd();
    
    if (channel->index() < 0) {
        // 新的Channel
        struct epoll_event event;
        memset(&event, 0, sizeof(event));
        event.events = channel->events();
        event.data.ptr = channel;
        
        if (::epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &event) < 0) {
            LOG_ERROR("epoll_ctl ADD error");
        }
        
        channel->setIndex(static_cast<int>(channels_.size()));
        channels_[fd] = channel;
    } else {
        // 更新现有的Channel
        struct epoll_event event;
        memset(&event, 0, sizeof(event));
        event.events = channel->events();
        event.data.ptr = channel;
        
        if (::epoll_ctl(epollfd_, EPOLL_CTL_MOD, fd, &event) < 0) {
            LOG_ERROR("epoll_ctl MOD error");
        }
    }
}

void Poller::removeChannel(Channel* channel) {
    int fd = channel->fd();
    
    if (channel->index() < 0) {
        return;
    }
    
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = channel->events();
    event.data.ptr = channel;
    
    if (::epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd, &event) < 0) {
        LOG_ERROR("epoll_ctl DEL error");
    }
    
    size_t n = channels_.erase(fd);
    (void)n;
    channel->setIndex(-1);
}

} // namespace gomoku