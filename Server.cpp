
#include "Server.h"
#include "CommandsManager.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sstream>
#include <strings.h>


using namespace std;

#define THREADS_NUM 5
#define MAX_CONNECTED_CLIENTS 10
#define MAX_COMMAND_LEN 50

static void *acceptClients(void *);
static void *handleClient(void *);

Server::Server(int port): port(port), serverSocket(0), serverThreadId(0),pool(THREADS_NUM) {
    cout << "Server" << endl;
}

void Server::start() {
    // Create a socket point
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        throw "Error opening socket";
    }
    // Assign a local address to the socket
    struct sockaddr_in serverAddress;
    bzero((void *)&serverAddress, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1) {
        throw "Error on binding";
    }
    // Start listening to incoming connections
    listen(serverSocket, MAX_CONNECTED_CLIENTS);
    pthread_create(&serverThreadId,NULL,&acceptClients,(void *)this);
}

static void * acceptClients(void *server1) {
    Server *server=(Server*)server1;
    long serverSocket=(long)server->getServerSocket();

    // Define the client socket's structures
    struct sockaddr_in clientAddress;
    socklen_t clientAddressLen=sizeof(clientAddress);

    while (true) {
        cout<<"waiting for client connections..."<<endl;

        //Accept a new client connection
        long clientSocket=accept(serverSocket,(struct sockaddr*)&clientAddress,&clientAddressLen);
        cout<<"client connected"<<endl;
        if (clientSocket == -1)
            throw "Error on accept";

        Task *task=new Task(handleClient,(void *)clientSocket);
        server->addTaskToVector(task);
        server->addTaskToPool(task);
    }
}

static void *handleClient(void *socket) {
    long clientSocket=(long)socket;
    char commandStr[MAX_COMMAND_LEN];

    //Read the command from the socket
    int n =read(clientSocket,commandStr,sizeof(commandStr));
    if (n == -1){
        cout<<"Error reading command"<<endl;
        return NULL;
    }
    if (n == 0){
        cout << "Client disconnected" << endl;
        return NULL;
    }
    cout<<"Received command ";

    //Split the command string to the command name and the arguments
    string str(commandStr);
    cout<<str<<endl;
    istringstream stream(str);
    string command;
    stream >> command;
    vector<string> args;
    while(stream){
        string arg;
        stream >> arg;
        args.push_back(arg);
    }

    CommandsManager::getInstance()->executeCommand(command,args,clientSocket);
    return NULL;
}

void Server::stop() {
    vector<string> args;
    string command="close_server";
    CommandsManager *commandsManager= CommandsManager::getInstance();
    commandsManager->executeCommand(command,args,serverSocket);
    delete commandsManager;
    pool.terminate();
    for (vector<Task*>::iterator it = taskVec.begin() ; it != taskVec.end(); ++it){
        delete *it;
    }
    pthread_cancel(serverThreadId);
    cout<<"Server stopped"<<endl;
}

int Server::getServerSocket() const {
    return serverSocket;
}

void Server::addTaskToVector(Task *task) {
    taskVec.push_back(task);
}

void Server::addTaskToPool(Task *task) {
    pool.addTask(task);
}


