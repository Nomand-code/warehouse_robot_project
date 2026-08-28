#include "server.hpp"

server::server(int port) {
    // 1. Создаем сокет ("покупаем телефон")
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        std::cerr << "Ошибка создания сокета\n";
        return;
    }
    
    // 2. Настраиваем адрес и порт ("присваиваем номер")
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    
    // 3. Биндим сокет ("втыкаем провод в стену")
    if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Ошибка bind\n";
        close_socket();
        return;
    }

    // 4. Включаем прослушивание ("включаем звонок") — БЕЗ ЭТОГО accept() НЕ РАБОТАЕТ
    if (listen(server_fd_, 1) < 0) {
        std::cerr << "Ошибка listen\n";
        close_socket();
        return;
    }
}

void server::close_socket() {
    if (server_fd_ >= 0) {
        close(server_fd_);
        server_fd_ = -1;
    }
}

server::~server() {
    close_socket();
}

std::string server::receive() {
    if (server_fd_ < 0) {
        std::cerr << "Сервер не инициализирован или закрыт\n";
        return "";
    }

    // accept заблокирует поток, пока не появится клиент
    int client_fd = accept(server_fd_, nullptr, nullptr);
    if (client_fd < 0) {
        std::cerr << "Ошибка accept\n";
        return "";
    }
    std::cout << "Клиент подключен!\n";

    char buffer[1024] = {0};
    // read заблокирует поток, пока клиент не пришлет пакет
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    
    std::string result = "";
    if (bytes_read > 0) {
        // Создаем строку строго из полученных байт
        result = std::string(buffer, bytes_read);
        write(client_fd,&OKMessage,sizeof(OKMessage)-1);

    }

    // ВАЖНО: Закрываем сокет клиента, чтобы освободить ресурсы!
    close(client_fd); 
    
    return result;
}
