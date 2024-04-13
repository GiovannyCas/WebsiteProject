#include "include/global.h"



#define PORT 8181
#define LISTENADDR "127.0.0.1"

/* http://localhost:8181/img/test.jpg */


/* structures */
struct sHttpRequest
{
    char method[8];
    char url[128];
};
typedef struct sHttpRequest httpreq;

struct sFile
{
    char filename[64];
    char* fContent;
    int size;
};

typedef struct sFile File;

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

    n = strlen(data);
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
void hexdump(char* str, int size)
{
    char *p;
    int i;

    for(p = str, i=0 ; i<size; i++)
    {
        printf("0x%2.x", *p++);
    }
    printf("\n");
    fflush(stdout);

    return;
}

/* 1 = everything is ok, 0 = on error. */
int sendfileToClient(int c, char* contentType, File* file)
{ 
    char buf[512];
    char* p;
    int n, x;
    

    if(!file)
    {
        return 0;
    }

    memset(buf, 0, 512);
    snprintf(buf, 511, 
    "Content-Type: %s\n"
    "Content-Length: %d\n\n",
    contentType, file->size);
    
    n = strlen(buf);
    write(c, buf, n);

    n = file->size;
    p = file->fContent;
    while(1)
    {
        
        x = write(c, p, (n < 512)?n:512);
        
        if(x < 1)
        {
            return 0;
        }
        n -= x;
        if(n < 1)
        {
            break;
        }
        else
        {
            p += x;
        }
    }

    return 1;

}


/* returns 0 on error, or a file structure */
File* readfile(char* filename)
{
    char buf[512];
    char* p;
    int n, x, fd;
    File* f;
    printf("Opening file\n");
    fd = open(filename, O_RDONLY);
    
    if(fd < 0)
    {
        printf("Failed to open file");
        return 0;
    }
    f = malloc(sizeof(struct sFile));
    if(!f)
    {
        close(fd);
        return 0;
    }

    strncpy(f->filename, filename, 63);
    f->fContent = malloc(512);
    printf("Now read() the file\n");
    x = 0; /* bytes read */
    while(1)
    {
        memset(buf, 0, 512);
        n = read(fd, buf, 512);
        if(!n)
        {
            break;
        }
        else if(x == -1)
        {
            close(fd);
            free(f->fContent);
            free(f);
            printf("Failed error.");
            return 0;
        }

        memcpy((f->fContent) + x, buf,  n);
        x += n;
        f->fContent = realloc(f->fContent, (512 + x));
    }
    f->size = x;
    close(fd);
    return f;

}



void cli_conn(int s, int c)
{
    httpreq *req;
    char *p;
    char *res;
    char str[96];
    File* file;

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
    /* TODO: improve security by checking for things like "../.." etc */
    if(!strcmp(req->method, "GET") && !strncmp(req->url, "/img/", 5))
    {
        if(strstr(req->url, ".."))
        {
            http_headers(c, 403); /* 403-series = access denied etc */
            res = "Access denied";
            http_response(c, "text/plain", res);

        }
        memset(str, 0, 96);
        snprintf(str,95, ".%s", req->url);
        file = readfile(str);
        if(!file)
        {
            res = "404 - File not found";
            http_headers(c, 404); /* 404 = file not found. */
            http_response(c, "text/plain", res);
            
        }
        else
        {
            http_headers(c, 200);
            if(!sendfileToClient(c, "image/png", file))
            {
                res = "HTTP server error";
                http_headers(c, 500); /* 500 = server error. */
                http_response(c, "text/plain", res);
            }
            
        }
    }
    if(!strcmp(req->method, "GET") && !strcmp(req->url, "/app/webpage"))
    {
        res = "<html><img src='/img/test.jpg' alt='image' /></html>";
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
    close(c);
    free(file->fContent);
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
            close(s);
            exit(0);
        }
        close(c);
      
    }

    printf("Server shuting down");

    return -1;
}



