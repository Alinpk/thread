#include "http_conn.h"

#define XX(http_state_no, title, form)                 \
    const char *HTTP_##http_state_no##_TITLE = #title; \
    const char *HTTP_##http_state_no##_FORM = #form;

#define HTTP_MAP(XX)                                                                         \
    XX(200, OK, );                                                                           \
    XX(400, Bad Request, Your request has bad syntax or is inherently impossible to satisfy) \
    XX(403, Forbidden, You do not have permission to get file from this server)              \
    XX(404, Not Found, The requested file was not found on this server)                      \
    XX(500, Internal Error, There was an unusual problem serving the requested file)

HTTP_MAP(XX)

#undef XX
#undef HTTP_MAP

const char *DOC_ROOT = "/Users/huangzhujiang/home/prj/thread/resource";

namespace
{
    int SetNonblocking(int fd)
    {
        int old_option = fcntl(fd, F_GETFL);
        int new_option = old_option | O_NONBLOCK;
        fcntl(fd, F_SETFL, new_option);
        return old_option;
    }

    void AddFd(int epollfd, int fd, bool oneShot)
    {
        epoll_event event;
        event.data.fd = fd;
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
        if (oneShot)
        {
            event.events |= EPOLLONESHOT;
        }

        epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
        SetNonblocking(fd);
    }

    void RemoveFd(int epollfd, int fd)
    {
        epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
        close(fd)
    }

    void ModFd(int epollfd, int fd, int ev)
    {
        epoll_event event;
        event.data.fd = fd;
        event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
        epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
    }
}

int HttpConn::m_userCount = 0;
int HttpConn::m_epollfd = -1;

void HttpConn::CloseConn(bool realClose)
{
    if (realClose && (m_sockfd != -1))
    {
        removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        m_userCount--;
    }
}

void HttpConn::Init(int sockfd, const sockaddr_in &addr)
{
    m_sockfd = sockfd;
    m_address = addr;
    // 避免TIME_WAIT状态影响
    // TIME_WAIT：用户关闭套接字后，内核栈维持一段时间
    int reuse = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    AddFd(m_epollfd, sockfd, true);
    m_userCount;

    Init();
}

void HttpConn::Init()
{
    m_checkState = CheckState::REQUESTLINE;
    m_linger = false;

    // 暂时仅支持GET方法
    m_method = GET;
    m_url = 0;
    m_version = 0;
    m_contentLength = 0;
    m_host = 0;
    m_startLine = 0;
    m_checkedIndex = 0;
    m_readIndex = 0;
    m_writeIndex = 0;
    memset(m_readBuffer, '\0', READ_BUFFER_SIZE_LIMIT);
    memset(m_writeBuffer, '\0', WRITE_BUFFER_SIZE_LIMIT);
    memset(m_realFile, '\0', FILENAME_LEN_LIMIT);
}

HttpConn::LineStatus HttpConn::ParseLine()
{
    char temp;
    for (; m_checkedIndex < m_readIndex; ++m_checkedIndex)
    {
        temp = m_readBuffer[m_checkedIndex];

        if (temp == '\r')
        {
            // readIndex的定义保证checkIndex一定小于readindex
            if ((m_checkedIndex + 1) == m_readIndex)
            {
                // 如果下次读进来的还是\n则可以继续解析
                return LINE_OPEN;
            }
            else if (m_readBuffer[m_checkedIndex + 1] == '\n')
            {
                // 遇到\r\n为完整的一行
                m_readBuffer[m_checkedIndex++] = '\0';
                m_readBuffer[m_checkedIndex++] = '\0';
                return LINE_OK;
            }

            return LINE_BAD;
        }

        if (temp == '\n')
        {
            //  对应上面的LINE_OPEN情况
            if ((m_checkedIndex > 1) && (m_readBuffer[m_checkedIndex - 1] == '\r'))
            {
                m_readBuffer[m_checkedIndex - 1] = '\0';
                m_readBuffer[m_checkedIndex - 1] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }

        return LINE_OPEN;
    }
}

bool HttpConn::Read()
{
    if (m_readIndex >= READ_BUFFER_SIZE_LIMIT)
    {
        return false;
    }

    int bytesRead = 0;
    while (1)
    {
        bytesRead = recv(m_sockfd, m_readBuffer + m_readIndex, READ_BUFFER_SIZE_LIMIT - m_readIndex, 0);
        if (bytes_read == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            return false;
        }
        else if (bytesRead == 0)
        {
            return false;
        }

        m_readIndex += bytesRead;
    }
    return true;
}

// format[method|url|version]
HttpConn::HttpCode HttpConn::ParseRequestLine(char* text)
{
    // 找到第一个" \t"所在位置，做切分，并将url指向该位置,metho指向前面一段
    m_url = strpbrk(text, " \t");
    if (!m_url)
    {
        return HttpConn::BAD_REQUEST;
    }
    *m_url = '\0';
    ++m_url;
    char* method = text;
    if (strcasecmp(method, "GET") == 0) 
    {
        m_method = GET;
    }
    else
    {
        printf("[%s] is not supported", method);
        return BAD_REQUEST;
    }

    // 把多余的" \t"消除
    m_url += strspn(m_url, " \t");
    m_version = strpbrk();

    if (!m_version) 
    {
        return HttpCode::BAD_REQUEST;
    }

    *m_version++ = '\0';
    m_version += strspn(m_version, " \t");
    if (strcasecmp(m_version, "HTTP/1.1") != 0)
    {
        // Only support http 1.1
        return HttpCode::BAD_REQUEST;
    }

    if (strcasecmp(m_url, "http://", 7) == 0)
    {
        m_url += 7;
        m_url = strchr(m_url, '/');
    }

    if (!m_url || m_url[0] != '/')
    {
        return HttpCode::BAD_REQUEST;
    }

    m_checkState = CheckState::HEADER;
    return HttpCode::NO_REQUEST;
}

HttpConn::HttpCode HttpConn::ParseRequestHeaders(char* text)
{
    // 遇见空行表示头部字段解析完毕
    if (text[0] == '\0')
    {
        if (m_contentLength != 0)
        {
            m_checkState = CheckState::CONTENT;
            return NO_REQUEST;
        }
        return HttpCode::GET_REQUEST;
    }

    if (strncasecmp(text, "Connection:", 11) == 0)
    {
        text += 11;
        text += strspn(text, "\t");
        if (strcasecmp(text, "keep-alive") == 0)
        {
            m_linger = true;
        }
    }
    else if (strcasecmp(text, "Content-Length:", 15) == 0)
    {
        text += 15;
        text += strspn(text, " \t");
        m_contentLength = atol(text);
    }
    else if (strncasecmp(text, "Host:", 5) == 0)
    {
        text += 5;
        text += strspn(text, " \t");
        m_host = text;
    }
    else
    {
        printf("Unknow header %s\n", text);
    }
    return NO_REQUEST;
}

HttpConn::HttpCode HttpConn::ParseRequestContent(char* text)
{
    // 说明context已经被完整读入了
    if (m_readIndex >= (m_contentLength + m_checkedIndex))
    {
        text[m_contentLength] = '\0';
        return HttpCode::GET_REQUEST;
    }
    return HttpCode::NO_REQUEST;
}

HttpConn::HttpCode HttpConn::ProcessRead()
{
    LineStatus lineStatus = LINE_OK;
    HttpCode ret = NO_REQUEST;
    char* text = 0;

    // cond1: 正在content段且完成一行接收
    // cond2: 非content（header/request_line)且完成一行接收
    while (((m_checkState == CheckState::CONTENT) && (lineStatus == LineStatus::LINE_OK))
        || ((lineStatus = ParseLine()) == LineStatus::LINE_OK))
    {
        text = GetLine();
        
        m_startLine = m_checkedIndex;
        printf("got 1 http line: %s\n", text);

        switch(m_checkedIndex)
        {
            case CheckState::REQUESTLINE:
            {
                ret = ParseRequestLine(text);
                if (ret == BAD_REQUEST)
                {
                    return BAD_REQUEST;
                }
                break;
            }
            case CheckState::HEADER:
            {
                ret = ParseRequestHeaders(text);
                if (ret == BAD_REQUEST)
                {
                    return BAD_REQUEST;
                }
                else if (ret == GET_REQUEST)
                {
                    return DoRequest();
                }
                break;
            }
            case CheckState::CONTENT:
            {
                ret = ParseRequestContent(text);
                if (ret == HttpCode::GET_REQUEST)
                {
                    return DoRequest();
                }
                lineStatus = LINE_OPEN;
                break;
            }
            default:
            {
                return INTERNAL_ERROR;
            }
        }
    }
    return NO_REQUEST;
}

HttpConn::HttpCode HttpConn::DoRequest()
{
    strcpy(m_realFile, DOC_ROOT);
    int len = strlen(DOC_ROOT);
    strncpy(m_realFile + len, m_url, FILENAME_LEN_LIMIT - len - 1);

    if (stat(m_realFile, &m_fileStat) < 0)
    {
        return HttpCode::NO_RESOURCE;
    }

    if (!(m_fileStat.mode & S_IROTH))
    {
        return HttpCode::FORBIDDEN_REQUEST;
    }

    if (S_ISDIR(m_fileStat.st_mode))
    {
        return HttpCode::BAD_REQUEST;
    }

    int fd = open(m_realFile, O_RDONLY);
    m_fileAddr = (char*)mmap(0, m_fileStat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    return FILE_REQUEST;
}

void HttpConn::Unmap()
{
    if (m_fileAddr)
    {
        munmap(m_fileAddr, m_fileStat.st_size);
        m_fileAddr = 0;
    }
}

bool HttpConn::Write()
{
    int temp = 0;
    int bytesHaveSend = 0;
    int bytesToSend = m_writeIndex;

    if (bytesToSend == 0)
    {
        ModFd(m_epollfd, m_sockfd, EPOLLIN);
        Unit();
        return true;
    }

    while (1)
    {
        temp = writev(m_sockfd, m_iv, m_iv_count);
        if (temp <= -1)
        {
            // 如果TCP写缓冲区没有空间，则等待下一轮EPOLLOUT事件。
            // 在此期间，服务器无法立即接受同一客户的下一个请求，但却可以保证连接的完整性
            if (errno == EAGAIN)
            {
                ModFd(m_epollfd, m_sockfd, EPOLLOUT);
                return true;
            }
            // else: send failed
            Unmap();
            return false;
        }
        
        bytesToSend -= temp;
        bytesHaveSend += temp;
        if (bytesToSend <= bytesHaveSend)
        {
            Unmap();
            if (m_linger)
            {
                Init();
                ModFd(m_epollfd, m_sockfd, EPOLLIN);
                return true;
            }
            else
            {
                // ！为什么不关闭m_sockfd?
                ModFd(m_epollfd, m_sockfd, EPOLLIN);
                return false;
            }
        }
    }
}

bool HttpConn::AddResponse(const char* format, ...)
{
    if (m_writeIndex >= WRITE_BUFFER_SIZE_LIMIT)
    {
        return false;
    }
    va_list arg_list;
    va_start(arg_list, format);
    int len = vsnprintf(m_writeBuffer + m_writeIndex, WRITE_BUFFER_SIZE_LIMIT - 1 - m_writeIndex, format, arrg_list);

    if (len >= (WRITE_BUFFER_SIZE_LIMIT - 1 - m_writeIndex))
    {
        return false;
    }
    m_writeIndex += len;
    va_end(arg_list);
    return true;
}

bool HttpConn::AddStatusLine(int status, const char* title)
{
    return AddResponse("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool HttpConn::AddHeaders(int contentLen)
{
    return AddContentLength(contentLen)
    && AddLinger()
    && AddBlankLine();
}

bool HttpConn::AddContentLength(int contentLen)
{
    return AddResponse("Content-Lenght: %d\r\n", content_len);
}

bool HttpConn::AddLinger()
{
    return AddResponse("Connection: %s\r\n", (m_linger == true) ? "keep-alive" : "close");
}

bool HttpConn::AddBlankLine()
{
    return AddResponse("%s", "\r\n");
}

bool HttpConn::AddContent(const char* content)
{
    return AddResponse("%s", content);
}

bool HttpConn::ProcessWrite(HttpCode ret)
{
    switch(ret)
    {
        case INTERNAL_ERROR:
        {
            AddStatusLine(500, HTTP_500_TITLE);
            AddHeaders(strlen(HTTP_500_FORM));
            if (!AddContent(HTTP_500_FORM))
            {
                return false;
            }
            break;
        }
        case BAD_REQUEST:
        {
            AddStatusLine(400, HTTP_400_TITLE);
            AddHeaders(strlen(HTTP_400_FORM));
            if (!AddContent(HTTP_400_FORM))
            {
                return false;
            }
            break;
        }
        case NO_RESOURCE:
        {
            AddStatusLine(404, HTTP_404_TITLE);
            AddHeaders(strlen(HTTP_404_FORM));
            if (!AddContent(HTTP_404_FORM))
            {
                return false;
            }
            break;
        }
        case FORBIDDEN_REQUEST:
        {
            AddStatusLine(403, HTTP_403_TITLE);
            AddHeaders(strlen(HTTP_403_FORM));
            if (!AddContent(HTTP_403_FORM))
            {
                return false;
            }
            break;
        }
        case FILE_REQUEST:
        {
            AddStatusLine(200, HTTP_200_TITLE);
            if (m_fileStat.st_size != 0)
            {
                AddHeaders(m_fileStat.st_size);
                m_iv[0].iov_base = m_writeBuffer;
                m_iv[0].iov_len = m_writeIndex;
                m_iv[1].iov_base = m_fileAddr;
                m_iv[1].iov_len = m_fileStat.st_size;
                m_iv_count = 2;
                return true;
            }
            else
            {
                const char* ok_string = "<html><body></body></html>";
                AddHeaders(strlen(ok_string));
                if (!AddContent(ok_string))
                {
                    return false;
                }
            }
        }
        default:
        {
            return false;
        }
        m_iv[0].iov_base = m_writeBuffer;
        m_iv[0].iov_len = m_writeIndex;
        m_ivCount = 1;
        return true;
    }
}

void HttpConn::Process()
{
    HttpCode readRet = ProcessRead();
    if (readRet == NO_REQUEST)
    {
        ModFd(m_epollfd, m_sockfd, EPOLLIN);
        return;
    }

    bool writeRet = ProcessWrite(readRet);
    if (!writeRet)
    {
        CloseConn();
    }

    ModFd(m_epollfd, m_sockfd, EPOLLOUT);
}