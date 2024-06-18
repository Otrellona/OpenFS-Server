#include <SFML/Network.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include <memory>
#include <mutex>

std::mutex clientsMutex;
std::vector<std::unique_ptr<sf::TcpSocket>> clients;

void handleClient(sf::TcpSocket* client) {
    while (true) {
        sf::Packet packet;
        if (client->receive(packet) == sf::Socket::Disconnected) {
            std::cout << "Client disconnected: " << client->getRemoteAddress() << std::endl;
            break;
        }

        std::lock_guard<std::mutex> lock(clientsMutex);
        for (auto& otherClient : clients) {
            if (otherClient.get() != client) {
                otherClient->send(packet);
                client->send(packet);
            }
        }
    }

    std::lock_guard<std::mutex> lock(clientsMutex);
    clients.erase(std::remove_if(clients.begin(), clients.end(), [client](const std::unique_ptr<sf::TcpSocket>& ptr) {
        return ptr.get() == client;
        }), clients.end());
}

int main() {
    sf::TcpListener listener;
    listener.setBlocking(false);

    // Привязываем сокет к порту 54000
    if (listener.listen(54000) != sf::Socket::Done) {
        std::cerr << "Error listening on port 54000" << std::endl;
        return -1;
    }

    std::cout << "Server is listening on port 54000" << std::endl;

    while (true) {
        auto client = std::make_unique<sf::TcpSocket>();
        if (listener.accept(*client) == sf::Socket::Done) {
            std::lock_guard<std::mutex> lock(clientsMutex);
            std::cout << "Client connected: " << client->getRemoteAddress() << std::endl;
            clients.push_back(std::move(client));
            std::thread(handleClient, clients.back().get()).detach();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}
