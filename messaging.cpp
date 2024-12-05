#include <iostream>
#include <thread>
#include <string>
#include <cstring>
#include <atomic>
#include <future>
#include <chrono>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


std::string ID = "1";


std::atomic<bool> running(true);

std::string computerMsg() {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::string msg = "Hello my name is " + ID;
    return msg;
}

void receiver(const std::string& host, int port, std::string my_own_ip) {
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

    std::cout << "Listening for broadcast messages on " << host << ":" << port << "..." << std::endl;

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
        std::string sender_ip = inet_ntoa(clientaddr.sin_addr);

        // Check if the sender is myself
        if (sender_ip == my_own_ip) {
            // This message originated from me, ignore it
            continue;
        }

        std::cout << "\nReceived broadcast message: " << data << " from "
                  << inet_ntoa(clientaddr.sin_addr) << ":" << ntohs(clientaddr.sin_port) << std::endl;
    }

    close(sockfd);
}

void sender(int port) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        return;
    }

    int broadcastEnable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0) {
        perror("Error enabling broadcast on socket");
        close(sockfd);
        return;
    }


    std::string broadcast_ip = "138.16.161.255";

    struct sockaddr_in broadcast_addr;
    std::memset(&broadcast_addr, 0, sizeof(broadcast_addr));

    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_addr.s_addr = inet_addr(broadcast_ip.c_str());
    broadcast_addr.sin_port = htons(port);

    std::cout << "Broadcasting computed values to " 
              << broadcast_ip << ":" << port << " whenever computations finish..." << std::endl;

    while (running) {
        auto future_value = std::async(std::launch::async, computerMsg);

        std::string message = future_value.get();

        ssize_t n = sendto(sockfd, message.c_str(), message.size(), 0,
                           (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
        if (n < 0) {
            perror("Error sending broadcast data");
            break;
        }

        std::cout << "Broadcasted message: " << message << " to " << broadcast_ip << ":" << port << std::endl;
    }

    close(sockfd);
}

int main() {
    int port = 5005;

    std::thread receiver_thread(receiver, "0.0.0.0", port, "138.16.161.194");

    sender(port);

    running = false;
    receiver_thread.join();

    return 0;
}
