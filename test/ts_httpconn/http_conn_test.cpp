#include "../test_util.h"
#include "http/http_conn.h"
#include "workshop/simple_thread_pool.h"
#include <vector>

const char *ip = "127.0.0.1";
constexpr int port = 6666;

void Regular();
void Timer();
void AddFd(int epollfd, int fd, bool oneShot);

extern std::atomic<bool> g_stop;

// 视作main入口
/*
    完成以下工作：
        1.监听端口，为来临的任务请求派发任务
        2.为每个存在问题的地方记录日志
        3.定时器定时删除超时连接
        4.处理每一个到来的任务

*/


void StartTimerJob(ThreadPool& threadPool)
{
    g_stop.store(true);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    g_stop.store(false);
    threadPool.SignUpJob([]() { Regular(); });
    threadPool.SignUpJob([]() { Timer(); });
}

void WorkWrapper(HttpConn& conn)
{
    conn.Process();
}

TEST(HttpConn, RequestTest)
{
    /*
        线程至少需要：
            <1>一个监听线程 （放在主线程，不计入线程池）
            <2>一个定时器线程
            <3>一个用来推动定时器的线程
            <4>若干处理线程（暂时配置4个）
            // <5>模拟发送接收的线程
        因此配置 <2> + <3> + <4> = 6
    */
    constexpr uint8_t threadsNum = 6;
    ThreadPool threadPool(threadsNum);
    StartTimerJob(threadPool);

    constexpr int USERS_LIMIT = 65536;
    std::vector<HttpConn> users(USERS_LIMIT);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd > 0);

    // 用于设置套接字延迟关闭
    struct linger tmp = {1, 0};
    setsockopt(listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret >= 0);

    ret = listen(listenfd, 5);
    assert(ret >= 0);

    constexpr int MAX_EVENT_NUMBER = 10000;
    epoll_event events[MAX_EVENT_NUMBER];

    int epollfd = epoll_create(5);
    assert(epollfd != -1);

    AddFd(epollfd, listenfd, false);
    HttpConn::m_epollfd = epollfd;

    while(true) 
    {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if ((number < 0) && (errno != EINTR))
        {
            printf("epoll failure\n");
            break;
        }

        for (int  i = 0; i < number; ++i)
        {
            int sockfd = events[i].data.fd;
            if (sockfd == listenfd)
            {
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_addrlength);
                
                if (connfd < 0)
                {
                    printf("errno is : %d\n", errno);
                    continue;
                }

                if (HttpConn::m_userCount >= USERS_LIMIT)
                {
                    printf("%d, Internal server busy\n", connfd);
                    continue;
                }

                users[connfd].Init(connfd, client_address);
            }
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                users[sockfd].CloseConn();
            }
            else if (events[i].events & EPOLLIN)
            {
                if (users[sockfd].Read())
                {
                    //!!!!
                    threadPool.SignUpJob([&users, sockfd]() { users[sockfd].Process(); } );
                }
                else
                {
                    users[sockfd].CloseConn();
                }
            }
            else if (events[i].events & EPOLLOUT)
            {
                if (!users[sockfd].Write())
                {
                    users[sockfd].CloseConn();
                }
            }
            else
            {}
        }
    }
    close(epollfd);
    close(listenfd);
    g_stop.store(true);
}