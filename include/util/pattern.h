#pragma once

#define DISALLOW_COPY(Typename) Typename(const Typename&) = delete;
#define DISALLOW_ASSIGN(Typename) Typename& operator=(const Typename&) = delete;
#define DISALLOW_MOVE(Typename) Typename(Typename&&) = delete;
#define DEFAULT_COPY(Typename) Typename(const Typaname&) = delete;

#define DISTALLOW_COPY_AND_ASSIGN(Typename) DISALLOW_ASSIGN(Typename)\
                                            DISALLOW_COPY(Typename);

#define DISTALLOW_COPY_AND_MOVE(Typename)   DISALLOW_ASSIGN(Typename)\
                                            DISALLOW_COPY(Typename)\
                                            DISALLOW_MOVE(Typename);

class NonCopyable {
protected:
    NonCopyable() = default;
    virtual ~NonCopyable() = default;
private:
    DISTALLOW_COPY_AND_MOVE(NonCopyable);
};

// 用以支持构造函数无入参的类
#define DECLARE_SINGLETON(ClassName) \
private:                             \
    friend Singleton<ClassName>;     \
    ClassName();                     \
    ~ClassName();

template<typename T>
class Singleton : public NonCopyable {
public:
    static T& GetInstance()
    {
        return m_instance;
    }
private:
    static T m_instance;
};

template<typename T>
T Singleton<T>::m_instance;