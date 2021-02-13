#include <stdio.h>
#include <assert.h>

#include "csapp.h"

#include "cache.h"

#define USE_CACHE    // ues web cache

void *client_handle(void *arg);
int free_port(char *buf);

#ifdef USE_CACHE
void reply_client(int remote, int client, char *buf, char *url);
#else
void reply_client(int remote, int client, char *buf);
#endif

void add_reqheader(char *req, char *host);
int parse_request(char *buf, char *req, char *host, char *port, char *url);

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

int main(int argc, char *argv[])
{
    char port[10] = {0};
    if (argc < 2) {
        free_port(port);
    } else {
        memcpy(port, argv[1], sizeof(port) - 1);
    }

    fprintf(stderr, "free port: %s\n", port);
#ifdef USE_CACHE
    init_web_cache();   // init cache
#endif
    int sock = Open_listenfd(port);

    signal(SIGPIPE, SIG_IGN);    // ignore SIGPIPE

    while (1) {
        int fd = Accept(sock, NULL, NULL);
        fprintf(stderr, "A client come: fd = %d\n", fd);
       
        pthread_t tid;
        if (pthread_create(&tid, NULL, client_handle, (void *)(long)fd) < 0) {
            fprintf(stderr, "create thread for client [%d] error\n", fd);
            close(fd);
        }
    }

    return 0;
}

void *client_handle(void *arg)
{
    pthread_detach(pthread_self());

    int cfd = (long)arg;

    size_t n;
    char *buf = (char *)malloc(MAXLINE);
    char *req = (char *)malloc(MAXLINE); 
    rio_t rio;
    Rio_readinitb(&rio, cfd);

    int remote = -1;
    char url[64], host[100], port[10];
   
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        if(strncmp(buf, "GET ", 4) == 0) {
            if (remote != -1) close(remote);    // don't forget
            
            req[0] = '\0';
            
            if (parse_request(buf, req, host, port, url) < 0) goto out;  // bad request
            
            continue;
        }
        
        if (strncmp(buf, "Host: ", 6) == 0 ||
            strncmp(buf, "User-Agent: ", 12) == 0 ||
            strncmp(buf, "Connection: ", 12) == 0 ||
            strncmp(buf, "Proxy-Connection: ", 18) == 0) {
            continue;
        }

        strcat(req, buf);   // add to request header

        if (strncmp(buf, "\r\n", 3)) continue;

        // now we can process the request
#ifdef USE_CACHE
        if (process_cache(cfd, url)) continue;    // the web object has cached, so need not connect to remot
#endif
        // cache miss, need to connect to remot
        remote = Open_clientfd(host, port);
        if (remote < 0) goto out;

        add_reqheader(req, host);   // add other request header
        Rio_writen(remote, req, strlen(req)+1);   // send request to remote

        fprintf(stderr, "%s", req);
        req[0] = '\0';

#ifdef USE_CACHE
        reply_client(remote, cfd, buf, url);    // reply client
#else
        reply_client(remote, cfd, buf);    // reply client
#endif
    }
    
    if (remote != -1) close(remote);

out:
    fprintf(stderr, "\n--- client [%d] disconnected ---\n", cfd);
    close(cfd);
    free(buf);
    free(req);
    return NULL;
}

/*
 * get response from remote server, and reply to client
 */
#ifdef USE_CACHE
void reply_client(int remote, int client, char *buf, char *url)
{
    size_t total = 0;
    char *cache = (char *)malloc(MAX_OBJECT_SIZE);
    char *p = cache;

    size_t n;
    while ((n = Rio_readn(remote, buf, MAXLINE)) != 0) {
        Rio_writen(client, buf, n);   // reply client
    
        total += n;
        memcpy(p, buf, n);
        p = cache + total % MAX_OBJECT_SIZE;
    }

    if (total <= MAX_OBJECT_SIZE) {
        cache_webobj(url, cache, total);
    }

    free(cache);
}
#else
void reply_client(int remote, int client, char *buf)
{
    size_t n;
    while ((n = Rio_readn(remote, buf, MAXLINE)) != 0) {
        Rio_writen(client, buf, n);   // reply client
    }
}
#endif

/*
 * parse request line
 */
int parse_request(char *buf, char *req, char *host, char *port, char *url)
{
    int ret = -1;

    strcat(req, "GET ");

    char *str = &buf[4];
    if (strncmp(&buf[4], "http://", 7) == 0) {
        str += 7;
    } else if (strncmp(&buf[4], "https://", 8) == 0) {
        str += 8;
    }

    char *p = strchr(str, ' ');
    if (NULL == p) goto out;
    *p = '\0';

    strcpy(url, str);     // url

    p = strchr(str, '/');
    if (NULL == p) {
        strcat(req, "/");
    } else {
        strcat(req, p);
    }
    *p = '\0';
    strcat(req, " HTTP/1.0\r\n");

    p = strchr(str, ':');
    if (NULL == p) {
        strcpy(port, "80");
    } else {
        strncpy(port, p + 1, 9);    // port
    }
    *p = '\0';
    strncpy(host, str, 99);    // host

    ret = 0;

out:
    return ret;
}


/*
 * add other request headers
 */
void add_reqheader(char *req, char *host)
{
    char tmp[512] = {0};

    sprintf(tmp, "Host: %s\r\n", host);
    strcat(req, tmp);

    sprintf(tmp, "User-Agent: %s", user_agent_hdr);
    strcat(req, tmp);
    
    sprintf(tmp, "Connection: close\r\n");
    strcat(req, tmp);
    
    sprintf(tmp, "Proxy-Connection: close\r\n");
    strcat(req, tmp);
}

/* get free port number by `free-port.sh` */
int free_port(char *buf)
{
    FILE *fp = popen("./free-port.sh", "r");
    assert(fp);
    fscanf(fp, "%s", buf);
    pclose(fp);
    
    return 1;
}
