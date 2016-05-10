#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>


//----------------------------------------------------------------------------------------

const char *socket_name = "/tmp/virtual_serial";

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

int init_connection() {
	ssize_t r;
	size_t addrlen = strlen(socket_name);
	

	char addrbuf[offsetof(struct sockaddr_un, sun_path) + addrlen + 1];
	memset(addrbuf, 0, sizeof(addrbuf));

	struct sockaddr_un *addr = (struct sockaddr_un *)addrbuf;
	addr->sun_family = AF_UNIX;
	memcpy(addr->sun_path, socket_name, addrlen+1);

	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		printf("Failed to create socket: %s\n", strerror(errno));
		return -1;
	}

	if (connect(fd, (struct sockaddr*)addr, sizeof(addrbuf)) < 0) {
		printf("Can't connect to `%s': %s\n", socket_name, strerror(errno));
		return -1;
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



void cat_r(int fd, char *cmd)
{
	char	cmd_buf[1024];
	char	reply[1024];

        cmd_buf[0] = 'r'; cmd_buf[1] = 0;
        strcat(cmd_buf, cmd);
        the_scope->Send(cmd_buf);
        send(fd, the_scope->reply, strlen(the_scope->reply) + 1, 0);
        //gotoxy(1, 10);printf("reply %s\n", the_scope->reply);
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
        //gotoxy(1, 10);printf("reply %s              \n", the_scope->reply);
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
	
	/*gotoxy(1, 9);*/printf("got %s             \n", string);
        
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
	    cat_r(fd, string);
	    return;
        }
        
        if (match(string, ":Sr")) {
	    cat_c(fd, string);
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
 

	printf("*** missed %s\n", string);
}

//-------------------------------------------------------------------------------------


int main()
{
	int	fd;

        //cls();
        fd = init_connection();
	if (fd < 0)
		return 1;

	the_scope = new Scope();
	the_scope->Init();

	//send(fd, buf, r, 0);


	
	char	cur_cmd[1024];
	char	buf[1024];	
	int	cp = 0;

	while (1) {
		int r = recv(fd, buf, sizeof(buf), 0);
		if (r < 0) {
			fprintf(stderr, "read: %s\n", strerror(errno));
			return 1;
		}
		for (int i = 0; i < r; i++) {
			cur_cmd[cp] = buf[i];
			cur_cmd[cp+1] = 0;
			if (buf[i] == '#') {
				process(fd, cur_cmd);
				cp = 0;
			}
			else
				cp++;	
		}
	}
	return 0;
}
