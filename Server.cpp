#define WIN32_LEAN_AND_MEAN
#ifdef _WIN32
    #include<windows.h>
    #include<WinSock2.h>
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include<string.h>
    #define SOCKET int
    #define INVALID_SOCKET (SOCKET)(~0)
    #define SOCKET_ERROR (-1)
#endif

#include<iostream>
#include<vector>
#include <algorithm>
using namespace std;

enum CMD
{
    CMD_LOGIN,
    CMD_LOGIN_RESULT,
    CMD_LOGOUT,
    CMD_LOGOUT_RESULT,
    CMD_NEW_USER_JOIN,
    CMD_ERROR
};
struct  DataHeader
{
    short dataLength;  //数据长度
    short cmd; //命令
};

//DataPackage
struct  Login : public DataHeader
{
    Login()
    {
        dataLength = sizeof(Login);
        cmd = CMD_LOGIN;
    }
    char userName[32];
    char PassWord[32];
};

struct LoginResult : public DataHeader
{
    LoginResult()
    {
        dataLength = sizeof(LoginResult);
        cmd = CMD_LOGIN_RESULT;
        result = 0;
    }
    int result;
};

struct LogOut : public DataHeader
{
    LogOut()
    {
        dataLength = sizeof(LogOut);
        cmd = CMD_LOGOUT;
    }
    char userName[32];
};

struct LogoutResult : public DataHeader
{
    LogoutResult()
    {
        dataLength = sizeof(LogoutResult);
        cmd = CMD_LOGOUT_RESULT;
        result = 1;
    }
    int result;
};

struct NewUserJoin : public DataHeader
{
    NewUserJoin()
    {
        dataLength = sizeof(LogoutResult);
        cmd = CMD_NEW_USER_JOIN;
        sock = 0;
    }
    int sock;
};
vector<SOCKET> g_clients;
int processor(SOCKET _cSock)
{
    //缓冲区
    char szRecv[1024] = { };
    //5、接收客户端数据
    int nlen = (int)recv(_cSock, szRecv, sizeof(DataHeader), 0);
    DataHeader* header = (DataHeader*)szRecv;
    if (nlen <= 0)
    {
        cout << "客户端<SOCKET = "<< _cSock<<">已退出，任务结束。" << endl;
        return -1;
    }
    //6、处理请求
    switch (header->cmd)
    {
        case CMD_LOGIN:
        {
            recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
            Login* login = (Login*)szRecv;
            //数据有偏移。因为已经接收了Dataheader。
            cout << "收到客户端<SOCKET = "<< _cSock <<">请求:CMD_LOGIN"
                 << ",数据长度：" << login->dataLength
                 << ",用户名：" << login->userName
                 << ",密码：" << login->PassWord << endl;
            //忽略判断用户密码是否正确
            LoginResult ret;
            send(_cSock, (char *)&ret, sizeof(LoginResult), 0);
        }
            break;
        case CMD_LOGOUT:
        {
            recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
            //数据发生偏移
            LogOut* logout = (LogOut*)szRecv;
            cout << "收到客户端<SOCKET = " << _cSock << ">请求:CMD_LOGOUT"
                 << ",数据长度：" << logout->dataLength
                 << ",用户名：" << logout->userName << endl;

            LogoutResult ret;
            send(_cSock, (char *)&ret, sizeof(LogoutResult), 0);
        }
            break;
        default:
        {
            DataHeader header = { 0, CMD_ERROR };
            send(_cSock, (char *)&header, sizeof(header), 0);
        }
            break;
    }
    return 0;
}

int main()
{
#ifdef _WIN32
    //启动Windows socket 2.x环境
    WORD ver = MAKEWORD(2, 2);//版本号
    WSADATA dat;
    WSAStartup(ver, &dat);
#endif
    //---用socket API建立简易Tcp服务端

    //1、建立一个socket 套接字
    SOCKET _sock = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

    //2、bind 绑定用于接收客户端连接的网络端口
    sockaddr_in _sin = {};
    _sin.sin_family = AF_INET;
    _sin.sin_port = htons(4567);  //host to net unsigned short

#ifdef _WIN32
    _sin.sin_addr.S_un.S_addr = INADDR_ANY;//inet_addr("127.0.0.1");
#else
    _sin.sin_addr.s_addr = INADDR_ANY;
#endif

    if (SOCKET_ERROR == bind(_sock, (sockaddr*)&_sin, sizeof(_sin)))
    {
        std::cout << "ERROR,绑定用于接收客户端连接的网络端口失败" << endl;
    }else {
        cout << "绑定网络端口成功。。。"  <<endl;
    }
    //3、listen 监听网络端口
    if (SOCKET_ERROR == listen(_sock, 5))
    {
        cout << "监听网络端口失败" << endl;
    }else {
        cout << "监听网络端口成功。。。" << endl;
    }

    while(true)
    {
        //伯克利套接字 BSD  socket
        fd_set fdRead; //描述符（socket）集合
        fd_set fdWrite;
        fd_set fdExp;

        //清理集合
        FD_ZERO(&fdRead);
        FD_ZERO(&fdWrite);
        FD_ZERO(&fdExp);

        //将描述符（socket）加入集合中
        FD_SET(_sock, &fdRead);
        FD_SET(_sock, &fdWrite);
        FD_SET(_sock, &fdExp);

        auto maxSock = _sock;
        for (int n = (int)g_clients.size()-1; n >= 0; n--)
        {
            FD_SET(g_clients[n], &fdRead);
            if(g_clients[n] > maxSock)
                maxSock = g_clients[n];
        }

        //nfds 是一个整数值 是指fd_set集合中所有描述符（socket）的范围。而不是数量
        // 既是所有文件描述符最大值+1  在Windows中这个参数无所谓，可以写0

        timeval t = { 1 , 0 };
        int ret = select(maxSock + 1, &fdRead, &fdWrite, &fdExp, &t);
        if (ret < 0)
        {
            cout << "select任务结束。" << endl;
            break;
        }
        //判断描述符（socket）是否在集合中
        if (FD_ISSET(_sock, &fdRead)||FD_ISSET(_sock, &fdWrite))
        {
            FD_CLR(_sock, &fdRead);//清理
            //4、accept 等待接受客户端连接
            sockaddr_in clientAddr = {};
            int nAddrLen = sizeof(sockaddr_in);
            SOCKET _cSock = INVALID_SOCKET;
            char msgBuf[] = "Hello ,I am Server.";

            _cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t *)&nAddrLen);
            if (INVALID_SOCKET == _cSock)
            {
                cout << "错误，接收到无效客户端SOCKET" << endl;
            }
            else
            {
                for (int n = (int)g_clients.size() - 1; n >= 0; n--)
                {
                    NewUserJoin userjoin;
                    send(g_clients[n], (const char*)&userjoin, sizeof(NewUserJoin), 0);
                }
                g_clients.push_back(_cSock);
                cout << "新客户端加入，IP = " << inet_ntoa(clientAddr.sin_addr) << "   socket = " << _cSock << endl;
            }
        }
        for (int n = (int)g_clients.size()-1; n >= 0; n--)
        {
            if (FD_ISSET(g_clients[n], &fdRead)||FD_ISSET(g_clients[n], &fdWrite))
            {
                if (-1 == processor(g_clients[n]))
                {
                    auto iter = g_clients.begin() + n;
                    //std::vector<SOCKET>::iterator iter = g_clients.begin();
                    if (iter != g_clients.end())
                    {
                        g_clients.erase(iter); //客户端退出，擦除
                    }
                }

            }
        }

        //cout << "空闲时间处理其他业务。。。" << endl;
    }

#ifdef _WIN32
    //closesocket关闭套接字
    for (size_t n = g_clients.size() - 1; n >= 0; n--)
    {
        closesocket(g_clients[n]);
    }

    //6、closesocket()关闭套接字
    closesocket(_sock);
#else
    //closesocket关闭套接字
    for (size_t n = g_clients.size() - 1; n >= 0; n--)
    {
        close(g_clients[n]);
    }

    //6、closesocket()关闭套接字
    close(_sock);
#endif

#ifdef _WIN32
    //清除Windows socket环境
    WSACleanup();
#endif

    cout << "已退出，任务结束。" << endl;
    getchar();
    return 0;
}

