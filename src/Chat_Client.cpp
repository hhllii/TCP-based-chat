#include "Chat_Client.h"

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
    char command[10] = "/wait";
    if(send(sockserver, command, 10, 0) < 0){
        //printf("send list request error: %s(errno: %d)\n", strerror(errno), errno);
        return -1;
    }
    // start a char server
    struct sockaddr_in  servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = 0;

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
        //TODO connect to client 
    }else{
        return -1;
    }
    return 1;
 }

int main(int argc, char const *argv[]) 
{ 
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
    char cstrin[BUFFER_SIZE];
    // string strin;
    cout << '>';
    while(cin.getline(cstrin, BUFFER_SIZE)){
        //get the command
        string strin(cstrin);
        int spc_pos = 0;
        string connect_id = "";
        if(strin[0] == '/'){ //is a command
            if((spc_pos = strin.find(' ')) > 0){
                connect_id = strin.substr(spc_pos + 1, strin.size() - spc_pos - 1);
                strin = strin.substr(0, spc_pos);
            }
            if(strin =="/list"){
                //list function TODO
                printf("show list\n");
                RequstList(sockserver);
            }else if(strin =="/wait"){
                //wait function TODO
                printf("waiting for connection\n");
                RequstWait(sockserver);
            }else if(strin =="/connect"){
                //connect function TODO * need id
                printf("connect to %s...\n", connect_id.c_str());
                RequstConnect(sockserver, connect_id.c_str());
            }else if(strin =="/quit"){
                break;
            }
        }else{ // message send 
            cout << '<' <<strin << endl;
        }
        cout << '>';
        // clear strin
        strin = "";
    }
    close(sockserver);
    return 0;

}