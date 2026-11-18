#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

/* Константы для доменов */
constexpr int DOMAIN_AF_INET = 0;
constexpr int DOMAIN_AF_INET6 = 1;

/* Константы для типов сокетов */
constexpr int TYPE_STREAM = 0;
constexpr int TYPE_DGRAM = 1;

/* Класс для работы с сокетами (RAII паттерн для автоматического закрытия ресурсов) */
class SocketManager {
private:
    SOCKET m_socket;

public:
    SocketManager() : m_socket(INVALID_SOCKET) {}

    /* Деструктор: гарантирует закрытие сокета даже при выходе из функции */
    ~SocketManager() {
        if (m_socket != INVALID_SOCKET) {
            Close();
        }
    }

    /* Создание сокета */
    bool Create(int domain, int type) {
        int family = (domain == DOMAIN_AF_INET) ? AF_INET : AF_INET6;
        int sock_type = (type == TYPE_STREAM) ? SOCK_STREAM : SOCK_DGRAM;

        m_socket = socket(family, sock_type, 0);
        if (m_socket == INVALID_SOCKET) {
            std::cerr << "socket failed: " << WSAGetLastError() << std::endl;
            return false;
        }

        std::cout << "Сокет создан успешно. Дескриптор (Handle): " << m_socket << std::endl;
        return true;
    }

    /* Привязка сокета к адресу и порту */
    bool Bind(int domain, const std::string& ip, int port) {
        struct sockaddr_in addr4;
        struct sockaddr_in6 addr6;
        int family = (domain == DOMAIN_AF_INET) ? AF_INET : AF_INET6;

        if (family == AF_INET) {
            std::memset(&addr4, 0, sizeof(addr4));
            addr4.sin_family = AF_INET;
            addr4.sin_port = htons(port); /* Преобразование порядка байт (Host to Network) */

            /* Преобразование IP из текста в бинарный формат */
            if (inet_pton(AF_INET, ip.c_str(), &addr4.sin_addr) <= 0) {
                if (WSAGetLastError() == 0) {
                    std::cerr << "Ошибка: неверный формат IPv4-адреса '" << ip << "'" << std::endl;
                }
                else {
                    std::cerr << "inet_pton failed: " << WSAGetLastError() << std::endl;
                }
                return false;
            }

            if (bind(m_socket, (struct sockaddr*)&addr4, sizeof(addr4)) == SOCKET_ERROR) {
                int err = WSAGetLastError();
                if (err == WSAEADDRINUSE) {
                    std::cerr << "bind: Address already in use (порт занят)" << std::endl;
                }
                else {
                    std::cerr << "bind failed: " << err << std::endl;
                }
                return false;
            }
        }
        else {
            std::memset(&addr6, 0, sizeof(addr6));
            addr6.sin6_family = AF_INET6;
            addr6.sin6_port = htons(port);

            if (inet_pton(AF_INET6, ip.c_str(), &addr6.sin6_addr) <= 0) {
                if (WSAGetLastError() == 0) {
                    std::cerr << "Ошибка: неверный формат IPv6-адреса '" << ip << "'" << std::endl;
                }
                else {
                    std::cerr << "inet_pton failed: " << WSAGetLastError() << std::endl;
                }
                return false;
            }

            if (bind(m_socket, (struct sockaddr*)&addr6, sizeof(addr6)) == SOCKET_ERROR) {
                int err = WSAGetLastError();
                if (err == WSAEADDRINUSE) {
                    std::cerr << "bind: Address already in use (порт занят)" << std::endl;
                }
                else {
                    std::cerr << "bind failed: " << err << std::endl;
                }
                return false;
            }
        }

        std::cout << "Сокет успешно привязан к " << ip << ":" << port << std::endl;
        return true;
    }

    /* ВАРИАНТ 13: Вывод типа сокета через getsockopt(SO_TYPE) */
    bool GetSocketType() {
        int sock_type = 0;
        int optlen = sizeof(sock_type);

        if (getsockopt(m_socket, SOL_SOCKET, SO_TYPE, (char*)&sock_type, &optlen) == 0) {
            if (sock_type == SOCK_STREAM) {
                std::cout << "Тип сокета (getsockopt): SOCK_STREAM (TCP)" << std::endl;
            }
            else if (sock_type == SOCK_DGRAM) {
                std::cout << "Тип сокета (getsockopt): SOCK_DGRAM (UDP)" << std::endl;
            }
            else {
                std::cout << "Тип сокета (getsockopt): " << sock_type << " (неизвестный)" << std::endl;
            }
            return true;
        }
        else {
            std::cerr << "getsockopt(SO_TYPE) failed: " << WSAGetLastError() << std::endl;
            return false;
        }
    }

    /* Получение реального порта (если был указан 0) */
    bool GetActualPort() {
        struct sockaddr_in addr;
        int addrlen = sizeof(addr);

        if (getsockname(m_socket, (struct sockaddr*)&addr, &addrlen) == 0) {
            std::cout << "Реальный порт (getsockname): " << ntohs(addr.sin_port) << std::endl;
            return true;
        }
        else {
            std::cerr << "getsockname failed: " << WSAGetLastError() << std::endl;
            return false;
        }
    }

    /* Закрытие сокета */
    void Close() {
        if (m_socket != INVALID_SOCKET) {
            if (closesocket(m_socket) == SOCKET_ERROR) {
                std::cerr << "closesocket failed: " << WSAGetLastError() << std::endl;
            }
            else {
                std::cout << "\nСокет закрыт. Дескриптор освобождён." << std::endl;
            }
            m_socket = INVALID_SOCKET;
        }
    }

    SOCKET GetHandle() const { return m_socket; }
};

/* Функция вывода использования */
void print_usage(const std::string& prog_name) {
    std::cout << "Использование: " << prog_name << " <domain> <type> <ip> <port> [--bind]" << std::endl;
    std::cout << "\nАргументы:" << std::endl;
    std::cout << "  domain  - семейство адресов: 0=AF_INET (IPv4), 1=AF_INET6 (IPv6)" << std::endl;
    std::cout << "  type    - тип сокета: 0=SOCK_STREAM (TCP), 1=SOCK_DGRAM (UDP)" << std::endl;
    std::cout << "  ip      - IP-адрес (например: 127.0.0.1 или ::1)" << std::endl;
    std::cout << "  port    - номер порта (0-65535, 0=случайный свободный)" << std::endl;
    std::cout << "  --bind  - выполнить привязку сокета к адресу" << std::endl;
    std::cout << "\nПример:" << std::endl;
    std::cout << "  " << prog_name << " 0 0 127.0.0.1 8080 --bind" << std::endl;
}

/* Функция валидации аргументов командной строки */
bool validate_args(int argc, char* argv[], int& domain, int& type,
    std::string& ip, int& port, bool& do_bind) {

    if (argc < 5) {
        std::cerr << "Ошибка: недостаточно аргументов" << std::endl;
        print_usage(argv[0]);
        return false;
    }

    /* Валидация domain */
    try {
        domain = std::atoi(argv[1]);
    }
    catch (...) {
        std::cerr << "Ошибка: неверный формат domain" << std::endl;
        return false;
    }
    if (domain != DOMAIN_AF_INET && domain != DOMAIN_AF_INET6) {
        std::cerr << "Ошибка: неверный домен. Должен быть 0 (IPv4) или 1 (IPv6)" << std::endl;
        return false;
    }

    /* Валидация type */
    try {
        type = std::atoi(argv[2]);
    }
    catch (...) {
        std::cerr << "Ошибка: неверный формат type" << std::endl;
        return false;
    }
    if (type != TYPE_STREAM && type != TYPE_DGRAM) {
        std::cerr << "Ошибка: неверный тип сокета. Должен быть 0 (TCP) или 1 (UDP)" << std::endl;
        return false;
    }

    /* Валидация IP */
    ip = argv[3];

    /* Проверка формата IP (базовая) */
    if (ip.empty()) {
        std::cerr << "Ошибка: пустой IP-адрес" << std::endl;
        return false;
    }

    /* Валидация порта */
    try {
        port = std::atoi(argv[4]);
    }
    catch (...) {
        std::cerr << "Ошибка: неверный формат порта" << std::endl;
        return false;
    }
    if (port < 0 || port > 65535) {
        std::cerr << "Ошибка: порт должен быть в диапазоне 0-65535" << std::endl;
        return false;
    }

    /* Проверка флага --bind */
    do_bind = (argc > 5 && std::strcmp(argv[5], "--bind") == 0);

    return true;
}

/* Функция инициализации Winsock */
bool init_winsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
        return false;
    }
    return true;
}

/* Функция завершения Winsock */
void cleanup_winsock() {
    WSACleanup();
}

int main(int argc, char* argv[]) {
    int domain, type, port;
    std::string ip;
    bool do_bind;
    SocketManager socket_mgr;

    SetConsoleOutputCP(1251);

    std::cout << "=== Лабораторная работа №1 - Продвинутый уровень, Вариант 13 ===" << std::endl;
    std::cout << std::endl;

    /* Шаг 1: Инициализация Winsock */
    std::cout << "Шаг 1: Инициализация Winsock" << std::endl;
    if (!init_winsock()) {
        return EXIT_FAILURE;
    }
    std::cout << "Winsock инициализирован успешно" << std::endl;
    std::cout << std::endl;

    /* Шаг 2: Валидация аргументов */
    std::cout << "Шаг 2: Валидация аргументов командной строки" << std::endl;
    if (!validate_args(argc, argv, domain, type, ip, port, do_bind)) {
        cleanup_winsock();
        return EXIT_FAILURE;
    }
    std::cout << "Аргументы валидированы: domain=" << domain
        << ", type=" << type << ", ip=" << ip
        << ", port=" << port << ", bind=" << (do_bind ? "1" : "0") << std::endl;
    std::cout << std::endl;

    /* Шаг 3: Создание сокета */
    std::cout << "Шаг 3: Создание сокета" << std::endl;
    if (!socket_mgr.Create(domain, type)) {
        cleanup_winsock();
        return EXIT_FAILURE;
    }
    std::cout << std::endl;

    /* Шаг 4: Вывод информации о сокете через getsockopt(SO_TYPE) - ВАРИАНТ 13 */
    std::cout << "Шаг 4: Вывод типа сокета через getsockopt(SO_TYPE)" << std::endl;
    socket_mgr.GetSocketType();
    std::cout << std::endl;

    /* Шаг 5: Привязка сокета (если указан флаг --bind) */
    if (do_bind) {
        std::cout << "Шаг 5: Привязка сокета к адресу " << ip << ":" << port << std::endl;
        if (!socket_mgr.Bind(domain, ip, port)) {
            socket_mgr.Close();
            cleanup_winsock();
            return EXIT_FAILURE;
        }

        /* Если порт был 0, показать реальный порт */
        if (port == 0) {
            std::cout << "\nПорт 0: выбран случайный свободный порт" << std::endl;
            socket_mgr.GetActualPort();
        }
        std::cout << std::endl;
    }

    /* Шаг 6: Вывод информации о сокете */
    std::cout << "Шаг 6: Вывод информации о сокете" << std::endl;
    std::cout << "--- Информация о сокете ---" << std::endl;
    std::cout << "Семейство адресов: " << ((domain == DOMAIN_AF_INET) ? "AF_INET (IPv4)" : "AF_INET6 (IPv6)") << std::endl;
    std::cout << "Тип сокета: " << ((type == TYPE_STREAM) ? "SOCK_STREAM (TCP)" : "SOCK_DGRAM (UDP)") << std::endl;
    std::cout << "IP-адрес: " << ip << std::endl;
    std::cout << "Порт: " << port << std::endl;
    std::cout << std::endl;

    /* Шаг 7: Закрытие сокета */
    std::cout << "Шаг 7: Закрытие сокета" << std::endl;
    socket_mgr.Close();

    /* Шаг 8: Завершение Winsock */
    cleanup_winsock();

    std::cout << "\nПрограмма завершена успешно." << std::endl;
    return EXIT_SUCCESS;
}