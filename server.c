#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#define SRV_PORT 60000
#define LISTEN_ENQ 5 
#define MAX_RECV_BUF 1000
#define MAX_SEND_BUF 1000

int flag;

char* substring_input(char* cmd,int index)
{

        if(strlen(cmd)-1 != index)
        {
                char* temp;
                temp = malloc(4 * sizeof(char) + 1);
                int i,j=0;
                for(i=index; i<strlen(cmd);i++,j++)
                        temp[j] = cmd[i];
                temp[j]='\0';

                return temp;
        }
        else
                return cmd;
}


int recv_file(int sock, char* file_name)
{
         char send_str [MAX_SEND_BUF]; /* message to be sent to server*/
         int f; /* file handle for receiving file*/
         ssize_t sent_bytes, rcvd_bytes, rcvd_file_size;
        char temp[128];
         char recv_str[MAX_RECV_BUF]; /* buffer to hold received data */
         size_t send_strlen; /* length of transmitted string */


        if((rcvd_bytes = recv(sock, recv_str, MAX_RECV_BUF, 0)) > 0)
        {

                if(strcmp(recv_str,"file not found"))
                {

                        /* attempt to create file to save received data. 0644 = rw-r--r-- */
                        if ( (f = open(file_name, O_WRONLY|O_CREAT, 0644)) < 0 )
                        {
                                perror("error creating file");
                                return -1;
                        }

                        rcvd_file_size = 0; /* size of received file */


                        /* continue receiving until ? (data or close) */
                        while (1)
                        {



                                memset(recv_str, 0, sizeof(recv_str));

                                rcvd_bytes = recv(sock, recv_str, MAX_RECV_BUF, 0);


                                if(rcvd_bytes > 0)
                                {

                                        rcvd_file_size += rcvd_bytes;

                                        if (write(f, recv_str, rcvd_bytes) < 0 )
                                        {
                                                perror("error writing to file");
                                                return -1;
                                        }
                                else if(rcvd_bytes < MAX_RECV_BUF)
                                        break;

                                }
                                else
                                        break;
                        }

                        close(f); /* close file*/
                        printf("Server received: %d bytes(%s)\n", (int)rcvd_file_size,file_name);
                        return rcvd_file_size;


                }
                else
                {
                        printf("Sevrver received: %d bytes(file not found)\n", (int)rcvd_bytes);
                        return rcvd_bytes;

                }
        }
}


void get_file_name(int sock, char* file_name)
{
        char recv_str[MAX_RECV_BUF]; /* to store received string */
        ssize_t rcvd_bytes; /* bytes received from socket */
        flag = -2;
        memset(file_name, 0, sizeof(file_name));
        memset(recv_str, 0, sizeof(recv_str));
        /* read name of requested file from socket */
        if ( (rcvd_bytes = recv(sock, recv_str, MAX_RECV_BUF, 0)) > 0)
        {
                sscanf (recv_str, "%s", file_name); /* discard CR/LF */



                if(!strcmp(file_name,"quit"))
                        flag = -1;
                else if(!strcmp(substring_input(file_name, strlen(file_name) - 4 ), "_get"))
                {
                       flag = 0;
                        file_name[strlen(file_name)-4] = '\0';
                }
                else
                        flag = 1;
        }
        else
                return;
}


int send_file(int sock, char *file_name)
{
        int sent_count; /* how many sending chunks, for debugging */
        ssize_t read_bytes, /* bytes read from local file */
        sent_bytes, /* bytes sent to connected socket */
        sent_file_size;
        char send_buf[MAX_SEND_BUF]; /* max chunk size for sending file */

        int f; /* file handle for reading local file*/
        sent_count = 0;
        sent_file_size = 0;


        memset(send_buf, 0, sizeof(send_buf));


 /* attempt to open requested file for reading */
        if( (f = open(file_name, O_RDONLY)) < 0) /* can't open requested file */
        {

                perror(file_name);
               if( (sent_bytes = send(sock, "file not found", strlen("file not found"), 0))< 0 )
                {
                        perror("send error");
                        return -1;
                }
                printf("Done sending to client. Sent %d bytes(file not found)\n",(int)sent_bytes);

                return 0;

        }
        else /* open file successful */
        {

                if( (sent_bytes = send(sock, "succcess", strlen("success"), 0)) < 0 )
                {
                        perror("send error");
                        return -1;
                }

                sleep(1);

                printf("Sending file: %s\n", file_name);
        while( (read_bytes = read(f, send_buf, MAX_RECV_BUF)) > 0 )
        {
                if( (sent_bytes = send(sock, send_buf, read_bytes, 0))< read_bytes )
                {
                        perror("send error");
                        return -1;
               }
                sent_count++;
                sent_file_size += sent_bytes;

                memset(send_buf, 0, sizeof(send_buf));

             }
        close(f);
        } /* end else */

        printf("Done sending to client. Sent %d bytes(%s)\n",(int)sent_file_size,file_name);
        return sent_count;
}


int main(int argc, char* argv[])
{
        int listen_flag = 1;
        int listen_fd, conn_fd;
        struct sockaddr_in srv_addr, cli_addr;
        socklen_t cli_len;
        char file_name [MAX_RECV_BUF]; /* name of the file to be sent */
        char print_addr [INET_ADDRSTRLEN];
        memset(&srv_addr, 0, sizeof(srv_addr)); /* zero-fill srv_addr structure*/
        memset(&cli_addr, 0, sizeof(cli_addr)); /* zero-fill cli_addr structure*/
        srv_addr.sin_family = AF_INET;
        srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        /* if port number supplied, use it, otherwise use SRV_PORT */
        srv_addr.sin_port = htons(SRV_PORT);
        if ( (listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        {
                perror("socket error");
                exit(EXIT_FAILURE);
        }

        /*Forcefully attaching socket to the port 60000*/
        if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                  &listen_flag, sizeof(listen_flag)))
        {
                perror("setsockopt");
                exit(EXIT_FAILURE);
        }
         /* bind to created socket */
         if( bind(listen_fd, (struct sockaddr*) &srv_addr, sizeof(srv_addr)) < 0
)
        {
                perror("bind error");
                exit(EXIT_FAILURE);
        }

        printf("Listening on port number %d ...\n", ntohs(srv_addr.sin_port));
        if( listen(listen_fd, LISTEN_ENQ) < 0 )
        {
                perror("listen error");
                 exit(EXIT_FAILURE);
        }
        cli_len = sizeof(cli_addr);
        printf ("Waiting for a client to connect...\n\n");
                /* block until some client connects */
        if ( (conn_fd = accept(listen_fd, (struct sockaddr*) &cli_addr,&cli_len)) < 0 )
                perror("accept error");


        /* convert numeric IP to readable format for displaying */
        inet_ntop(AF_INET, &(cli_addr.sin_addr), print_addr, INET_ADDRSTRLEN);
        printf("Client connected!\n");


        int i;
        for(;;)
        {

                memset(file_name, 0, sizeof(file_name));

                get_file_name(conn_fd, file_name);


                if(flag == 0 && strlen(file_name) > 0)
                        send_file(conn_fd, file_name);
                else if( flag == 1 && strlen(file_name) > 0)
                        recv_file(conn_fd,file_name);

                else if(flag ==-1 && strlen(file_name) > 0)
                {
                        printf("***Closing connection***\n");
                        close(conn_fd);
                        close(listen_fd);
                        break;
                }
        }

        return 0;
}




