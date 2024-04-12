#include "include/global.h"



#define PORT 8181
#define LISTENADDR "127.0.0.1"


/* structures */
struct sHttpRequest
{
    char method[8];
    char url[128];
};
typedef struct sHttpRequest httpreq;

char* error;

/* return 0 on error, or it return a socket fd. */
int srv_init(int portno)
{
    int s;
    struct sockaddr_in srv;

    s = socket(AF_INET, SOCK_STREAM, 0);

    if(s <  0)
    {
        error = "socket() error";
        return 0;
    }

    srv.sin_family = AF_INET;
    srv.sin_addr.s_addr = inet_addr(LISTENADDR);
    srv.sin_port = htons(portno);

    if(bind(s, (struct sockaddr *)&srv, sizeof(srv)))
    {
        close(s);
        error = "bind() error";
        return 0;
    }

    if(listen(s, 5) != 0)
    {
        close(s);
        error = "listen() error";
        return 0;
    }
    
    return s;

}

/* return 0 on error, or it return the new client's socket fd. */
int cli_accept(int s)
{
    int c;
    socklen_t addrlen;
    struct sockaddr_in cli;

    addrlen = 0;
    memset(&cli, 0, sizeof(cli));
    c = accept(s, (struct sockaddr *)&cli, &addrlen);
    if(c < 0)
    {
        error = "accept() error";
        return 0;
    }

    return c;
}

/* return 0 on error, or it return httpreq structure.  */
httpreq *parse_http(char* str)
{
    httpreq *req;
    char *p;

    req = malloc(sizeof(httpreq**));
  

    for(p = str; *p && *p != ' '; p++);
    if( *p == ' ')
    {
        *p = 0;
    }
    else 
    {
    error = "parse_htttp() NOSPACE error";
    free(req);

    return 0;
    }

    strncpy(req->method, str, 7);

    for(str = ++p; *p && *p != ' '; p++);
    if( *p == ' ')
    {
        *p = 0;
    }
    else 
    {
    error = "parse_htttp() 2NDSPACE error";
    free(req);

    return 0;
    }


    strncpy(req->url, str, 127);
    return req;
}


/* return 0 on error, or return the data. */
char *cli_read(int c)
{
    static char buf[512];
    memset(buf, 0, 512);
    if(read(c, buf, 511) < 0)
    {
        error = "read() error";
        return 0;
    }
    return buf;
    
}

void http_response(int c, char* contentType, char* data)
{
    char buf[512];
    int n;

    memset(buf, 0, 512);

    n= strlen(data);
    snprintf(buf, 511, 
    "Content-Type: %s\n"
    "Content-Length: %d\n"
    "\n%s\n",
    contentType, n, data);

    n = strlen(buf);
    write(c, buf, n);

    return;
}

void http_headers(int c, int code)
{
    char buf[512];
    int n;

    memset(buf, 0, 512);
    snprintf(buf, 511, 
    "HTTP/1.0 %d OK\n"
    "Server: main.c\n"
    "Cache-Control: no-store, no-cache, max-age=0, private\n"
    "Content-Language: en\n"
    "Expires: -1\n"
    "X-Frame-Options: SAMEORIGIN\n",
    code);
    
    n = strlen(buf);
    write(c, buf, n);

    return;
}

void cli_conn(int s, int c)
{
    httpreq *req;
    char *p;
    char *res;

    

    p = cli_read(c);
    if(!p)
    {
        fprintf(stderr, "%s\n", error);
        close(c);
        return;
    }

    req = parse_http(p);
    if(!req)
    {
        fprintf(stderr, "%s\n", error);
        close(c);
        return;
    }

    
    if(!strcmp(req->method, "GET") && (!strcmp(req->url, "/app/webpage")))
    {
        res = "<html>Hello world </html>";
        http_headers(c, 200); /* 200 = everything is ok. */
        http_response(c, "text/html", res);
    }
    else
    {
        res = "File not found";
        http_headers(c, 404); /* 404 = file not found. */
        http_response(c, "text/plain", res);
    }

    free(req);
    close(s);
    close(c);

    return;
}

int main(int argc, char *argv[])
{
    
    int s, c;
   
    
    s = srv_init(PORT);

    if(!s)
    {
        fprintf(stderr, "%s\n", error);
        return -1;
    }   

    printf("Listening on %s:%i\n", LISTENADDR, PORT);
    while(1)
    {
        c = cli_accept(s);
        if(!c)
        {
            fprintf(stderr, "%s\n", error);
            continue;
        }

        printf("Incoming connection\n");

        /* for the main process: return the new process' id
         * for the new process: return 0
         * */
        if(!fork())
        {
            cli_conn(s, c);
        }
      
    }



    return -1;
}



