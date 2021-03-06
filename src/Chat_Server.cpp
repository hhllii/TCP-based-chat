#include "Chat_Server.h"

vector<Client_Info> *client_list = new vector<Client_Info>();

int SendClientList(int sockclient);

int AddWaitList(Client_Info client_info, int sockclient, int new_port);

int SendClientInfo(int sockclient, const char* connect_id);

void RemoveFromWait(const char* client_id);

int HandleClientRequest(int sockclient, Client_Info client_info);

void *serverThread(void *arg);

pthread_mutex_t clist_lock;

int SendClientList(int sockclient){
    //lock and read the client 
    pthread_mutex_lock(&clist_lock);
    char send_buffer[MAX_ID_LEN];
    for(auto c : *client_list){
        char* client_id = c.id;
        memset(send_buffer, 0, MAX_ID_LEN);
        strcpy(send_buffer, client_id);
        if(send(sockclient, send_buffer, MAX_ID_LEN, 0) < 0){
            //printf("send list error: %s(errno: %d)\n", strerror(errno), errno);
            pthread_mutex_unlock(&clist_lock);
            return -1;
        }
    }
    if(send(sockclient, "\n", 1, 0) < 0){
        //printf("send list error: %s(errno: %d)\n", strerror(errno), errno);
        pthread_mutex_unlock(&clist_lock);
        return -1;
    }
    pthread_mutex_unlock(&clist_lock);
    return 1;
}

int AddWaitList(Client_Info client_info, int sockclient, int new_port){
    // update port 
    printf("Add waiting port for %s: %d\n", client_info.id, new_port);
    client_info.port = new_port;
    //add to wait list 
    int find = 0;
    pthread_mutex_lock(&clist_lock);
    for(vector<Client_Info>::iterator it = client_list->begin(); it != client_list->end(); it++){
        if(it == client_list->end()){
            break;
        }
		if(strcmp(it->id, client_info.id) == 0){
            find = 1;
            *it = client_info;
            break;
        }
    }
    if(find == 0){
        client_list -> push_back(client_info);
    }
    pthread_mutex_unlock(&clist_lock);
    return 1;
}

int SendClientInfo(int sockclient, const char* connect_id){
    pthread_mutex_lock(&clist_lock);
    int idx = 0;
    for(auto c : *client_list){
        if(strcmp(c.id, connect_id) == 0){
            char send_buffer[BUFFER_SIZE];
            sprintf(send_buffer, "%s %d", c.address, c.port);
            if(send(sockclient, send_buffer, BUFFER_SIZE, 0) < 0){
                //printf("send info error: %s(errno: %d)\n", strerror(errno), errno);
                pthread_mutex_unlock(&clist_lock);
                return -1;
            }
            client_list->erase(client_list->begin() + idx);
            pthread_mutex_unlock(&clist_lock);
            return 1;
        }
        idx++;
    }
    //no match id
    if(send(sockclient, "\n", 1, 0) < 0){
        //printf("send info error: %s(errno: %d)\n", strerror(errno), errno);
        pthread_mutex_unlock(&clist_lock);
        return -1;
    }
    pthread_mutex_unlock(&clist_lock);
    
}

void RemoveFromWait(const char* client_id){
    pthread_mutex_lock(&clist_lock);
    int idx = 0;
    for(auto c : *client_list){
        if(strcmp(c.id, client_id) == 0){
            client_list->erase(client_list->begin() + idx);
            pthread_mutex_unlock(&clist_lock);
            return;
        }
        idx++;
    }
    pthread_mutex_unlock(&clist_lock);
}

int HandleClientRequest(int sockclient, Client_Info client_info){
    char reques_buffer[BUFFER_SIZE];
    while(recv(sockclient, reques_buffer, BUFFER_SIZE, 0) > 0){
        int spc_pos = 0;
        string connect_id;
        string strin(reques_buffer);
        if(strin[0] == '/'){ //is a command
            if((spc_pos = strin.find(' ')) > 0){ //split
                connect_id = strin.substr(spc_pos + 1, strin.size() - spc_pos - 1);
                strin = strin.substr(0, spc_pos);
            }
            if(strin =="/list"){
                printf("%s>show list\n", client_info.id);
                //send list to client 
                if(SendClientList(sockclient) < 0){
                    printf("send list error: %s(errno: %d)\n", strerror(errno), errno);
                    break;
                }
            }else if(strin =="/wait"){
                //wait function TODO
                printf("%s>wait for connection\n", client_info.id);
                int chat_port = atoi(connect_id.c_str());
                AddWaitList(client_info, sockclient, chat_port);
            }else if(strin =="/connect"){
                //connect function TODO * need id
                printf("%s>connect to %s...\n", client_info.id, connect_id.c_str());
                if(SendClientInfo(sockclient, connect_id.c_str()) < 0){
                    printf("send client info error: %s(errno: %d)\n", strerror(errno), errno);
                    break;
                }
            }else if(strin =="/left"){
                printf("%s>stop waiting\n", client_info.id);
                RemoveFromWait(client_info.id);
            }
        }
    }
    
}


void *serverThread(void *arg){
    struct ThreadAttri *temp;
    temp = (struct ThreadAttri *)arg;
    int sockclient = temp->sockclient;

    struct sockaddr_in Addclient;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    if(!getpeername(sockclient, (struct sockaddr *)&Addclient, &addrlen))
    {
        // printf( "client IP：%s \n", inet_ntoa(Addclient.sin_addr));
        // printf( "client PORT：%d \n", ntohs(Addclient.sin_port));
    }else{
        printf("Socket invalid\n");
    }

    // recv the first connection message
    char recv_id[MAX_ID_LEN];
    if(recv(sockclient, recv_id, MAX_ID_LEN, 0) <= 0){
        printf("Fail to connect with the client\n");
        close(sockclient);
        pthread_exit((void*)-1);
    }

    struct Client_Info client_info;
    strcpy(client_info.address, inet_ntoa(Addclient.sin_addr));
    strcpy(client_info.id, recv_id);
    client_info.port =  ntohs(Addclient.sin_port); //*bug need to modify a new port at wait part
    printf("%s is comming!\n", client_info.id);
    printf("%s %d\n\n", client_info.address, ntohs(Addclient.sin_port));


    //handle request
    HandleClientRequest(sockclient, client_info);


    printf("%s left.\n", client_info.id);
    close(sockclient);
    pthread_exit((void*)1);
}

int main(int argc, char const *argv[]) {
    int connfd, listenfd;
    struct sockaddr_in  servaddr;
    if(argc != 2){
        printf("usage: ./server <listen-port>\n");
        return 0;
    }

    // Varify port
    if(!portVarify(argv[1])){
        printf("\n Port invalid\n"); 
        return -1;
    }

    int port_num = atoi(argv[1]);
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port_num);
    // Listen socket create
    if( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){
        printf("Create socket error: %s(errno: %d)\n",strerror(errno),errno);
        return -1;
    }

    if( bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
        printf("Bind socket error: %s(errno: %d)\n",strerror(errno),errno);
        return -1;
    }
    //listenning
    if( listen(listenfd, 10) == -1){
        printf("Listen socket error: %s(errno: %d)\n",strerror(errno),errno);
        return -1;
    }

    printf("======Waiting for client's request======\n");
    while(1){
        if( (connfd = accept(listenfd, (struct sockaddr*)NULL, NULL)) == -1){
            printf("Accept socket error: %s(errno: %d)",strerror(errno),errno);
            continue;
        }else{
            // Create new thread to handle client request
            pthread_t thid;
            struct ThreadAttri Attri;
            struct ThreadAttri *ptAttri = &Attri;
            
            ptAttri->sockclient = connfd;

            if (pthread_create(&thid,NULL,serverThread,(void*)ptAttri) == -1){
                printf("Thread create error!\n");
                return -1;
            }
            
        }
    }
    return 0;
}