#include "Chat_Client.h"
int client_mode = 0; //INFO 0 WAIT 1 CHAT 2
int chatsock = 0;
int waitsock = 0;
int serversock = 0;

char my_id[MAX_ID_LEN];
char guest_id[MAX_ID_LEN];

volatile sig_atomic_t flag = 0;
static void my_handler(int sig){ // can be called asynchronously
    if(client_mode == 1){
        shutdown(waitsock, 2);
        printf("\nStopped waiting.\n");
        printf("%s> ", my_id);
        fflush(stdout);
    }
    if(client_mode == 2){
        shutdown(chatsock, 2);
        printf("\nLeft conversation.\n");
        printf("%s> ", my_id);
        fflush(stdout);
    }
    
    client_mode = 0; // reset mode to info
}


int RequstList(int sockserver);

int RequstWait(int sockserver);

int RequstConnect(int sockserver, const char* connect_id);

int StartChat(int chatsock);

void *recvThread(void *arg);

 void *listenThread(void *arg);

int RequstList(int sockserver){
    char command[10] = "/list";
    if(send(sockserver, command, 10, 0) < 0){
        printf("send list request error: %s(errno: %d)\n", strerror(errno), errno);
        return -1;
    }
    char client_id[MAX_ID_LEN];
    int count = 1;
    while(recv(sockserver, client_id, MAX_ID_LEN, 0) > 0){
        if(client_id[0] == '\n'){
            break;
        }
        printf("%d) %s\n", count, client_id);
        ++count;
        memset(client_id, 0, MAX_ID_LEN);
    }
    return 1;
 }

 int RequstWait(int sockserver){

    // Start a char server
    // Create new listen socket
    // int waitsock = 0;
    struct sockaddr_in  servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = 0;
    // Socket create
    if( (waitsock = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){
        printf("Create socket error: %s(errno: %d)\n",strerror(errno),errno);
        return -1;
    }
    //bind socket
    if( bind(waitsock, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
        printf("Bind socket error: %s(errno: %d)\n",strerror(errno),errno);
        return -1;
    }
    //listenning
    if( listen(waitsock, 10) == -1){
        printf("Listen socket error: %s(errno: %d)\n",strerror(errno),errno);
        return -1;
    }

    // Get new port number
    struct sockaddr_in connAddr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    if(getsockname(waitsock, (sockaddr*)&connAddr, &addrlen) != 0){
        return -1;
    }
    int new_port = ntohs(connAddr.sin_port);

    // Send new port to server
    char send_buffer[20];
    sprintf(send_buffer,"/wait %d",new_port);
    // printf("Send new port:%d\n", new_port);
    if(send(sockserver, send_buffer, 20, 0) < 0){
        printf("send list request error: %s(errno: %d)\n", strerror(errno), errno);
        return -1;
    }
    int connfd = 0;
    // waitsock = waitsock;

    // Create new thread to listen
    pthread_t thid;
    struct ThreadAttri Attri;
    struct ThreadAttri *ptAttri = &Attri;
    
    ptAttri->sockclient = waitsock;

    client_mode = 1; // to wait state
    if (pthread_create(&thid,NULL,listenThread,(void*)ptAttri) == -1){
        printf("Thread create error!\n");
        return -1;
    }



    return 1;
 }

int RequstConnect(int sockserver, const char* connect_id){
    char command[MAX_ID_LEN + 10];
    sprintf(command, "/connect %s", connect_id);
    if(send(sockserver, command, MAX_ID_LEN + 10, 0) < 0){
        //printf("send list request error: %s(errno: %d)\n", strerror(errno), errno);
        return -1;
    }
    char recv_buffer[BUFFER_SIZE];
    if(recv(sockserver, recv_buffer, BUFFER_SIZE, 0) > 0){
        if(recv_buffer[0] == '\n'){
            printf("ID not found in waiting list!\n");
            return 1;
        }
        printf("connection address: %s\n",recv_buffer);
    }else{
        return -1;
    }
    //connect to client 
    char address[20];
    char port[10];
    sscanf(recv_buffer, "%s %s", address, port);

    // int chatsock = 0;
    struct sockaddr_in serv_addr; 
    // Create socket
    if ((chatsock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        printf("Socket creation error %s(errno: %d)\n", strerror(errno),errno);
        return -1; 
    }else{
        printf("Socket created \n"); 
    }
    // Create socket address
    memset(&serv_addr, '0', sizeof(serv_addr)); 
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(atoi(port)); 

    // Convert IPv4 and IPv6 addresses from text to binary form 
    if(inet_pton(AF_INET, address, &serv_addr.sin_addr)<=0)  
    { 
        printf("\n Invalid address: %s\n",address);
        return -1; 
    } 
    // Socket connection 
    if (connect(chatsock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    { 
        printf("\n Connection Failed %s(errno: %d)\n",strerror(errno),errno);
        return -1; 
    }
    StartChat(chatsock);

    return 1;
 }

int StartChat(int connfd){
    // Create new thread to recv msg
    pthread_t thid;
    struct ThreadAttri Attri;
    struct ThreadAttri *ptAttri = &Attri;
    
    ptAttri->sockclient = connfd;


    if (pthread_create(&thid,NULL,recvThread,(void*)ptAttri) == -1){
        printf("Thread create error!\n");
        return -1;
    }
    //send id exchange with each other
    if(send(connfd, my_id, MAX_ID_LEN, 0) < 0){
        printf("send my id error: %s(errno: %d)\n", strerror(errno), errno);
        return -1;
    }
    client_mode = 2; //switch chat mode
    return 1;
 }

void *recvThread(void *arg){
    struct ThreadAttri *temp;
    temp = (struct ThreadAttri *)arg;
    // int sockclient = temp->sockclient;
    int sockclient = chatsock;
    char recv_buffer[BUFFER_SIZE];
    //recv chat id 
    char chat_id[MAX_ID_LEN];
    if(recv(sockclient, chat_id, MAX_ID_LEN, 0) < 0){
        printf("Recv id error: %s(errno: %d)",strerror(errno),errno);
        pthread_exit((void*)-1);
    }
    printf("\nConnection from %s\n",chat_id);
    printf("%s> ", my_id);
    fflush(stdout);
    memset(guest_id, 0, MAX_ID_LEN);
    strcpy(guest_id,chat_id); 
    //recv msg
    while(recv(sockclient, recv_buffer, BUFFER_SIZE, 0) > 0){
        printf("\n%s: %s\n",chat_id, recv_buffer);
        printf("%s> ", my_id);
        fflush(stdout);
    }
    printf("\nLeft conversation with %s\n",chat_id);
    printf("%s> ", my_id);
    fflush(stdout);
    client_mode = 0;
    pthread_exit((void*)1);
 }

 void *listenThread(void *arg){
    struct ThreadAttri *temp;
    temp = (struct ThreadAttri *)arg;
    int sockclient = temp->sockclient;
    char recv_buffer[BUFFER_SIZE];

    struct timeval timeout={2,0};
    fd_set  rset;
    
    // printf("selsect start!\n");
    while(1){
        FD_ZERO(&rset);
        FD_SET(waitsock, &rset);
        if(client_mode == 0){
            char command[BUFFER_SIZE] = "/left";
            if(send(serversock, command, BUFFER_SIZE, 0) < 0){
                printf("send list request error: %s(errno: %d)\n", strerror(errno), errno);
            }
            //printf("%s> ", my_id);
            fflush(stdout);
            break;
        }
        int ret = select(waitsock + 1, &rset, NULL, NULL,&timeout); 
        if(ret == 0){
            // printf("timeout listen\n");
            sleep(1);
            continue;
        }else if(ret > 0 && client_mode == 1){
            FD_CLR(waitsock,&rset);
            int connfd = accept(waitsock, (struct sockaddr*)NULL, NULL);
            // printf("Selected!!!\n");
            chatsock = connfd;
            if(StartChat(chatsock) < 0){
                printf("Chat error: %s(errno: %d)",strerror(errno),errno);
            }
            break;
        }else if(ret < 0){
            printf("selsect error\n");
            pthread_exit((void*)-1);
        }
    }
    // printf("selsect end\n");
    pthread_exit((void*)1);
 }

int main(int argc, char const *argv[]) 
{ 
    signal(SIGINT, my_handler);
    int sockserver = 0;
    struct sockaddr_in serv_addr; 
    char buffer[BUFFER_SIZE] = {0}; 
    char* sendline;
    if(argc != 4){
        printf("usage: ./client <ipaddress> <port> <id>\n");
        return 0;
    }

    // Varify port
    if(!portVarify(argv[2])){
        printf("\n Port invalid\n"); 
        return 0;
    }

    // Varify id
    if(strlen(argv[3]) > MAX_ID_LEN){
        printf("\n ID too long\n"); 
        return 0;
    }
    strcpy(my_id, argv[3]);
    // Create socket
    if ((sockserver = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        printf("Socket creation error %s(errno: %d)\n", strerror(errno),errno);
        return -1; 
    }else{
        printf("Socket created \n"); 
    }
    // Create socket address
    memset(&serv_addr, '0', sizeof(serv_addr)); 
    int port_num = atoi(argv[2]);
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(port_num); 

    // Convert IPv4 and IPv6 addresses from text to binary form 
    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0)  
    { 
        printf("\n Invalid address: %s\n",argv[1]);
        return -1; 
    } 
    // Socket connection 
    if (connect(sockserver, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    { 
        printf("\n Connection Failed %s(errno: %d)\n",strerror(errno),errno);
        return -1; 
    }
    char connectbuffer[MAX_ID_LEN];
    strcpy(connectbuffer, argv[3]);
    //init message
    if (send(sockserver, connectbuffer, BUFFER_SIZE, 0) < 0)
    {
        printf("Send conection message error: %s(errno: %d)\n", strerror(errno), errno);
    }
    serversock = sockserver;
    char cstrin[BUFFER_SIZE];
    // string strin;
    printf("%s> ", my_id);
    while(cin.getline(cstrin, BUFFER_SIZE)){
        //get the command
        string strin(cstrin);
        int spc_pos = 0;
        string connect_id = "";
        if(strin == "/quit"){
                break;
        }
        if(strin[0] == '/' && client_mode == 0){ //is a command and in info state
            if((spc_pos = strin.find(' ')) > 0){
                connect_id = strin.substr(spc_pos + 1, strin.size() - spc_pos - 1);
                strin = strin.substr(0, spc_pos);
            }
            if(strin =="/list"){
                //list function
                printf("show list\n");
                RequstList(sockserver);
            }else if(strin =="/wait"){
                //wait function TODO
                printf("waiting for connection.\n");
                if(RequstWait(sockserver) < 0){
                    printf("Wait error: %s(errno: %d)\n", strerror(errno), errno);
                }
            }else if(strin =="/connect"){
                //connect function TODO * need id
                printf("connect to %s...\n", connect_id.c_str());
                RequstConnect(sockserver, connect_id.c_str());
            }else{
                printf("No such command!\n");
            }
        }else if(client_mode == 2){ //not command and in chat mode send message
            if(send(chatsock, cstrin, BUFFER_SIZE, 0) < 0){
                printf("send my id error: %s(errno: %d)\n", strerror(errno), errno);
            }
        }
        printf("%s> ", my_id);
        // clear strin
        strin = "";
    }
    close(sockserver);
    return 0;

}