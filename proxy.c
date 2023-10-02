#include <stdio.h>
#include "csapp.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

char headers[MAXLINE];
void doit(int fd);
void read_requesthdrs(rio_t *rp);
void parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg);

int main(int argc, char **argv)
{
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clinetlen;
    struct sockaddr_storage clientaddr;

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);

    while (1)
    {
        clinetlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clinetlen);
        Getnameinfo((SA *)&clientaddr, clinetlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connetion from (%s,%s)\n", hostname, port);
        doit(connfd);
        Close(connfd);
    }
    return 0;
}

char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
char filename[MAXLINE], port[MAXLINE];
void doit(int fd)
{
    rio_t rio;
    rio_t tiny_rio;

    /* Read request line and headers */
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE))
        return;
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);
    if (strcasecmp(method, "GET"))
    {
        clienterror(fd, method, "501", "Not Implemented",
                    "Tiny does not implement this method");
        return;
    }
    /* Parse URI from GET request */
    parse_uri(uri, filename, port);
    read_requesthdrs(&rio);


    // printf("uri: (%s) (%d) \n", uri, strlen(uri));
    // printf("filename: (%s) (%d) \n", filename, strlen(filename));
    // printf("port: (%s) \n", port);
    // printf("method: (%s)\n", method);
    strcpy(uri, "localhost"); // +7
    int tinyfd = Open_clientfd(uri, port);
    Rio_readinitb(&tiny_rio, tinyfd);
   
    Rio_writen(tinyfd, headers, strlen(headers));
    int n;
    while ((n=Rio_readlineb(&tiny_rio, buf, MAXLINE)) > 0)
    {
        printf("%s", buf);
        Rio_writen(fd, buf, strlen(buf));
    }
}
// GET http://localhost:20000/godzilla.jpg HTTP/1.0
// GET /godzilla.jpg HTTP/1.0
void read_requesthdrs(rio_t *rp)
{
    sprintf(headers,"%s %s %s\r\n",method,filename,version);
    sprintf(headers, "%s Connection: close\r\n", headers);
    sprintf(headers, "%s Proxy-Connection: close\r\n", headers);
    sprintf(headers, "%s Host: %s\r\n\r\n", headers,uri);

    char buf[MAXLINE];
    Rio_readlineb(rp, buf, MAXLINE);
    sprintf(headers, "%s%s", headers, buf);
    
    while (strcmp(buf, "\r\n"))
    {
        Rio_readlineb(rp, buf, MAXLINE);
        sprintf(headers, "%s%s", headers, buf);
    }
}
void parse_uri(char *uri, char *filename, char *port)
{
    char *ptr, *colon;
    int i = 0, colon_counter = 0;

    while (i < 100)
    {
        if (uri[i] == ':')
        {
            colon_counter++;
            if (colon_counter == 2)
            {
                colon = uri + i;
            }
        }
        if (uri[i] == '/' && uri[i + 1] != '/' && uri[i - 1] != '/')
        {
            ptr = uri + i;
            break;
        }
        i++;
    }
    *ptr = NULL;
    strcpy(filename, "/");
    strcat(filename, (ptr + 1));

    if (colon_counter == 2)
    {
        *colon = NULL;
        strcpy(port, colon + 1);
    }
}

void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg)
{
    exit(1);
}
