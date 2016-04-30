#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <zmq.hpp>


//----------------------------------------------------------------------------------------

class APServer {
public:;
    
	APServer();
    	~APServer();
    
	int	Init();
    	int	Send(const char*);
    	int	Reply();
    	char	GetCC();
    	void	Stop();
    	void	Done();
    	void	Siderial(); 
	void	LongFormat();	
	void	Log(); 
    	double	Elevation();
    	double	Dec(); 
    	double	RA();	
	double	Azimuth(); 
  	double	SiderialTime();	
	double	LocalTime();	

	int 	fd;
    	char 	reply[512]; 
    	char	buf[512];
	char	short_format;
	char	trace;
private:
    	double	GetF();
	double	GetF_RA();
};

//----------------------------------------------------------------------------------------


int set_interface_attribs (int fd, int speed, int parity)
{
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0)
    {
        printf("error %d from tcgetattr\n", errno);
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
        printf("error %d from tcsetattr\n", errno);
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
        printf("error %d from tggetattr\n", errno);
        return;
    }
    
    tty.c_cc[VMIN]  = should_block ? 1 : 0;
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout
    
    if (tcsetattr (fd, TCSANOW, &tty) != 0)
        printf("error %d setting term attributes\n", errno);
}


//----------------------------------------------------------------------------------------

const char *portname = "/dev/ttyUSB1";
const char *portname1= "/dev/ttyUSB0";

int APServer::Init()
{
    short_format = 0;

    trace = 0; 
    
    fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
    
    if (fd < 0) {
        printf("error %d opening %s: %s\n", errno, portname, strerror (errno));
   	return -1; 
    }
   
    if (fd < 0) { 
    	fd = open (portname1, O_RDWR | O_NOCTTY | O_SYNC);
    	if (fd < 0) {
        	printf("error %d opening %s: %s\n", errno, portname1, strerror (errno));
        	return(-1);
	}
    }

    set_interface_attribs (fd, B9600, 0);  // set speed to 115,200 bps, 8n1 (no parity)
    set_blocking (fd, 0);                // set no blocking
    
    Send("#");
    Send("U#");
    return 0;
}

//----------------------------------------------------------------------------------------


APServer::APServer()
{
}

//----------------------------------------------------------------------------------------

APServer::~APServer()
{
}

//----------------------------------------------------------------------------------------

int APServer::Send(const char *cmd)
{
   	if (trace) printf("cmd %s\n", cmd); 
	return write(fd, cmd, strlen(cmd));
}


//----------------------------------------------------------------------------------------


void APServer::Stop()
{
	Send(":Q#");
}

//----------------------------------------------------------------------------------------


void APServer::Done()
{
	Send(":RT9#");
}

//----------------------------------------------------------------------------------------


void APServer::Log()
{

	
	double last_az = Azimuth();
	double last_el = Elevation();
	double last_st = SiderialTime();	
	double last_ra = RA();
	double last_dec = Dec();

	printf("stime = %f\taz = %f\t el = %f\t ra = %f\t dec = %f\n", last_st, last_az, last_el, last_ra, last_dec);
	
	if (last_dec < -20 || last_dec > 70 || last_el < 20.0 || last_az > 195) {
		Stop();	
		Done();
		printf("emergency limit\n");
		exit(-1); 	
	}

}

//----------------------------------------------------------------------------------------

double APServer::GetF()
{
	Reply();
	int	a,b,c;
	
	if (short_format == 1) {
		sscanf(reply, "%d*%d#", &a, &b);
		c = 0;	
	}
	else {
		sscanf(reply, "%d*%d:%d#", &a, &b, &c);
	}	
	double v = a + (b /60.0) + (c / 3600.0);
	return v;
}

//----------------------------------------------------------------------------------------

double APServer::GetF_RA()
{
        Reply();
        int     hh, mm;
	float	ss;
        
	sscanf(reply, "%d:%d:%f#", &hh, &mm, &ss);
        
	double v = hh + (mm /60.0) + (ss / 3600.0);
        return v;
}



//----------------------------------------------------------------------------------------

double APServer::Dec()
{	
	Send(":GD#");
	return(GetF());
}

//----------------------------------------------------------------------------------------

double APServer::RA()
{
        Send(":GR#");
        return(GetF_RA());
}

//----------------------------------------------------------------------------------------

double APServer::LocalTime()
{
        Send(":GL#");
        return GetF_RA();
}


//----------------------------------------------------------------------------------------

double APServer::SiderialTime()
{
	Send(":GS#");
	return GetF_RA();
}

//----------------------------------------------------------------------------------------

double APServer::Azimuth()
{
        Send(":GZ#");
        return(GetF());
}


//----------------------------------------------------------------------------------------

double APServer::Elevation()
{	
	Send(":GA#");
	return(GetF());
}

//----------------------------------------------------------------------------------------

char APServer::GetCC()
{
    long        total_time = 0;
    char        c;
    int		n;

    usleep(15000);
    do {
        usleep(1000);
        total_time += 1000;

        n = read(fd, &c, 1);
    } while(n == 0 && total_time<1000000);
    if (trace) printf("%c\n", c);
    return c;
}


//----------------------------------------------------------------------------------------

int APServer::Reply()
{
    reply[0] = 0;
    int	idx = 0;
    long	total_time = 0;
    char	c;
   
    usleep(15000); 
    do {
        total_time += 200000;
        //printf("reply wait\n"); 
        int n = read(fd, &c, 1);
        if (n > 0) {
            reply[idx] = c;
 	    reply[idx + 1] = 0;            
	    idx++;
        }	
    } while(c != '#' && total_time<100000000);
    if (trace) printf("%s\n", reply); 
    return idx;
}

//----------------------------------------------------------------------------------------



int main()
{
    APServer  *ap;
    char      command[1024];
 
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REP);
    socket.bind ("tcp://*:5555");

    int timeout = 250;
    socket.setsockopt (ZMQ_RCVTIMEO, &timeout, sizeof (int));
 
    ap = new APServer();
  
    int error = 0;
 
    do { 
	error = ap->Init();
	if (error != 0) {
		usleep(1000*1000);		//sleep for 1 second 
	}
    } while(error != 0);


     while(1) {
        
       	zmq::message_t request;
        int result = socket.recv (&request);
	if (request.size() > 0) {
		memcpy(command, request.data(), request.size());
		printf("got request %s\n", command);

		if (command[0] == 's') {	//silent command. not waiting for reply from the mount
			ap->Send(command + 1);
			ap->reply[0] = 0;
		}

		if (command[0] == 'c') {
			ap->Send(command + 1);
			ap->reply[0] = ap->GetCC();	
			ap->reply[1] = 0;
		}

		if (command[0] == 'r') {
			ap->Send(command + 1);
			ap->Reply();	
		}
		if (ap->trace) {
			printf("server:: in command %s\n", command);
			printf("server:: reply %s\n", ap->reply);
		}	
		zmq::message_t msg_reply(strlen(ap->reply) + 1);
        	memcpy (msg_reply.data (), ap->reply, strlen(ap->reply) + 1);
        	socket.send(msg_reply);
	}
	ap->Log();
    }
    
    return 0;
}
