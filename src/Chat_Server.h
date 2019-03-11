#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <string> 
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <netdb.h>
#include <time.h>
#include <sys/time.h>
#include "simpleSocket.h"

using namespace std;

struct ThreadAttri{
    int sockclient;

};

struct Client_Info
{
    char id[MAX_ID_LEN];
    char address[30];
    int port;
};
