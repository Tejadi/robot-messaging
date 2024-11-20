#include <iostream>
#include <thread>
#include <string>
#include <cstring>
#include <atomic>
#include <unistd.h>    
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

std::atomic<bool> running(true);

void udp_receive(const std::string& host, int port) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        return;
    }

    struct sockaddr_in servaddr;
    std::memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    if (host == "0.0.0.0") {
        servaddr.sin_addr.s_addr = INADDR_ANY;
    } else {
        servaddr.sin_addr.s_addr = inet_addr(host.c_str());
    }
    servaddr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("Error binding socket");
        close(sockfd);
        return;
    }

    std::cout << "Listening for messages on " << host << ":" << port << "..." << std::endl;

    while (running) {
        char msg[4096];
        struct sockaddr_in clientaddr;
        socklen_t addrlen = sizeof(clientaddr);

        ssize_t n = recvfrom(sockfd, msg, sizeof(msg) - 1, 0, (struct sockaddr*)&clientaddr, &addrlen);
        if (n < 0) {
            perror("Error receiving data");
            break;
        }

        msg[n] = '\0';
        std::string data(msg);

        std::cout << "\nReceived message: " << data << " from "
                  << inet_ntoa(clientaddr.sin_addr) << ":" << ntohs(clientaddr.sin_port) << std::endl;
    }

    close(sockfd);
}

void udp_send(const std::string& target_ip, int port) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        return;
    }

    struct sockaddr_in servaddr;
    std::memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(target_ip.c_str());
    servaddr.sin_port = htons(port);

    while (true) {
        std::string message;
        std::cout << "Enter message to send (or 'exit' to quit): ";
        std::getline(std::cin, message);
        if (message == "exit" || message == "Exit") {
            std::cout << "Exiting..." << std::endl;
            running = false;  // Signal the receiver to stop
            break;
        }

        ssize_t n = sendto(sockfd, message.c_str(), message.size(), 0, (struct sockaddr*)&servaddr, sizeof(servaddr));
        if (n < 0) {
            perror("Error sending data");
            break;
        }

        std::cout << "Sent message to " << target_ip << ":" << port << std::endl;
    }

    close(sockfd);
}

int main() {
    int port = 5005;
    std::string target_ip;

    std::cout << "Enter the target computer's IP address: ";
    std::getline(std::cin, target_ip);

    std::thread receiver_thread(udp_receive, "0.0.0.0", port);

    udp_send(target_ip, port);

    receiver_thread.join();

    return 0;
}
