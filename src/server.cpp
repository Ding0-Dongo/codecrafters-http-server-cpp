#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fstream>
#include <sstream>
#include <thread>

void http_request(int client_fd, std::string dir){
  std::string incomingMessage(1024, '\0');
  std::string contentStr = "";
  std::string OkMessage = "HTTP/1.1 200 OK\r\n\r\n";
  std::string errMessage = "HTTP/1.1 404 Not Found\r\n\r\n";

  recv(client_fd, (void *)&incomingMessage[0], incomingMessage.max_size(), 0);

  std::cout << incomingMessage;
  std::cout << "\n";
  if(incomingMessage.starts_with("POST /files/")){
    auto tempPath = incomingMessage.substr(11);
    std::string path = tempPath.substr(0, tempPath.find(" "));
    std::ofstream outputFile(dir + path);
    int messageSize = stoi(incomingMessage.substr(incomingMessage.find("Content-Length:") + 16, 2));
    std::string fileMessage = incomingMessage.substr(incomingMessage.find("\r\n\r\n") + 4, messageSize);
    outputFile << fileMessage;
    outputFile.close();
    std::string postMessage = "HTTP/1.1 201 Created\r\n\r\n";
    send(client_fd, postMessage.c_str(), postMessage.length(), 0);;
  }
  else if(incomingMessage.starts_with("GET /files/")){
    auto tempPath = incomingMessage.substr(11);
    std::string path = tempPath.substr(0, tempPath.find(" "));
    std::ifstream file(dir + path);
    if (file.good()) {
      std::string fileMessage = "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: ";
      std::stringstream fileContent;
      fileContent << file.rdbuf();
      std::stringstream respond("");
      fileMessage = fileMessage + std::to_string(fileContent.str().length()) + "\r\n\r\n" + fileContent.str() + "\r\n";
      send(client_fd, fileMessage.c_str(), fileMessage.length(), 0);;
    } else{
      std::cout << "error encountered";
      send(client_fd, errMessage.c_str(), errMessage.length(), 0);;
    }
  }
  else if(incomingMessage.starts_with("GET /user-agent HTTP/1.1\r\n")){
    int startOfStr = incomingMessage.find("User-Agent: ") + 12;
    int endOfStr = incomingMessage.find("\r\n", startOfStr);
    contentStr = incomingMessage.substr(startOfStr, endOfStr - startOfStr);
    std::string message = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(contentStr.size()) + "\r\n\r\n"+ contentStr;
    send(client_fd, message.c_str(), message.length(), 0);
  }
  else if(incomingMessage.starts_with("GET /echo/")){
    int endOfStr = incomingMessage.find("HTTP/1.1");
    contentStr = incomingMessage.substr(10, endOfStr - 11);
    std::string message = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(contentStr.size()) + "\r\n\r\n"+ contentStr;
    send(client_fd, message.c_str(), message.length(), 0);
  }
  else if(incomingMessage.starts_with("GET / HTTP/1.1\r\n")){
    send(client_fd, OkMessage.c_str(), OkMessage.length(), 0);
  }
  else{
    send(client_fd, errMessage.c_str(), errMessage.length(), 0);
  }
}

int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;


  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    std::cerr << "Failed to create server socket\n";
    return 1;
  }
  
  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
  
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }
  
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }
  
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);

  std::string dir;
  if (argc == 3 && strcmp(argv[1], "--directory") == 0)
  {
  	dir = argv[2];
  }
  
  std::cout << "Waiting for a client to connect...\n";


  while(true){
    int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
    std::cout << "Client connected\n";
    std::thread clientThread(http_request, client_fd, dir);
    clientThread.detach();
  }


  close(server_fd);

  return 0;
}
