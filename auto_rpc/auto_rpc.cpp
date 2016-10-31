// auto_rpc.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "rpc_enpoint.h"

//demo
/*
#include <iostream>
int main()
{
using rpc_type = autorpc::Endpoint<autorpc::OutputArchiveSimple, autorpc::InputArchiveSimple>;

rpc_type rpc;

std::string str;
std::string ret;

rpc.reg("notify",
[](int a, std::string& b) {
std::cout << "recived notify : " << b << "+" << a << std::endl << std::endl;
});
str = rpc.request("notify", 1, std::string("hello world!"));
rpc.process(str.c_str(), str.size());


//在回调的最后使用RequestInfo类型是可以取到上下文的enpoint和id的.
// define a processor on one side.
rpc.reg("test",
[&ret](std::string& a, int b, rpc_type::RequestInfo& info) {
std::cout << "3. recived test invoke:" << a << " " << b << std::endl;
ret = info.endpoint_->response(info.seq_, b + 1, 222, 333);
});


std::cout << "1. call the processor on another side." << std::endl;
//在请求的最后使用的lambda, 是针对此次调用的回调
str = rpc.request("test", std::string("bbb"), 3,
[](int a, int b, int c) {
std::cout << "5. recived test reply:" << a << " " << b << " " << c << std::endl;
});


std::cout << "2. process invoked buf." << std::endl;
rpc.process(str.c_str(), str.size());


std::cout << "4. process reply buf." << std::endl;
rpc.process(ret.c_str(), ret.size());
}

//*/
#include <iostream>
int func()
{
    return 5;
}

struct memfff
{
    int funcc(int x)
    {
        member = x;
        std::cout << x << std::endl;
        return member;
    }
    int member = 0;
};
int main()
{
    using rpc_type = autorpc::Endpoint<autorpc::OutputArchiveSimple, autorpc::InputArchiveSimple>;
    rpc_type server;
    rpc_type client;

    std::string str;
    std::string ret;

    //1 一方向另一个发送通知
    client.reg("notify",
               [](int a, const std::string& b) {
        std::cout << "recived notify : " << b << "+" << a << std::endl << std::endl;
    });
    str = server.request("notify", 1, std::string("hello world!"));
    client.process(str.c_str(), str.size());


    //2 客户端向服务端请求数据

    // 服务端定义一个处理函数,该处理函数会回应客户端消息
    server.reg("test",
               [&ret](const std::string& a, int b, rpc_type::RequestInfo& info) {
        ////在回调的最后使用RequestInfo类型是可以取到上下文的enpoint和id的.
        std::cout << "3. recived test invoke:" << a << " " << b << std::endl;
        ret = info.endpoint_->response(info.seq_, b + 1, 222, 333);
    });


    std::cout << "1. call the processor on another side." << std::endl;
    //客户端向服务端发送请求
    str = client.request("test", std::string("bbb"), 3,
                         [](int a, int b, int c) {
        //在请求的最后使用的lambda, 是针对此次调用的回调
        std::cout << "5. recived test reply:" << a << " " << b << " " << c << std::endl;
    });


    std::cout << "2. process invoked buf." << std::endl;
    server.process(str.c_str(), str.size());

    std::cout << "4. process reply buf." << std::endl;
    client.process(ret.c_str(), ret.size());
}

