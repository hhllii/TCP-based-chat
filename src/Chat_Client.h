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
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
#include "simpleSocket.h"

struct ThreadAttri{
    int sockclient;
};