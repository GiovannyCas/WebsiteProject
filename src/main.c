#include "include/global.h"


#define PORT 8183
#define LISTENADDR "127.0.0.1"

/* http://localhost:8183/about */

/* Structures */
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

struct Route 
{
	char* key;
	char* value;

	struct Route *left, *right;
};

/*************/

/* Error messagers*/
char* error;
/******************/

/* HTTP FUNCTIONS */
httpreq *parse_http(char* str);
void http_response(int c, char* contentType, char* data);
void http_headers(int c, int code);
/******************/

/* ROUTES FUNCTIONS */
struct Route* initRoute(char* key, char* value);
struct Route* addRoute(struct Route* root, char* key, char* value);
struct Route* search(struct Route * root, char * key);
void inorder(struct Route * root );
/********************/

/* OTHERS FUNCTIONS */
File* readfile(char* filename);
/******************/

/* SERVER FUNCTIONS */
int srv_init(int portno);
/********************/

/* DEBUG TOOLS */
void hexdump(char* str, int size);

/* CLIENT FUNCTIONS */
int cli_accept(int s);
char *cli_read(int c);
void cli_conn(struct Route* route, int c);
/********************/


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

	struct Route* route = initRoute("/", "index.html");
	addRoute(route, "/about", "about.html");
	

	printf("\n====================================\n");
	printf("=========ALL VAILABLE ROUTES========\n");
	// display all available routes
	inorder(route);

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
			
			cli_conn(route, c);
			close(s);
			exit(0);
		}
		close(c);
	  
	}

	printf("Server shuting down");

	return -1;
}

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
	char buf[715];
	int n;

	memset(buf, 0, 715);

	n = strlen(data);
	snprintf(buf, 714, 
	"Content-Type: %s\r\n"
	"Content-Length: %d\r\n"
	"\n%s\r\n",
	contentType, n, data);

	printf("%s", buf);

	n = strlen(buf);
	send(c, buf, n, 0);

	return;
}

void http_headers(int c, int code)
{
	char buf[512];
	int n;

	memset(buf, 0, 512);
	snprintf(buf, 511, 
	"HTTP/1.1 %d OK\r\n"
	"Server: main.c\r\n"
	"Cache-Control: no-store, no-cache, max-age=0, private\r\n"
	"Content-Language: enr\r\n"
	"Expires: -1\r\n"
	"X-Frame-Options: SAMEORIGIN\r\n",
	code);
	
	printf("%s", buf);
	n = strlen(buf);
	send(c, buf, n, 0);

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

char* render_static_file(char* filename)
{
	FILE* file = fopen(filename, "r");
	int ch;
	int i = 0;

	if(file == NULL)
	{
		return NULL;
	}
	else
	{
		printf("%s does exist \n", filename);
	}

	fseek(file, 0, SEEK_END);
	long fsize = ftell(file);
	fseek(file, 0, SEEK_SET);

	char* temp = malloc(sizeof(char) * (fsize + 1));
	while((ch = fgetc(file)) != EOF)
	{
		temp[i] = putchar(ch);
		i++;
	}

	fclose(file);
	return temp;
}




void cli_conn(struct Route* route, int c)
{
	httpreq *req;
	char *p;
	char* response_data;

	char template[100] = "";
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

	printf("The method is %s\n", req->method);
	printf("The route is %s\n", req->url);

	if(!strcmp(req->method, "GET"))
	{
		if(strstr(req->url , "/static/") != NULL)
		{
			strcat(template, "static/index.css");
			response_data = render_static_file(template);
	
			http_headers(c, 200);
			http_response(c, "text/html; charset=utf-8", response_data);
			free(response_data);
		}
		else 
		{
			struct Route* dest = search(route, req->url);
			strcat(template, "templates/");

			if(dest == NULL)
			{
				strcat(template, "404.html");
				response_data = render_static_file(template);
	
				http_headers(c, 404);
				http_response(c, "text/html; charset=utf-8", response_data);
				free(response_data);
			}
			else
			{
				strcat(template, dest->value);
				response_data = render_static_file(template);
				
				http_headers(c, 200);
				http_response(c, "text/html; charset=utf-8", response_data);
				free(response_data);
			}
		}
	}

	free(req);
	close(c);
	
	
	return;
}

struct Route* initRoute(char* key, char* value) {
	struct Route * temp = (struct Route *) malloc(sizeof(struct Route));

	temp->key = key;
	temp->value = value;

	temp->left = temp->right = NULL;
	return temp;
}

void inorder(struct Route* root)
{

	if (root != NULL) {
		inorder(root->left);
		printf("%s -> %s \n", root->key, root->value);
		inorder(root->right);
	}
}

struct Route* addRoute(struct Route * root, char* key, char* value) {
	if (root == NULL) {
		return initRoute(key, value);
	}

	if (strcmp(key, root->key) == 0) {
		printf("============ WARNING ============\n");
		printf("A Route For \"%s\" Already Exists\n", key);
	}
	else if (strcmp(key, root->key) > 0) {
		root->right = addRoute(root->right, key, value);
	}
	else {
		root->left = addRoute(root->left, key, value);
	}
}

struct Route* search(struct Route * root, char* key) {
	if (root == NULL) {
		return NULL;
	} 

	if (strcmp(key, root->key) == 0){
		return root;
	}else if (strcmp(key, root->key) > 0) {
		return search(root->right, key);
	}else if (strcmp(key, root->key) < 0) {
		return search(root->left, key);
	}  

}
