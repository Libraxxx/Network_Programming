#define WIN32_LEAN_AND_MEAN

#ifdef _WIN32
    #include<windows.h>
    #include<WinSock2.h>
#else
    #include<unistd.h>  //uni std
    #include<arpa/inet.h>
    #include<string.h>

    #define SOCKET int
    #define INVALID_SOCKET (SOCKET)(~0)
    #define SOCKET_ERROR (-1)

#endif

#include<thread>
#include<iostream>
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

int processor(SOCKET _cSock)
{
    //缓冲区
    char szRecv[1024] = { };
    //5、接收服务端数据
    int nlen = recv(_cSock, szRecv, sizeof(DataHeader), 0);
    DataHeader* header = (DataHeader*)szRecv;
    if (nlen <= 0)
    {
        cout << "与服务器断开连接。" << endl;
        return -1;
    }
    //6、处理请求
    switch (header->cmd)
    {
        case CMD_LOGIN_RESULT:
        {
            recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
            LoginResult* login = (LoginResult*)szRecv;
            //数据有偏移。因为已经接收了Dataheader。
            cout << "收到服务端消息:CMD_LOGIN_RESULT"
                 << ",数据长度：" << header->dataLength << endl;
        }
            break;
        case CMD_LOGOUT_RESULT:
        {
            recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
            LogoutResult* logout = (LogoutResult*)szRecv;
            //数据有偏移。因为已经接收了Dataheader。
            cout << "收到服务端消息:CMD_LOGOUT_RESULT"
                 << ",数据长度：" << header->dataLength << endl;
        }
            break;
        case CMD_NEW_USER_JOIN:
        {
            recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
            NewUserJoin* userjoin = (NewUserJoin*)szRecv;
            //数据有偏移。因为已经接收了Dataheader。
            cout << "收到服务端消息:CMD_NEW_USER_JOIN"
                 << ",数据长度：" << header->dataLength << endl;
        }
            break;
    }
}

bool g_bRun = true;
void cmdThread(SOCKET sock)
{
    while(true)
    {
        char cmdBuf[256] = { };
        cin >> cmdBuf;
        if (0 == strcmp(cmdBuf, "exit"))
        {
            g_bRun = false;
            cout << "退出cmdThread线程" << endl;
            break;
        }
        else if (0 == strcmp(cmdBuf, "login"))
        {
            Login login;
            strcpy(login.userName, "xjs");
            strcpy(login.PassWord, "xjspassword");
            send(sock, (const char *)&login, sizeof(Login), 0);
        }
        else if (0 == strcmp(cmdBuf, "logout"))
        {
            LogOut logout;
            strcpy(logout.userName, "xjs");
            send(sock, (const char *)&logout, sizeof(LogOut), 0);
        }
        else
        {
            cout << "不支持的命令！" << endl;
        }
    }
}

int main()
{
#ifdef _WIN32
    //(只存在Windows环境下)
    //启动Windows socket 2.x环境
    WORD ver = MAKEWORD(2, 2);//版本号
    WSADATA dat;
    WSAStartup(ver, &dat);
#endif
    //---用socket API建立简易Tcp客户端

    //1、建立一个socket 套接字
    SOCKET _sock = socket(AF_INET, SOCK_STREAM, 0);
    if (INVALID_SOCKET == _sock)
    {
        cout << "错误，建立SOCKET失败" << endl;
    }
    else {
        cout << "建立SOCKET成功"<<endl;
    }
    //2、连接服务器connect
    sockaddr_in _sin = {};
    _sin.sin_family = AF_INET;
    _sin.sin_port = htons(4567);

#ifdef _WIN32
    _sin.sin_addr.S_un.S_addr = inet_addr("10.20.75.100");
#else
    _sin.sin_addr.s_addr = inet_addr("10.20.75.100");
#endif

    int ret = connect(_sock,(sockaddr*)&_sin,sizeof(sockaddr_in));
    if (SOCKET_ERROR == ret)
    {
        std::cout << "错误，连接服务器失败...." << endl;
    }
    else {
        std::cout << "连接服务器成功...." << endl;
    }

    //启动线程
    thread t1(cmdThread, _sock);
    t1.detach();//线程分离

    while (g_bRun)
    {
        fd_set fdReads;
        fd_set fdWrite;
        FD_ZERO(&fdReads);
        FD_ZERO(&fdWrite);
        FD_SET(_sock, &fdReads);
        FD_SET(_sock, &fdWrite);
        timeval t = { 1 , 0 };

        //select---该函数用于监视文件描述符的变化情况——读写或是异常
        //参数一：最大的文件描述符加1。
        //参数二：用于检查可读性，
        //参数三：用于检查可写性，
        //参数四：用于检查带外数据，
        //参数五：一个指向timeval结构的指针，用于决定select等待I/O的最长时间。如果为空将一直等待。
        int ret = select(_sock+1, &fdReads, &fdWrite, 0, &t);
        if (ret < 0 )
        {
            cout << "select任务结束1。" << endl;
            break;
        }

        //判断描述符_sock是否在给定的描述符集fdReads中
        if (FD_ISSET(_sock, &fdReads))
        {
            FD_CLR(_sock, &fdReads);//清理

            //调用processor()函数进行处理
            if(-1 == processor(_sock))
            {
                cout << "select任务结束2。 " << endl;
                break;
            }
        }
        //cout << "空闲时间处理其他业务。。。" << endl;
        //Sleep(1000);
    }
#ifdef _WIN32
    //7、closesocket关闭套接字
    closesocket(_sock);
#else
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

