#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

//----------------------------------------------------------------------------------------

class AP {
public:;

    AP();
    ~AP();
    
    int Init();
    int Send(const char*);
    int Reply();
 
    int fd;
    char reply[512]; 
};

//----------------------------------------------------------------------------------------


int set_interface_attribs (int fd, int speed, int parity)
{
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0)
    {
        printf("error %d from tcgetattr", errno);
        return -1;
    }
    
    cfsetospeed (&tty, speed);
    cfsetispeed (&tty, speed);
    
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    tty.c_iflag &= ~IGNBRK;         // disable break processing
    tty.c_lflag = 0;                // no signaling chars, no echo,
    // no canonical processing
    tty.c_oflag = 0;                // no remapping, no delays
    tty.c_cc[VMIN]  = 0;            // read doesn't block
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout
    
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl
    
    tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
    // enable reading
    tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    
    if (tcsetattr (fd, TCSANOW, &tty) != 0)
    {
        printf("error %d from tcsetattr", errno);
        return -1;
    }
    return 0;
}

//----------------------------------------------------------------------------------------

void set_blocking (int fd, int should_block)
{
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0)
    {
        printf("error %d from tggetattr", errno);
        return;
    }
    
    tty.c_cc[VMIN]  = should_block ? 1 : 0;
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout
    
    if (tcsetattr (fd, TCSANOW, &tty) != 0)
        printf("error %d setting term attributes", errno);
}


//----------------------------------------------------------------------------------------

const char *portname = "/dev/ttyUSB0";

int AP::Init()
{
    fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0)
    {
        printf("error %d opening %s: %s", errno, portname, strerror (errno));
        return -1;
    }
    
    set_interface_attribs (fd, B9600, 0);  // set speed to 115,200 bps, 8n1 (no parity)
    set_blocking (fd, 0);                // set no blocking
    return 0;
}

//----------------------------------------------------------------------------------------


    AP::AP()
    {
    }
   
//----------------------------------------------------------------------------------------
 
    AP::~AP()
    {
    }

//----------------------------------------------------------------------------------------

int AP::Send(const char *cmd)
{
	return write(fd, cmd, strlen(cmd));
}

//----------------------------------------------------------------------------------------

int AP::Reply()
{
	reply[0] = 0;
	int	idx = 0;
	long	total_time = 0;
	char	c;
	
	do {
		usleep(1000);
		total_time += 1000;

    		int n = read(fd, &c, 1);
		if (n > 0) {
			reply[idx] = c;
			idx++;
		}	
	} while(c != '#' && total_time<1000000);
	return idx;
}

//----------------------------------------------------------------------------------------

int main()
{
    AP  *ap;
    
    ap = new AP();
    
    ap->Init();
    ap->Send(":GR#");
    ap->Reply();
    printf("%s\n", ap->reply); 
    
    ap->Send(":GA#");
    ap->Reply();
    printf("%s\n", ap->reply);
 
    return 0;
}
