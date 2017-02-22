#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>


//----------------------------------------------------------------------------------------

const char *socket_path = "/tmp/virtual_serial";

#include "scope.cpp"

Scope	*the_scope;

void cls()
{
        printf( "%c[2J", 27);
}


void gotoxy(int x, int y)
{
    printf("%c[%d;%df",0x1B,y,x);
        //printf( "%c[2J", 0x1b);

}

//----------------------------------------------------------------------------------------

int init_connection()
{
   struct sockaddr_un addr;
   char buf[100];
   int fd,cl,rc;

    if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        exit(-1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    
    if (*socket_path == '\0') {
            *addr.sun_path = '\0';
            strncpy(addr.sun_path+1, socket_path+1, sizeof(addr.sun_path)-2);
    } else {
        strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
        unlink(socket_path);
    }

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind error");
        exit(-1);
    }

    if (listen(fd, 5) == -1) {
        perror("listen error");
        exit(-1);
    }

    return fd;
}

//----------------------------------------------------------------------------------------

bool match(char *buf, const char *command) 
{
    if (strncmp(buf, command, strlen(command)) == 0)
        return true;
    return false;
}

//----------------------------------------------------------------------------------------

void cat_f(int fd, char *cmd)
{
        char    cmd_buf[1024];
        char    reply[1024];

        cmd_buf[0] = 'f'; cmd_buf[1] = 0;
        strcat(cmd_buf, cmd);
        the_scope->Send(cmd_buf);
        send(fd, the_scope->reply, strlen(the_scope->reply) + 1, 0);
        printf("reply %s\n", the_scope->reply);
}


//----------------------------------------------------------------------------------------

void cat_r(int fd, char *cmd)
{
    char	cmd_buf[1024];
    char	reply[1024];

        cmd_buf[0] = 'r'; cmd_buf[1] = 0;
        strcat(cmd_buf, cmd);
        the_scope->Send(cmd_buf);
        send(fd, the_scope->reply, strlen(the_scope->reply) + 1, 0);
        printf("reply %s\n", the_scope->reply);
}

//----------------------------------------------------------------------------------------


void cat_c(int fd, char *cmd)
{
        char    cmd_buf[1024];
        char    reply[1024];

        cmd_buf[0] = 'c'; cmd_buf[1] = 0;
        strcat(cmd_buf, cmd);
        the_scope->Send(cmd_buf);
        send(fd, the_scope->reply, strlen(the_scope->reply) + 1, 0);
        printf("reply %s              \n", the_scope->reply);
}


//----------------------------------------------------------------------------------------

void cat_n(int fd, char *cmd)
{
        char    cmd_buf[1024];
        char    reply[1024];

        cmd_buf[0] = 's'; cmd_buf[1] = 0;
        strcat(cmd_buf, cmd);
        the_scope->Send(cmd_buf);
}


//----------------------------------------------------------------------------------------


void process(int fd, char *string)
{
        char    reply[1024];
    char	cmd[1024];

        printf("got %s\n", string);	
    ///*gotoxy(1, 9);*/printf("got %s             \n", string);
        
        if (match(string, ":V#")) {
        cat_r(fd, string);
        return;
        }
    if (match(string, ":Br")) {
        cat_c(fd, string);
        return;	
    }
        if (match(string, ":Bd")) {
                cat_c(fd, string);
                return;
        }

        if (match(string, ":GR#")) {
                cat_r(fd, string);
                return;
        }

        if (match(string, ":GD#")) {
                cat_r(fd, string);
                return;
        }

    if (match(string, ":GS#")) {
                cat_r(fd, string);
                return;
        }

        if (match(string, ":pS#")) {
                cat_r(fd, string);
                return;
        }
    
    if (match(string, ":RC")) {
        cat_n(fd, string);
        return;
    }

        if (match(string, ":Q")) {
                cat_n(fd, string);
                return;
        }
        
        if (match(string, ":Ms#")) {
                cat_n(fd, string);
                return;
        }
        
        if (match(string, ":Me#")) {
                cat_n(fd, string);
                return;
        }
        
        if (match(string, ":Mw#")) {
                cat_n(fd, string);
                return;
        }
        
        if (match(string, ":Mn#")) {
                cat_n(fd, string);
                return;
        }
        
        if (match(string, ":RD")) { 
                cat_c(fd, string);
                return;
        }
        
    if (match(string, ":RR")) {
                cat_c(fd, string);
                return;
        }

        
        if (match(string, ":RT")) {
                cat_n(fd, string);
                return;
        }
        
        if (match(string, ":RG")) {
                cat_n(fd, string);
                return;
        }

        if (match(string, ":RC")) {
                cat_n(fd, string);
                return;
        }

        
        if (match(string, ":RS")) {
                cat_n(fd, string);
                return;
        }

        if (match(string, ":Rs")) {
                cat_n(fd, string);
                return;
        }

        if (match(string, ":CM")) {
        cat_f(fd, string);
        return;
        }
        
        if (match(string, ":Sr")) {
        cat_c(fd, string);
        return;
        }

    if (match(string,":SL")) {
        cat_c(fd, string);
        return;
        }
    if (match(string, ":SC")) {
        cat_f(fd, string);
        return;	
    }

        if (match(string, ":Sd")) {
        cat_c(fd, string);
        return;
        }

        if (match(string, ":MS#")) {
                cat_c(fd, string);
                return;
        }

    if (match(string, "#")) {
            cat_n(fd, string);
        return;	
    }
    
    //if (match(string, ":U#")) {
        //cat_n(fd, string);
        //return;
    //}


    //printf("*** missed %s\n", string);
}

//-------------------------------------------------------------------------------------


int main()
{
    int	fd;

    fd = init_connection();
    if (fd < 0)
        return 1;

    the_scope = new Scope();
    the_scope->Init();

    //send(fd, buf, r, 0);


    
    char	cur_cmd[1024];
    char	buf[1024];	
    int		cp = 0;
    int		semi_count = 0;
    int		rc;
    
    listen(fd, 5);
    
    while (1) {
        int cl = accept(fd, NULL, NULL);	
       	if (cl == -1) continue; 

    	while ( (rc=read(cl,buf,sizeof(buf))) > 0) {
        	if (rc > 0) printf("%d\n", rc);	
        	
		for (int i = 0; i < rc; i++) {
            		cur_cmd[cp] = buf[i];
            		cur_cmd[cp+1] = 0;
            		if (buf[i] == ':') {
                		semi_count++;
                		if (semi_count == 4) {
                    			buf[i] = '#';	
                    			cur_cmd[cp] = '#';
                    			semi_count = 0;
                		}	
            		}
            		if (buf[i] == '#') {
                		process(cl, cur_cmd);
                		cp = 0;
                		semi_count = 0;
            		}
            		else
                		cp++;	
		}
	}	
	if (rc == 0) close(cl);
    }
    return 0;
}
