#include <stdio.h> /* printf and standard I/O */
#include <sys/socket.h> /* socket, connect, socklen_t */
#include <arpa/inet.h> /* sockaddr_in, inet_pton */
#include <string.h> /* strlen */
#include <stdlib.h> /* atoi */
#include <fcntl.h> /* O_WRONLY, O_CREAT */
#include <unistd.h> /* close, write, read */
#define SRV_PORT 60000
#define MAX_RECV_BUF 1000
#define MAX_SEND_BUF 1000
#define IP_ADD "127.0.0.1"




int recv_file(int sock, char* file_name)
{

         char send_str [MAX_SEND_BUF]; /* message to be sent to server*/
         int f; /* file handle for receiving file*/
         ssize_t sent_bytes, rcvd_bytes, rcvd_file_size;
        char recv_str[MAX_RECV_BUF]; /* buffer to hold received data */
         size_t send_strlen; /* length of transmitted string */

         sprintf(send_str, "%s\n", file_name); /* add CR/LF (new line) */
         send_strlen = strlen(send_str); /* length of message to be transmitted */

         if( (sent_bytes = send(sock, file_name, send_strlen, 0)) < 0 )
        {
                 perror("send error");
                return  -1;
        }


        if((rcvd_bytes = recv(sock, recv_str, MAX_RECV_BUF, 0)) > 0)
        {
                if(strcmp(recv_str,"file not found"))
                {

                        file_name[strlen(file_name)-4] = '\0';

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
                        printf("Client received: %d bytes (%s)\n", (int)rcvd_file_size, file_name);
                        return rcvd_file_size;

                }

                else
                {
                         printf("Client received: %d bytes(file not found)\n", (int)rcvd_bytes);
                        return rcvd_bytes;
                }
        }
}

int send_file(int sock, char *file_name)
{

        char send_str [MAX_SEND_BUF]; /* message to be sent to server*/

        int sent_count; /* how many sending chunks, for debugging */
        ssize_t read_bytes, /* bytes read from local file */
        sent_bytes, /* bytes sent to connected socket */
        sent_file_size;
        char send_buf[MAX_SEND_BUF]; /* max chunk size for sending file */
        char * errmsg_notfound = "File not found\n";
        int f; /* file handle for reading local file*/
        size_t send_strlen; /* length of transmitted string */

        sent_count = 0;
        sent_file_size = 0;
        sprintf(send_str, "%s\n", file_name); /* add CR/LF (new line) */
         send_strlen = strlen(send_str); /* length of message to be transmitted */

         if( (sent_bytes = send(sock, file_name, send_strlen, 0)) < 0 )
        {
                 perror("send error");
                return  -1;
        }



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
                printf("Done sending to server. Sent %d bytes(file not found)\n",(int)sent_bytes);

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

                sent_bytes = 0;
        }
        close(f);
        } /* end else */

        printf("Done sending to server. Sent %d bytes(%s)\n",(int)sent_file_size,file_name);
        return sent_count;
}


char* substring_input(char* cmd,int index)
{
        if(strlen(cmd)-1 != index)
        {
                char* temp;
                temp = malloc(index * sizeof(char) + 1);
                int i;
                for(i=0; i<index;i++)
                temp[i] = cmd[i];
                temp[index+1]='\0';
                return temp;
        }
        else
                return cmd;
}


char* retrieve_filename(char* cmd)
{
        int index = 4;
        char *file_name = malloc(sizeof(char));
        int temp = 1;
        while(cmd[index]!='\n')
        {
                file_name = realloc(file_name, temp * sizeof(char));

                file_name[temp - 1] = cmd[index];

                temp++;

                index++;

                if(cmd[index] == '\n')
                        file_name[temp] = '\0';
        }

        return file_name;

}

int main(int argc, char* argv[])
{
        int sock_fd;
        struct sockaddr_in srv_addr;

       memset(&srv_addr, 0, sizeof(srv_addr));
        /* create a client socket */
        sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        srv_addr.sin_family = AF_INET; /* internet address family */

    
        if ( inet_pton(AF_INET, IP_ADD, &(srv_addr.sin_addr)) < 1 )
        {
                printf("Invalid IP address\n");
                exit(EXIT_FAILURE);
         }

        srv_addr.sin_port = htons(SRV_PORT);

        if(connect(sock_fd,(struct sockaddr*) &srv_addr, sizeof(srv_addr)) < 0 )
        {
                perror("connect error");
                exit(EXIT_FAILURE);
        }
        printf("connected to : %s : %d ..\n",IP_ADD,SRV_PORT);

        char option[512];

        printf("List of options\n1.get [file_name]\n2.put [file_name]\n3.quit\n");
        while(1)
        {
                printf("Enter the option : ");
                fgets(option, sizeof(option), stdin);

                if(!strcmp(substring_input(option,4), "get\n") || !strcmp(substring_input(option,5), "get \n") || !strcmp(substring_input(option,4), "put\n") || !strcmp(substring_input(option,5), "put\n") )
                        printf("Please specify the file name\n");

                else if(!strcmp(substring_input(option,4), "get ") )
                {
                        char *filename = retrieve_filename(option);
                        strcat(filename,"_get");

                       recv_file(sock_fd, filename);

                }

                else if(!strcmp(substring_input(option,4), "put ") )
                {

                        char *filename = retrieve_filename(option);

                        send_file(sock_fd, filename);
                }
                else if(!strcmp(substring_input(option,4), "quit\n"))
                        {

                                ssize_t sent_bytes;


                                 if( (sent_bytes = send(sock_fd, "quit", strlen("quit"), 0)) < 0 )
                                {
                                        perror("send error");
                                       return  -1;
                                }

                                printf("***Closing the connection***\n");
                                if(close(sock_fd) < 0)
                                {
                                        perror("socket close error");
                                        exit(EXIT_FAILURE);
                                }

                                exit(0);
                        }
                else
                        printf("Invalid input option\n");
        }

        return 0;
}






