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
    
	int	Init();
    	int	Send(const char*);
    	int	Reply();
    	void	Bump(float dx, float dy);
    	void	Stop();
    	void	Done();
    	void	Siderial(); 
    	void	Log(); 
    	float	Elevation();
    	float	Dec(); 
    	float	Azimuth(); 
    
	int 	fd;
    	char 	reply[512]; 
    	char	buf[512];

private:
    	float	GetF();
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
   	printf("cmd %s\n", cmd); 
	return write(fd, cmd, strlen(cmd));
}

//----------------------------------------------------------------------------------------

void AP::Bump(float dx, float dy)
{
	
    int min_move = 5;
    int idx = dx * 1000.0;
    int idy = dy * 1000.0;

    printf("bump %d %d\n", idx, idy); 
    if (idx>999) idx = 999; 
    if (idx<-999) idx = -999;
    if (idy>999) idy = 999;
    if (idy<-999) idy = -999;
 
    if (idx > min_move) {
        sprintf(buf, ":Me%03d#", idx);Send(buf);
    }
     if (idx < (-min_move)) {
        sprintf(buf, ":Mw%03d#", -idx);Send(buf);
    }
    if (idy > min_move) {
        sprintf(buf, ":Ms%03d#", idy);Send(buf);
    }
    if (idy < (-min_move)) {
        sprintf(buf, ":Mn%03d#", -idy);Send(buf);
    }
    usleep((fabs(dx) + fabs(dy)) * 1000000.0);
}

//----------------------------------------------------------------------------------------


void AP::Stop()
{
	Send(":Q#");
}

//----------------------------------------------------------------------------------------


void AP::Done()
{
	Send(":RT9#");
}

//----------------------------------------------------------------------------------------


void AP::Siderial()
{
        Send(":RT2#");
}

//----------------------------------------------------------------------------------------

void AP::Log()
{
	Send(":GD#");
        Reply();
        printf("%s ", reply);
        Send(":GR#");
        Reply();
        printf("%s\n", reply);
	printf("%f %f\n", Azimuth(), Elevation());
}

//----------------------------------------------------------------------------------------

float AP::GetF()
{
	Reply();
	int	a,b,c;
	sscanf(reply, "%d*%d:%d#", &a, &b, &c);
	float v = a + (b /60.0) + (c / 3600.0);
	return v;
}


//----------------------------------------------------------------------------------------

float AP::Dec()
{	
	Send(":GD#");
	return(GetF());
}

//----------------------------------------------------------------------------------------

float AP::Azimuth()
{
        Send(":GZ#");
        return(GetF());
}


//----------------------------------------------------------------------------------------

float AP::Elevation()
{	
	Send(":GA#");
	return(GetF());
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

int tmain()
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