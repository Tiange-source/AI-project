#ifndef POLLER_H
#define POLLER_H

#include <sys/epoll.h>
#include <vector>
#include <map>
#include <memory>

namespace gomoku {

// 前向声明
class Channel;

class Poller {
public:
    using ChannelList = std::vector<Channel*>;
    
    Poller();
    ~Poller();
    
    // 禁止拷贝
    Poller(const Poller&) = delete;
    Poller& operator=(const Poller&) = delete;
    
    // 等待事件
    int poll(int timeoutMs, ChannelList& activeChannels);
    
    // 更新Channel
    void updateChannel(Channel* channel);
    
    // 移除Channel
    void removeChannel(Channel* channel);

private:
    static const int kInitEventListSize = 16;
    
    // 填充活跃Channel列表
    void fillActiveChannels(int numEvents, ChannelList& activeChannels);
    
    int epollfd_;
    std::vector<struct epoll_event> events_;
    std::map<int, Channel*> channels_;
};

} // namespace gomoku

#endif // POLLER_H