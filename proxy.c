#include <stdio.h>
#include "csapp.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void *thread(void *data);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void doit(int fd);
void read_requesthdrs(rio_t *rp, char *method, char *filename, char *version, char *uri, char *headers);
void parse_uri(char *uri, char *filename, char *port);

typedef struct cache_item
{
    volatile int length;
    char filename[MAXLINE];
    char uri[MAXLINE];
    char content[MAX_OBJECT_SIZE];
} cache_item;

cache_item *cache[MAX_CACHE_SIZE];

sem_t mutex, w;
volatile int readcnt, clk;

int is_object_cached(cache_item *cached_item, char *uri, char *filename)
{

    P(&mutex);
    readcnt++;
    if (readcnt == 1)
        P(&w);
    V(&mutex);

    int is_cached = 0;
    for (int i = 0; i < MAX_CACHE_SIZE; i++)
    {

        if (cache[i] == NULL)
            continue;

        printf("from cache: %d\n", i);

        if ((!strcmp(uri, cache[i]->uri)) && (!strcmp(filename, cache[i]->filename)))
        {
            strcpy(cached_item->content,cache[i]->content);
            strcpy(cached_item->filename,cache[i]->filename);
            strcpy(cached_item->uri,cache[i]->uri);
            cached_item->length = cache[i]->length;
            printf("qqq\n");
            if(cached_item==NULL){
                printf("howwwwwwwwwwwwwwqqq\n");
            }
            is_cached = 1;
            break;
        }
    }
    printf("qqq\n");

    P(&mutex);
    readcnt--;
    if (readcnt == 0)
        V(&w);
    V(&mutex);

    printf("iscached: (%d)\n", is_cached);
    return is_cached;
}

void cache_object(char *content, int total_size, char *filename, char *uri)
{
    P(&w);

    if (total_size > MAX_OBJECT_SIZE)
        return;

    for (int i = 0; i < MAX_CACHE_SIZE; i++)
    {
        if (cache[i] == NULL)
        {
            cache[i] = (cache_item *)Malloc(sizeof(cache_item));
            strcpy(cache[i]->content, content);
            strcpy(cache[i]->filename, filename);
            strcpy(cache[i]->uri, uri);
            cache[i]->length = total_size;
            printf("cachelength: (%d)", cache[i]->length);
            V(&w);
            return;
        }
    }

    // int mn = MAX_CACHE_SIZE;
    // cache_item *LRU_item;
    // for (int i = 0; i < MAX_CACHE_SIZE; i++)
    // {
    //     cache_item *item = cache[i];
    //     if (item->last_access < mn)
    //     {
    //         mn = item->last_access;
    //         LRU_item = item;
    //     }
    // }

    // strcpy(LRU_item->content, content);
    // strcpy(LRU_item->filename, filename);
    // strcpy(LRU_item->uri, uri);
    // LRU_item->length = total_size;
    // LRU_item->last_access = clk++;

    V(&w);
}

int main(int argc, char **argv)
{
    Sem_init(&w, 0, 1);
    Sem_init(&mutex, 0, 1);
    int listenfd, *connfd;
    socklen_t clinetlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]);

    while (1)
    {
        clinetlen = sizeof(clientaddr);
        connfd = Malloc(sizeof(int));
        *connfd = Accept(listenfd, (SA *)&clientaddr, &clinetlen);
        Pthread_create(&tid, NULL, thread, connfd);
        // doit(connfd);
        // Close(connfd);
    }
    return 0;
}

void *thread(void *data)
{
    int fd = *((int *)data);
    pthread_detach(pthread_self());
    Free(data);
    doit(fd);
    Close(fd);
    return NULL;
}

void doit(int fd)
{
    char headers[MAXLINE], buf[MAXLINE], method[MAXLINE];
    char filename[MAXLINE], port[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char content[MAX_OBJECT_SIZE];
    strcpy(content, "");
    cache_item *cached_item=(cache_item *)Malloc(sizeof(cache_item));
    rio_t rio, tiny_rio;

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

    parse_uri(uri, filename, port);
    read_requesthdrs(&rio, method, filename, version, uri, headers);

    strcpy(uri, "localhost"); // +7

    printf("(%s) (%s)\n", uri, filename);
    if (is_object_cached(cached_item, uri, filename))
    {
        Rio_writen(fd, cached_item->content, cached_item->length);
        return;
    }
    int tinyfd = Open_clientfd(uri, port);
    Rio_readinitb(&tiny_rio, tinyfd);

    Rio_writen(tinyfd, headers, strlen(headers));
    int n, total_size = 0;
    while ((n = Rio_readlineb(&tiny_rio, buf, MAXLINE)) > 0)
    {
        printf("%s", buf);
        Rio_writen(fd, buf, n);
        total_size += n;

        if (total_size <= MAX_OBJECT_SIZE)
        {
            strcat(content, buf);
        }
    }
    cache_object(content, total_size, filename, uri);
}
// GET http://localhost:20000/tiny.c HTTP/1.0
// GET http://localhost:20000/home.html HTTP/1.0
// GET http://localhost:20000/csapp.c HTTP/1.0


// Fetching ./tiny/tiny.c into ./.proxy using the proxy
// Fetching ./tiny/home.html into ./.proxy using the proxy
// Fetching ./tiny/csapp.c into ./.proxy using the proxy
void read_requesthdrs(rio_t *rp, char *method, char *filename, char *version, char *uri, char *headers)
{
    sprintf(headers, "%s %s %s\r\n", method, filename, version);
    sprintf(headers, "%s Connection: close\r\n", headers);
    sprintf(headers, "%s Proxy-Connection: close\r\n", headers);
    sprintf(headers, "%s Host: %s\r\n\r\n", headers, uri);

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

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
    exit(1);
}
