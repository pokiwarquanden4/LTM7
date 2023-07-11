#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

void remove_user(int client);

    int users[64];      // Mang socket client da dang nhap
    char *user_ids[64]; // Mang luu tru id cua client da dang nhap
    int num_users = 0;  // So client da dang nhap

int main() 
{
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1)
    {
        perror("socket() failed");
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9000);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) 
    {
        perror("bind() failed");
        return 1;
    }

    if (listen(listener, 5)) 
    {
        perror("listen() failed");
        return 1;
    }

    fd_set fdread, fdtest;
    FD_ZERO(&fdread);
    FD_SET(listener, &fdread);

    char buf[256];

    while (1)
    {
        fdtest = fdread;

        int ret = select(FD_SETSIZE, &fdtest, NULL, NULL, NULL);

        if (ret < 0)
        {
            perror("select() failed");
            break;
        }

        for (int i = 0; i < FD_SETSIZE; i++)
            if (FD_ISSET(i, &fdtest))
            {
                if (i == listener)
                {
                    // Chap nhan ket noi
                    int client = accept(listener, NULL, NULL);
                    if (client < FD_SETSIZE)
                    {
                        printf("New client connected: %d\n", client);
                        FD_SET(client, &fdread);
                        
                    }
                    else
                    {
                        printf("Too many connections.\n");
                        close(client);
                    }
                }
                else
                {
                    // Nhan du lieu
                    int ret = recv(i, buf, sizeof(buf), 0);
                    if (ret <= 0)
                    {
                        printf("Client %d disconnected.\n", i);
                        char msg[64];
                        sprintf(msg,"QUIT %s\n", user_ids[i]);
                        for (int k=0;k<num_users;k++){
                            if (users[k] != i){
                                send(users[k], msg, strlen(msg), 0); 
                            }
                        }
                        close(i);
                        FD_CLR(i, &fdread);
                        remove_user(i);

                    }
                    else
                    {
                        buf[ret] = 0;
                        printf("Received from %d: %s\n", i, buf);

                        // Kiem tra trang thai dang nhap cua client
                        int client = i;

                        int j = 0;
                        for (; j < num_users; j++)
                            if (users[j] == client) break;

                        if (j == num_users)
                        {
                            // Chua dang nhap
                            // Xu ly cu phap yeu cau dang nhap
                            char cmd[32], id[32], tmp[32];
                            ret = sscanf(buf, "%s%s%s", cmd, id, tmp);
                            if (ret == 2)
                            {
                                if (strcmp(cmd, "JOIN") == 0)
                                {
                                    
                                    int error=0;
                                    int k = 0;
                                    for (unsigned int k =0;k< strlen(id);k++){
                                        if ((id[k]<=122 && id[k]>=97)|| (id[k]>=48 && id[k]<=57)){
                                            continue;
                                        }
                                        else {
                                            char *msg = "201 INVALID NICK NAME\n";
                                            send(client,msg, strlen(msg),0);
                                            error=1;
                                            break;
                                        }
                                    }
                                    if (error ==0)
                                    {
                                    for (; k < num_users; k++)
                                        if (strcmp(user_ids[k], id) == 0) break;
                                    
                                    if (k < num_users)
                                    {
                                        char *msg = "200 NICKNAME IN USE\n";
                                        send(client, msg, strlen(msg), 0);
                                        error=1;
                                    }
                                    else
                                    {
                                        users[num_users] = client;
                                        user_ids[num_users] = malloc(strlen(id) + 1);
                                        strcpy(user_ids[num_users], id);
                                        num_users++;
                                    }
                                    }
                                    if (error !=1)
                                    {
                                    char *msg = "100 OK\n";
                                    send(client, msg, strlen(msg), 0);                      
                                    }
                                }
                                else
                                {
                                    char *msg = "999 UNKNOWN ERROR\n";
                                    send(client, msg, strlen(msg), 0);
                                }
                            }
                            else
                            {
                                char *msg = "999 UNKNOWN ERROR\n";
                                send(client, msg, strlen(msg), 0);
                            }
                        }
                        else
                        {
                            // Da dang nhap
                            char cmd[32];
                            ret = sscanf(buf, "%s", cmd);
                            char sendbuf[512];
                            if (strcmp(cmd, "MSG") == 0)
                            {                               
                                for (int k = 0; k < num_users; k++)
                                    if (users[k] == client)
                                        sprintf(sendbuf, "MSG %s %s\n",user_ids[k],buf + 4);
                                for (int k = 0; k < num_users; k++){
                                    if (users[k] != client){
                                        send(users[k], sendbuf, strlen(sendbuf),0);
                                    }
                                }
                            }
                            if (strcmp(cmd,"PMSG") == 0){
                                char id[32],tmp[64];
                                int namePos;
                                int pos=-1;
                                ret = sscanf(buf, "%s%s%s",cmd,id,tmp);
                                if (ret==3){
                                    for (int k = 0; k < num_users; k++){
                                        if (strcmp(id,user_ids[k])==0){
                                            pos=k;
                                            break;
                                        }
                                    }
                                    for (int k = 0; k < num_users; k++)
                                    if (users[k] == client){
                                        namePos=k;
                                    }
                                if (pos !=-1 && strcmp(user_ids[namePos],id)!=0){
                                    sprintf(sendbuf, "PMSG %s %s\n",user_ids[namePos],buf + 6+ strlen(id) );
                                    send (users[pos],sendbuf,strlen(sendbuf),0);
                                }
                                else {
                                    char *msg = "201 INVALID NICK NAME\n";
                                    send(client,msg, strlen(msg),0);
                                }   
                                }

                                else {
                                    char *msg = "999 UNKNOWN ERROR\n";
                                    send(client,msg, strlen(msg),0);
                                }
                            }
                        }
                    }
                }
            }
    }
    
    close(listener);    

    return 0;
}
void remove_user(int client)
{
    int i = 0;
    for (; i < num_users; i++)
        if (users[i] == client)
            break;
    if (i < num_users)
    {
        if (i < num_users - 1){
            users[i] = users[num_users - 1];
            user_ids[i] = user_ids[num_users - 1];
        }
        num_users--;
    }
}