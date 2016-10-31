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


//�ڻص������ʹ��RequestInfo�����ǿ���ȡ�������ĵ�enpoint��id��.
// define a processor on one side.
rpc.reg("test",
[&ret](std::string& a, int b, rpc_type::RequestInfo& info) {
std::cout << "3. recived test invoke:" << a << " " << b << std::endl;
ret = info.endpoint_->response(info.seq_, b + 1, 222, 333);
});


std::cout << "1. call the processor on another side." << std::endl;
//����������ʹ�õ�lambda, ����Դ˴ε��õĻص�
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

    //1 һ������һ������֪ͨ
    client.reg("notify",
               [](int a, const std::string& b) {
        std::cout << "recived notify : " << b << "+" << a << std::endl << std::endl;
    });
    str = server.request("notify", 1, std::string("hello world!"));
    client.process(str.c_str(), str.size());


    //2 �ͻ�����������������

    // ����˶���һ��������,�ô��������Ӧ�ͻ�����Ϣ
    server.reg("test",
               [&ret](const std::string& a, int b, rpc_type::RequestInfo& info) {
        ////�ڻص������ʹ��RequestInfo�����ǿ���ȡ�������ĵ�enpoint��id��.
        std::cout << "3. recived test invoke:" << a << " " << b << std::endl;
        ret = info.endpoint_->response(info.seq_, b + 1, 222, 333);
    });


    std::cout << "1. call the processor on another side." << std::endl;
    //�ͻ��������˷�������
    str = client.request("test", std::string("bbb"), 3,
                         [](int a, int b, int c) {
        //����������ʹ�õ�lambda, ����Դ˴ε��õĻص�
        std::cout << "5. recived test reply:" << a << " " << b << " " << c << std::endl;
    });


    std::cout << "2. process invoked buf." << std::endl;
    server.process(str.c_str(), str.size());

    std::cout << "4. process reply buf." << std::endl;
    client.process(ret.c_str(), ret.size());
}

