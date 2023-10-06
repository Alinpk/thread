#ifndef HTTP_CONN_H
#define HTTP_CONN_H
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/event.h>
#include <sys/epoll.h>

#endif

class HttpConn {
public:
    static constexpr int FILENAME_LEN_LIMIT = 200;
    static constexpr int READ_BUFFER_SIZE_LIMIT = 2048;
    static constexpr int WRITE_BUFFER_SIZE_LIMIT = 1024;
    
    enum Method {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATCH,
    };

    enum CheckState {
        REQUESTLINE = 0,
        HEADER,
        CONTENT,
    };

    enum HttpCode {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };

    enum LineStatus {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN,
    };

public:
    // 初始化新接受的连接
    void Init(int sockfd, const sockaddr_in& addr);  

    void CloseConn(bool realClose = true);

    void Process();

    bool Read();

    bool Write();
  
private:
    void Init();

    HttpCode ProcessRead();

    bool ProcessWrite(HttpCode ret);

    // Following functions is for [ProcessRead]
    HttpCode ParseRequestLine(char* text);
    HttpCode ParseRequestHeaders(char* text);
    HttpCode ParseRequestContent(char* text);
    HttpCode DoRequest();
    char* GetLine() { return m_readBuf + m_startLine; }
    LineStatus ParseLine();

    // Following functions is for [ProcessWrite]
    void Unmap();
    bool AddResponse(const char* format, ...);
    bool AddContent(const char* content);
    bool AddStatusLine(int status, const char* title);
    bool AddHeaders(int contentLen);
    bool AddContentLength(int contentLen)
    bool AddLinger();
    bool AddBlankLine();

public:
    // 所有socket上事件都被注册到同一个epoll内核事件表中
    static int m_epollfd;
    // 用户计数
    static int m_userCount;

private:
    // 当前HttpConn连接的socket以及对端socket地址信息
    int m_sockfd;
    sockaddr_in m_address;

    char m_readBuffer[READ_BUFFER_SIZE_LIMIT];
    // 用来标识已经读如缓冲区的最后一个字节的下一个位置
    int m_readIndex;
    // 正在分析的字符在缓冲区内的索引
    int m_checkedIndex;
    // 正在解析的行在缓冲区内的起始位置
    int m_startLine;

    char m_writeBuffer[WRITE_BUFFER_SIZE_LIMIT];
    // 写缓冲区中等待发送的字节数
    int m_writeIndex;

    // 主状态机当前所处的状态
    CheckState m_checkState;
    // 请求方法
    Method m_method;

    // 客户所请求的资源的完整路径，=doc_root+m_url
    char m_realFile[FILENAME_LEN_LIMIT];
    char* m_url;
    char* m_version;
    char* m_host;
    int   m_contentLength;
    // HTTP请求是否需要保持连接状态
    bool  m_linger;

    // 客户请求的目标文件被mmap到内存中的起始位置
    char* m_fileAddr;
    // 目标的文件状态。【是否存在，文件类型（是否为目录），是否可读，文件大小】
    struct stat m_fileStat;
    // 采用writev来执行写操作，定义以下两个成员，m_ivCount表示写内存块的数量
    struct iovec m_iv[2];
    int m_ivCount;
};

#endif