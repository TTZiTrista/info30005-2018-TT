/*login id:tenghez
my name:tenghe zhang*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>  
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>



#define BACKLOG 5
#define BUFF 1024

void *receive(void *newfd);
void senddata(int newfd, char *rootpath);
void getMime(int newfd, char *ext);
char *concat(char *s1, char *s2);
int file_size(FILE *f1,char buff[BUFF]);
char *getfile(char buff[BUFF]);


typedef struct args{
    char *rootpath;
    int clientfd;
} ARGS;


/*Use the sample code server.c thread1.c thread2.c as reference */

int main(int argc, char *argv[]){

    int sockfd,newsocketfd,portno,cli_len;
    struct sockaddr_in serv_addr,cli_addr;
    pthread_t tid;
    ARGS *args = malloc(sizeof(ARGS));



    if (argc < 3) {
        printf("ERROR,no port/path provided");
        exit(1);
    }

    /* Create TCP socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    portno = atoi(argv[1]);
    args->rootpath = argv[2];

    /* Create address we're going to listen on (given port number)
     - converted to network byte order & any IP address for 
     this machine */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);// store in machine-neutral format

     /* Bind address to the socket */
    if (bind(sockfd, (struct sockaddr *) &serv_addr, (socklen_t)sizeof(cli_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }
    


    /* Listen on socket - means we're ready to accept connections - 
     incoming connection requests will be queued */
    if (listen(sockfd, BACKLOG) < 0) {
        perror("ERROR on listening");
        exit(1);
    }
    /* Accept a connection - block until a connection is ready to
     be accepted. Get back a new file descriptor to communicate on. */
    while(1){
        //printf("READY TO CREATE PTHREAD ::::::::::\n");
        cli_len = sizeof(cli_addr);
        newsocketfd = accept(sockfd, (struct sockaddr *) &cli_addr, 
                        &cli_len);

        if (newsocketfd < 0) {

            perror("ERROR on accepting connection");
            exit(1);
        
            }
            args->clientfd = newsocketfd;
            
            if(pthread_create(&tid, NULL, (void*)&receive, (void*)args)!= 0){
                 perror("ERROR on creating thread");
        }
  
       
    }

    /* wait for thread to exit */ 
    //pthread_join(tid, NULL);

    
    /* close socket */
    
    close(sockfd);
    
    return 0; 
}



/*thread doing the work*/
 void *receive(void* arg ){
    //printf("we have received");
    ARGS *request = (ARGS *) arg;
    int pthreadfd = request->clientfd;
    senddata(request->clientfd,request->rootpath);/*respond the client request send the data to the client*/
    close(pthreadfd);
}

/*read the client request and get the needed files source and sent the data from server*/
void senddata(int newfd, char *rootpath) {

    //printf("11111111");
    char buff[BUFF];
    int filesize;
    int n,i,k,j;
    FILE *fp = NULL;
    char *reply1,*reply2;
    char *source,*dest,*pChar;
    char Character;

    bzero(buff, BUFF);

    /* Read request*/

    n = read(newfd, buff, sizeof(buff) - 1);

    if (n < 0) 
    {
        perror("ERROR reading from socket");
        exit(1);
    }

    //printf("%s\n", buff);


    /* Get the file path and source*/
    char *file = getfile(buff);

    
    /*get the directory of the files*/
    char *filename = concat(rootpath, file);

    /* open file in bytes */
    fp = fopen(filename, "r");

    /*send header*/
    reply1 = "HTTP/1.0 404 NOT FOUND\r\n";
    reply2 = "HTTP/1.0 200 OK\r\n";

    /* if file is not exist, send 404 and close*/
    if (fp == NULL) {
        write(newfd, reply1, strlen(reply1));
        close(newfd); 
    }

    /* Other send 200 and get files,then read and reply */
    else {
        write(newfd, reply2, strlen(reply2));
       
    /*get MIME type*/
        char *ext = strrchr(file, '.');
        getMime(newfd, ext);

    /*send data*/


    while (!feof(fp)) {
        filesize = file_size(fp,buff);
        write(newfd, buff, filesize);
    }

        fclose(fp);
        close(newfd);
    }
}

/*get the mime and write the type*/
void getMime(int newfd, char *ext){
    char *mimetype;
    char *content = "Content-Type: ";

    if (strncmp(ext, ".html", 5) == 0) {
        mimetype = concat(content, "text/html\r\n\r\n");
    }

    else if (strncmp(ext, ".css", 4) == 0) {
        mimetype = concat(content, "text/css\r\n\r\n");
    }
    else if (strncmp(ext, ".jpg", 4) == 0) {
        mimetype = concat(content, "image/jpeg\r\n\r\n");
    }
    else if (strncmp(ext, ".js", 3) == 0) {
        mimetype = concat(content, "text/javascript\r\n\r\n");
    }
    write(newfd, mimetype, strlen(mimetype));

}



/*helper function to concatenate two strings*/

char *concat(char *s1, char *s2) {
    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);
    char* combine = malloc (len1 + len2 + 1);//+1 for the null-terminator
    //in real code you would check for errors in malloc here
    memcpy(combine, s1, len1);
    memcpy(combine + len1, s2, len2 + 1);
    return combine;
}

/*helper function to get the file name*/

char *getfile(char buff[BUFF]){
   char *path = strtok(buff, " /");
   char *file = strtok(NULL, " \t");

   /*for(k = 0;k < BUFF;k++){
        if(buff[k] == '.'){
            for(j = k;j > 0;j--){
                if(buff[j] == '/'){
                    i = j;
                }

            }
        }
    }
    
        //fseek(buff, j, SEEK_SET);           
        //Character = getc(buff);                    //seek and get next character
    while(buff[i] != '\n'){
        char *source = NULL;
        Character = buff[i];
        char *pChar = &Character;
        strcat(source, pChar);
        i++;
    }*/
   return file;
}


/*helper function to get the file size*/
int file_size(FILE *f1,char buff[BUFF]){
    int size=fread(buff, 1, sizeof(buff) - 1, f1);  

    /*fseek(fp, 0L, SEEK_END);  
    int size = ftell(fp);  
    fclose(fp);  
    return size; //from reference*/
    
    return size;  
}  



