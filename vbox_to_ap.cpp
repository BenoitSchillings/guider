#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

//----------------------------------------------------------------------------------------

const char *socket_name = "/tmp/virtual_serial";

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



void process(int fd, char *string)
{
        char    reply[1024];
	
	printf("got %s\n", string);
        
        if (match(string, ":V#")) {
            strcpy(reply, "5.31#");
            printf("sending reply %s\n", reply); 
	    send(fd, reply, strlen(reply) + 1, 0);
            return;
        }
}

//----------------------------------------------------------------------------------------


int main()
{
	int	fd;

	fd = init_connection();
	if (fd < 0)
		return 1;

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
