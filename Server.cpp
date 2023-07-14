#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <map>
#include <thread>
#include <mutex>

void addPollOptions();
void printPollChoice();
int processPollOptions();
int getNumberOfClients();
void acceptConnection(int);
void communicateWithServer(int);
void printResult(int, int);
std::string prepareMessageToSend();
void sendMessage(int, std::string);
void receiveMessage(int);
void addPollCount(std::string);
void increaseCount(std::string, int);

std::map<std::string, int> poll;
std::map<std::string, std::mutex> poll_mutexes;
int number_of_clients, clients_participated = 0;

int main() {

  int server_socket, client_socket, port_number;
  struct sockaddr_in server_address;

  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0) {
    std::cerr << "Error opening socket.\n";
    exit(1);
  }

  std::memset(&server_address, 0, sizeof(server_address));
  port_number = 8080;
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(port_number);

  if (bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
      std::cerr << "Error on binding.\n";
      exit(1);
  }

  //add polling choices
  addPollOptions();

  number_of_clients = getNumberOfClients();

  //listen and accept client connections
  acceptConnection(server_socket);

  printResult(number_of_clients, clients_participated);
  close(server_socket);

}

void addPollOptions() {

  int optionCount = 0;
  std::cout << "Enter the choices for poll" << std::endl;
  std::string new_choice;

  while(true) {
    int user_input;
    std::cout << "Option " << ++optionCount << std::endl;
    std::getline(std::cin >> std::ws, new_choice);
    poll[new_choice];
    std::cout << "Enter 1 to add Choice" << std::endl;
    std::cout << "Enter 0 to continue" << std::endl;
    std::cin >> user_input;
    if(user_input == 0) {
        printPollChoice();
        break;
    }
    else if(user_input < 1 || user_input > 1) {
        user_input = processPollOptions();
        if(user_input == 0) {
            std::cout << "Program terminated by user." << std::endl;
            exit(1);
        }
        else if(user_input == 2) {
            //for adding new options
        }
        else {
            break;
        }
    }
  }
}

int processPollOptions() {
    int loopControl;
    std::cout << std::endl;
    std::cout << "Do you want to continue :\n";
    std::cout << "Enter 1 for continue with options\n";
    std::cout << "Enter 2 for adding options\n";
    std::cout << "Enter 0 for exit the process\n";
    std::cin >> loopControl;
    return loopControl;
}

void printPollChoice() {
    int count = 0;
    std::cout << "Poll Choices: " << std::endl;
    for(auto it: poll) {
        std::cout << ++count << " - " << it.first << std::endl;
    }
}

int getNumberOfClients() {

    int clients_count = 0;
    std::cout << "Enter number of clients to participate.." << std::endl; 
    std::cin >> clients_count;
    if(clients_count > 0) {
        return clients_count;
    }
    else {
        std::cout << "Enter a Valid Input" << std::endl;
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        return getNumberOfClients();
    }
}

void acceptConnection(int server_socket) {

    int client_socket;
    struct sockaddr_in client_address;
    socklen_t client_length;

    if((listen(server_socket, 5)) == -1) {
      std::cerr << "Error in listening.\n";
    }
    while(true) {
        client_length = sizeof(client_address);
        client_socket = accept(server_socket, (struct sockaddr *) &client_address, &client_length);
        clients_participated++;
        if(clients_participated > number_of_clients) {
            sendMessage(client_socket, "Poll has ended.");
            clients_participated--;
            close(client_socket);
            break;
        }
        std::thread clientThread(communicateWithServer, client_socket);
        if(clientThread.joinable()) {
            clientThread.detach();
        }
    }
}

void communicateWithServer(int client_socket) {

    if (client_socket < 0) {
      std::cerr << "Error on accept.\n";
      exit(1);
    }
    std::string message = prepareMessageToSend();
    sendMessage(client_socket, message);
    receiveMessage(client_socket);
    close(client_socket);
}

std::string prepareMessageToSend() {

    std::string message_to_send = "Poll your choice: ";
    for(auto it : poll) {
        message_to_send.append("\n" + it.first);
    }
    return message_to_send;
}

void sendMessage(int client_socket, std::string message_to_send) {
   
    int bytes_read = send(client_socket, message_to_send.c_str(), message_to_send.size(), 0);
    if (bytes_read < 0) {
        std::cerr << "Error in sending message to client socket.\n";
    }
}

void receiveMessage(int client_socket) {
    int bytes_read;
    std::string incoming_message;
    bytes_read = read(client_socket, &incoming_message, 255);
        if (bytes_read < 0) {
            std::cerr << "Error reading from socket.\n";
            close(client_socket);
        }
        else {
            std::cout << "Client choice : " << incoming_message << std::endl;
            addPollCount(incoming_message);
        }
}

void addPollCount(std::string incoming_message) {
    
    if(poll.find(incoming_message) != poll.end()) {
        int count = poll[incoming_message];
        increaseCount(incoming_message, ++count);
    }
}

void increaseCount(std::string incoming_message, int count) {
    
    std::lock_guard<std::mutex> lock(poll_mutexes[incoming_message]);
    poll[incoming_message] = count;
}

void printResult(int clients, int participants) {

    std::cout << "\nResult: " << std::endl;
    std::cout << "Total number of participants: " << clients << std::endl;
    std::cout << "Participated clients: " << participants << std::endl;
    for(auto it : poll) {
        std::cout << it.first << " " << it.second << std::endl;
    }
}






    



    