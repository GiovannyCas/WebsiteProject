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

void cli_conn(int s, int c)
{
    httpreq *req;
    
    char *p;

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

    printf("'%s'\n'%s'\n", req->method, req->url);
    free(req);
    close(c);
    return;
}

int main(int argc, char *argv[])
{
    //printf("hello America ya\n");
    int s, c;
   // char* template;
    //httpreq *req;
    //char buf[512];
    
    //template = 
        // "GET /sdfsdfd HTTP/1.1\n"
        // "HOST: fagelsjo.net:8181\n"
        // "Upgrade-Insecure-Requests: 1\n"
        // "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\n"
        // "User-Agent: Mozilla/5.0 (Macintosh; Silicon Mac Os X 14_3_1) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/16.3 Safari/605.1.15\n"
        // "Accept-Language: en-GB, en;q=0.9\n"
        // "Accept-Encoding: gzip, deflate\n"
        // "Connection: keep-alive\n"
        // "\n", 0X00;
    

   // memset(buf, 0, 512);
    //strncpy(buf, template, 511);

    //req = parse_http(buf);

   // printf("Method: '%s'\nURL: '%s'\n", 
     //   req->method, req->url);
    //free(req);
    //return 0;
    // if(argc < 2)
    // {
    //     fprintf(stderr,"Usage: %s <listening port>\n",
    //     argv[0]);
    //     return -1;
    // }
    // else
    // {
    //     port = argv[1];
    // }
    
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



