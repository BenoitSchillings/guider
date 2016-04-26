#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#include "AACoordinateTransformation.h"
//----------------------------------------------------------------------------------------
#define NOT_SET -32000
//----------------------------------------------------------------------------------------
inline double td(double radians) {
    return radians * (180.0 / M_PI);
}
inline double tr(double degrees) {
    return degrees / (180.0 / M_PI);
}

//----------------------------------------------------------------------------------------

class AP {
public:;
    
	AP();
    	~AP();
    
	int	Init();
    	int	Send(const char*);
    	int	Reply();
    	int	GetCC();	
	void	Bump(double dx, double dy);
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
	int	SetRate(double ra, double dec); 
	int	SetRA(double ra); 
	int	SetDec(double dec);	


	double	ra_dec_to_elevation(double ra, double dec);
	double	ra_dec_to_azimuth(double ra, double dec);
	double	el_az_to_dec(double elevation, double azimuth);
	double	el_az_to_ra(double elevation, double azimuth);

	void	test_conversions();
	
	int	Goto();	
	int 	fd;
    	char 	reply[512]; 
    	char	buf[512];
	char	short_format;
	char	trace;
private:
    	double	GetF();
	double	GetF_RA();

	double	ra_set;
	double	dec_set;

	double	last_ra;
	double	last_dec;
	double	last_az;
	double	last_el;
	double	last_st;
	double	latitude;
	double	longitude;
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

const char *portname = "/dev/ttyUSB1";

int AP::Init()
{
    short_format = 0;

    ra_set = NOT_SET;
    dec_set = NOT_SET;
    last_ra = NOT_SET;
    last_dec = NOT_SET;
    last_az = NOT_SET;
    last_el = NOT_SET;

    trace = 0; 
    fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0)
    {
        printf("error %d opening %s: %s", errno, portname, strerror (errno));
        return -1;
    }
    
    set_interface_attribs (fd, B9600, 0);  // set speed to 115,200 bps, 8n1 (no parity)
    set_blocking (fd, 0);                // set no blocking
    
    Send("#");
    Send("U#");
    Send(":Gt#");

    latitude = GetF();
    printf("lat %f\n", latitude); 
    
    Send(":Gg#");
    longitude = GetF();
    printf("long %f\n", longitude);
    last_st = SiderialTime(); 
    //test_conversions(); 
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
   	if (trace) printf("cmd %s\n", cmd); 
	return write(fd, cmd, strlen(cmd));
}

//----------------------------------------------------------------------------------------

void AP::Bump(double dx, double dy)
{
	
    int min_move = 0;
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

void AP::LongFormat()
{
	Send(":U#");
}

//----------------------------------------------------------------------------------------


double	AP::ra_dec_to_elevation(double ra, double dec)
{

        double ha = last_st - ra;
 	
	CAACoordinateTransformation	tr;

	CAA2DCoordinate c;
	
	c = tr.Equatorial2Horizontal(ha, dec, latitude);
       	
	return c.Y;
 
}

//----------------------------------------------------------------------------------------

double	AP::ra_dec_to_azimuth(double ra, double dec)
{
       double ha = last_st - ra;

       CAACoordinateTransformation     tr;

       CAA2DCoordinate c;

       c = tr.Equatorial2Horizontal(ha, dec, latitude);

       return c.X;
}

//----------------------------------------------------------------------------------------

double	AP::el_az_to_dec(double elevation, double azimuth)
{
        CAACoordinateTransformation     tr;
        CAA2DCoordinate c;
        

        c = tr.Horizontal2Equatorial(azimuth, elevation, latitude);


       return c.Y;
}

//----------------------------------------------------------------------------------------

double	AP::el_az_to_ra(double elevation, double azimuth)
{
       	CAACoordinateTransformation     tr;
       	CAA2DCoordinate c;

 
	c = tr.Horizontal2Equatorial(azimuth, elevation, latitude);
           
 
	return c.X * 15.0;
}

//----------------------------------------------------------------------------------------


void AP::Log()
{
	last_az = Azimuth();
	last_el = Elevation();
	last_st = SiderialTime();	
	last_ra = RA();
	last_dec = Dec();

	//printf("Local is %f\n", LocalTime());	
	printf("stime = %f\taz = %f\t el = %f\t ra = %f\t dec = %f\n", last_st, last_az, last_el, last_ra, last_dec);
	
	if (last_dec < -20 || last_dec > 70 || last_el < 20.0 || last_az > 195) {
		Stop();	
		Done();
		printf("emergency limit\n");
		exit(-1); 	
	}

	printf("az %f el %f\n", ra_dec_to_azimuth(last_ra, last_dec), ra_dec_to_elevation(last_ra, last_dec));
	//printf("ra %f dec %f\n", el_az_to_ra(last_el, last_az), el_az_to_dec(last_el, last_az));	
	return;
}


void  AP::test_conversions()
{
	double	ra;
	double	dec;
	double	az;
	double	el;

	if (0)
	for (az = 0; az < 359; az += 20) {
		for (el = 20; el < 140; el += 10) {
			ra = el_az_to_ra(el, az);
			dec = el_az_to_dec(el, az);
			double az1 = ra_dec_to_azimuth(ra, dec);
			double el1 = ra_dec_to_elevation(ra, dec);

			printf("<%f %f> <%f %f> <%f %f>\n", az, el, ra, dec, az1, el1);
		}
	}

	if (1)
	for (ra = 0; ra < 23.9; ra += 0.5) {
		for (dec = -89; dec < 89; dec+=10) {
			double el = ra_dec_to_elevation(ra, dec);
			double az = ra_dec_to_azimuth(ra, dec);
		
			//if (el > 0 && az > 0) {	
				double ra1;
				double dec1;

				ra1 = el_az_to_ra(el, az);
				dec1 = el_az_to_dec(el, az);

				printf("<%f %f> <%f %f> <%f %f>\n", ra, dec, az, el, ra1, dec1);
			//}	
		}
	}
}


//----------------------------------------------------------------------------------------

double AP::GetF()
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

double AP::GetF_RA()
{
        Reply();
        int     hh, mm;
	float	ss;
        
	sscanf(reply, "%d:%d:%f#", &hh, &mm, &ss);
        
	double v = hh + (mm /60.0) + (ss / 3600.0);
        return v;
}



//----------------------------------------------------------------------------------------

double AP::Dec()
{	
	Send(":GD#");
	return(GetF());
}

//----------------------------------------------------------------------------------------

double AP::RA()
{
        Send(":GR#");
        return(GetF_RA());
}

//----------------------------------------------------------------------------------------

double AP::LocalTime()
{
        Send(":GL#");
        return GetF_RA();
}


//----------------------------------------------------------------------------------------

double AP::SiderialTime()
{
	Send(":GS#");
	return GetF_RA();
}

//----------------------------------------------------------------------------------------

double AP::Azimuth()
{
        Send(":GZ#");
        return(GetF());
}


//----------------------------------------------------------------------------------------

double AP::Elevation()
{	
	Send(":GA#");
	return(GetF());
}

/*
Command: :Sr HH:MM:SS# or :Sr HH:MM:SS.S#
Response: “1”
Defines the commanded Right Ascension, RA. Command may be issued in long or short format regardless of whether long format has been selected. RA specified as DD*MM:SS will also be interpreted properly if greater precision is required, but this is not typically how RA is expressed. This command automatically selects RA-DEC mode. Move and calibrate commands operate on the most recently defined RA if in RA-DEC mode.
Command: :Sd sDD*MM# or :Sd sDD*MM:SS#
Response: “1”
Defines the commanded Declination, DEC. Command may be issued in long or short format regardless of whether long format has been selected. This command automatically selects RA-DEC mode. Move and calibrate commands operate on the most recently defined DEC if in RA-DEC mode.
*/


//----------------------------------------------------------------------------------------

int AP::SetRA(double ra)
{
	char	buf[256];
	int	hour;
	int	min;
	int	sec;
	int	sec10;

	ra_set = ra;

	hour = ra;
	ra -= hour;
	ra *= 60.0;
	min = ra;
	ra -= min;
	ra *= 60.0;
	sec = ra;
	ra -= sec;
	ra *= 10.0;
	sec10 = ra;

	sprintf(buf, ":Sr %d:%d:%d.%d#", hour, min, sec, sec10);

	Send(buf);
	GetCC();
	
	return 0; 
}


//----------------------------------------------------------------------------------------

int AP::SetDec(double dec)
{
        char    buf[256];
        int     d;
        int     m;
        int     s;
	char	sign;



	dec_set = dec;
	
	if (dec < 0) {
		dec = - dec;	
		sign = '-';	
	}
	else {
		sign = '+';
	}
        
	d = dec;
        dec -= d;
        dec *= 60.0;
        m = dec;
        dec -= m;
        dec *= 60.0;
        s = dec;

        sprintf(buf, ":Sd %c%d*%d:%d#", sign, d, m, s);
	if (trace) printf("%s\n", buf);
	Send(buf);
	GetCC();

        return 0;
}

//----------------------------------------------------------------------------------------

int AP::Goto()
{
	if (dec_set == NOT_SET) {
		printf("no dec defined\n");
		return -1;	
	}
	if (ra_set == NOT_SET) {
		printf("no ra defined\n");
		return -1;
	}
	
	Send(":MS#");
	GetCC();
}

//----------------------------------------------------------------------------------------
/*
:RR sxxx.xxxx# Selects the tracking rate in the RA axis to xxx.xxxx
:RD sxxx.xxxx# Selects the tracking rate in the Dec axis to xxx.xxxx
example :
this means to slow down RA by 15arcsec/sec * 0.0448 = 0.072 arcsec/sec
:RR -00.0448#
*/

//Set Rate of 1.0, 0.0 is the normal for RA/DEC

int AP::SetRate(double ra, double dec)
{
	ra = ra - 1.0;
	dec = dec;

	char	buf[256];
	char	sign;

	if (ra > 0) {
		sign = '+';
	}
	else {
		sign = '-';	
		ra = -ra;	
	}
	
	sprintf(buf, ":RR %c%f#", sign, ra);
	Send(buf);

	if (dec >= 0) {
		sign = '+'; 
	}
	else {
		sign = '-';
		dec = -dec;
	}

	sprintf(buf, ":RD %c%f#", sign, dec);
	Send(buf);
	Reply();
}

//----------------------------------------------------------------------------------------

int AP::GetCC()
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
    return 0;
}


//----------------------------------------------------------------------------------------

int AP::Reply()
{
    reply[0] = 0;
    int	idx = 0;
    long	total_time = 0;
    char	c;
   
    usleep(15000); 
    do {
        usleep(1000);
        total_time += 1000;
        
        int n = read(fd, &c, 1);
        if (n > 0) {
            reply[idx] = c;
            idx++;
        }	
    } while(c != '#' && total_time<1000000);
    if (trace) printf("%s\n", reply); 
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
