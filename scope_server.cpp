#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <zmq.hpp>


//----------------------------------------------------------------------------------------

class ScopeServer {
public:;
    
	ScopeServer();
    	~ScopeServer();
    
	int	Init();
    	int	Send(const char*);
    	int	Reply();
    	int	EatReply();	
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
        
        
        int	HandleXCommands(const char *);

	int 	fd;
    	int	focus_fd;
 	int	err_count;	
	char 	reply[512]; 
    	char	buf[512];
	char	short_format;
	char	trace;
private:
    	double	GetF();
	double	GetF_RA();

	int	dither_request;
	int	need_guiding;
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

const char *portname = "/dev/ttyUSB0";
const char *portname1= "/dev/ttyUSB1";
const char *focusport="/dev/ttyACM0";

int ScopeServer::Init()
{
    short_format = 0;
    dither_request = 0;
    need_guiding = 1; 
    trace = 0; 
    err_count = 0;
 
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
    Send(":U#");
    Send(":Bd 00*00:05#");
    Send(":pS#");
    Reply(); 
    printf("reply is %s\n", reply); 
    focus_fd = open(focusport, O_RDWR | O_NOCTTY | O_SYNC);
    set_interface_attribs (focus_fd, B9600, 0);  // set speed to 115,200 bps, 8n1 (no parity)
    set_blocking (focus_fd, 0);                // set no blocking
 
    return 0;
}

//----------------------------------------------------------------------------------------


ScopeServer::ScopeServer()
{
}

//----------------------------------------------------------------------------------------

ScopeServer::~ScopeServer()
{
}


//----------------------------------------------------------------------------------------


int ScopeServer::HandleXCommands(const char * s)
{
    printf("got x command %s\n", s);

    if (strcmp(s, "dither") == 0) {
  	int tmp = dither_request;
	
	dither_request = 0; 
	return tmp; 
    }

	
    if (strcmp(s, "reqdither") == 0) {
	dither_request = 1;
	return 0;
    }	
  
    if (strcmp(s, "pause_guiding") == 0) {
   	need_guiding = 0; 
    }
    
    if (strcmp(s, "start_guiding") == 0) {
   	need_guiding = 1; 
    }
    
    if (strcmp(s, "need_guiding") == 0) {
   	return need_guiding; 
    }

 
    if (strncmp(s, "focus", 5) == 0) {
	int move;	
	sscanf(s, "focus%d", &move);
	printf("move focus %d\n", move);
        char buf[256];
        sprintf(buf, "#m%d\n", move);
        write(focus_fd, buf, strlen(buf));
   	usleep(300000); 
    }

    return 1234;
}

//----------------------------------------------------------------------------------------

int ScopeServer::Send(const char *cmd)
{
   	if (trace) printf("cmd %s\n", cmd); 
	return write(fd, cmd, strlen(cmd));
}


//----------------------------------------------------------------------------------------


void ScopeServer::Stop()
{
	Send(":Q#");
}

//----------------------------------------------------------------------------------------


void ScopeServer::Done()
{
	Send(":RT9#");
}

//----------------------------------------------------------------------------------------


void ScopeServer::Log()
{

	
	double last_az = Azimuth();
	double last_el = Elevation();
	double last_st = SiderialTime();	
	double last_ra = RA();
	double last_dec = Dec();

	printf("stime = %f\taz = %f\t el = %f\t ra = %f\t dec = %f\n", last_st, last_az, last_el, last_ra, last_dec);
	if ( last_dec < -20 || last_dec > 70 || last_el < 20.0 || last_az > 270) {
		err_count++;
		Send("#");
		if(err_count > 15) {	
			Stop();	
			Done();
			printf("emergency limit\n");
			err_count = 0;	
		}
		//exit(-1); 	
	}

}

//----------------------------------------------------------------------------------------

double ScopeServer::GetF()
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

double ScopeServer::GetF_RA()
{
        Reply();
        int     hh, mm;
	float	ss;
        
	sscanf(reply, "%d:%d:%f#", &hh, &mm, &ss);
        
	double v = hh + (mm /60.0) + (ss / 3600.0);
        return v;
}



//----------------------------------------------------------------------------------------

double ScopeServer::Dec()
{	
	Send(":GD#");
	return(GetF());
}

//----------------------------------------------------------------------------------------

double ScopeServer::RA()
{
        Send(":GR#");
        return(GetF_RA());
}

//----------------------------------------------------------------------------------------

double ScopeServer::LocalTime()
{
        Send(":GL#");
        return GetF_RA();
}


//----------------------------------------------------------------------------------------

double ScopeServer::SiderialTime()
{
	Send(":GS#");
	return GetF_RA();
}

//----------------------------------------------------------------------------------------

double ScopeServer::Azimuth()
{
        Send(":GZ#");
        return(GetF());
}


//----------------------------------------------------------------------------------------

double ScopeServer::Elevation()
{	
	Send(":GA#");
	return(GetF());
}

//----------------------------------------------------------------------------------------

char ScopeServer::GetCC()
{
    long        total_time = 0;
    char        c;
    int		n;

    usleep(15000);
    do {
        total_time += 100000;
        n = read(fd, &c, 1);
    } while(n == 0 && total_time<3000000);
    if (trace) printf("%c\n", c);
    return c;
}


//----------------------------------------------------------------------------------------

int ScopeServer::EatReply()
{
	usleep(400*1000);	//wait 1 second
	int cnt = read(fd, &reply, 80);	//read up to 33 char
	printf("eat = %d\n", cnt);
	return cnt;
}

//----------------------------------------------------------------------------------------


int ScopeServer::Reply()
{
    reply[0] = 0;
    int	idx = 0;
    long	total_time = 0;
    char	c;
   
    usleep(15000); 
    do {
        total_time += 100000;
        int n = read(fd, &c, 1);
        if (n > 0) {
            reply[idx] = c;
 	    reply[idx + 1] = 0;            
	    idx++;
        }	
    } while(c != '#' && total_time< 3000000);
    if (trace) printf("%s\n", reply); 
    return idx;
}

//----------------------------------------------------------------------------------------

void cls()
{
        printf( "%c[2J", 27);
}

//----------------------------------------------------------------------------------------


void gotoxy(int x, int y)
{
        printf("%c[%d;%df",0x1B,y,x);
	printf( "%c[2J", 0x1b);
}


//----------------------------------------------------------------------------------------


int main()
{
    ScopeServer  *scope;
    char      command[1024];
 
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REP);
    socket.bind ("tcp://*:5555");

    int timeout = 20;
    socket.setsockopt (ZMQ_RCVTIMEO, &timeout, sizeof (int));
 
    scope = new ScopeServer();
  
    int error = 0;
 
    do { 
	error = scope->Init();
	if (error != 0) {
		usleep(1000*1000);		//sleep for 1 second 
	}
    } while(error != 0);


    int check = 0;

     while(1) {
        check++; 
       	zmq::message_t request;
        int result = socket.recv (&request);
	if (request.size() > 0) {
		memcpy(command, request.data(), request.size());
		if (command[0] == 'x') {
			int result = scope->HandleXCommands(command + 1);
			sprintf(scope->reply, "%d", result);	
		}

		if (command[0] == 's') {	//silent command. not waiting for reply from the mount
			scope->Send(command + 1);
			scope->reply[0] = 0;
		}

		if (command[0] == 'c') {
			scope->Send(command + 1);
			scope->reply[0] = scope->GetCC();	
			scope->reply[1] = 0;
		}

		if (command[0] == 'r') {
			scope->Send(command + 1);
			scope->Reply();	
		}
		if (command[0] == 'f') {	//f**** up command
			scope->Send(command + 1);
			scope->EatReply();
			scope->reply[0] = 0;	
		}

		if (scope->trace) {
			printf("server:: in command %s\n", command);
			printf("server:: reply %s\n", scope->reply);
		}	
		zmq::message_t msg_reply(strlen(scope->reply) + 1);
        	memcpy (msg_reply.data (), scope->reply, strlen(scope->reply) + 1);
        	socket.send(msg_reply);
	}
	if (check % 25 == 0) {/*gotoxy(1, 1),*/ scope->Log();/*gotoxy(1, 5);*/}
    }
    
    return 0;
}
