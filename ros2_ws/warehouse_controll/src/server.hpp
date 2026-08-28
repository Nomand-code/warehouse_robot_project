#pragma once

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>

class server {
private:
    int server_fd_ = -1;
    const char OKMessage[3]  = "OK";

public:
    server(int port);
    ~server();
    
    // Запрещаем копирование, чтобы избежать двойного закрытия сокета
    server(const server&) = delete;
    server& operator=(const server&) = delete;

    std::string receive();
    void close_socket();
};
