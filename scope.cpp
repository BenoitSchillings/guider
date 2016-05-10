#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <zmq.hpp>
#include <iostream>

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

class Scope {
public:;
    
	Scope();
    	~Scope();
    
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

	int	XCommand(const char *cmd);	
	int	Goto();	
	int 	fd;
    	char 	reply[512]; 
    	char	buf[512];
	char	short_format;
	char	trace;
private:
    	zmq::context_t *ctx;	
	zmq::socket_t  *socket;	
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

int Scope::XCommand(const char* cmd)
{
	Send(cmd);
	
	int result;
	
	result = atoi(reply);
	printf("result is %d\n", result);
	return result;
}

//----------------------------------------------------------------------------------------

int Scope::Send(const char *cmd)
{
	int	len = strlen(cmd) + 1;
	
	zmq::message_t request(len);
	memcpy(request.data(), cmd, len);
	if (trace) printf("sending command %s\n", cmd);

	socket->send(request);

	zmq::message_t msg_reply;
	socket->recv(&msg_reply);

	int	reply_len = msg_reply.size();

	reply[0] = 0;
	
	memcpy(reply, msg_reply.data(), reply_len); 
	if (trace) printf("reply %s\n", reply);	
	return 0;
}

//----------------------------------------------------------------------------------------


int Scope::Init()
{
    ctx = new zmq::context_t(1);
    socket = new zmq::socket_t(*ctx, ZMQ_REQ);
    socket->connect ("tcp://localhost:5555");

    short_format = 0;

    ra_set = NOT_SET;
    dec_set = NOT_SET;
    last_ra = NOT_SET;
    last_dec = NOT_SET;
    last_az = NOT_SET;
    last_el = NOT_SET;

    trace = 0; 
    Send("r:Gt#");

    latitude = GetF();
    if (trace) printf("lat %f\n", latitude); 
    
    Send("r:Gg#");
    longitude = GetF();
    if (trace) printf("long %f\n", longitude);
    last_st = SiderialTime(); 
    return 0;
}

//----------------------------------------------------------------------------------------


Scope::Scope()
{
}

//----------------------------------------------------------------------------------------

Scope::~Scope()
{
}

//----------------------------------------------------------------------------------------

void Scope::Bump(double dx, double dy)
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
        sprintf(buf, "s:Me%03d#", idx);
	Send(buf);
    }
     if (idx < (-min_move)) {
        sprintf(buf, "s:Mw%03d#", -idx);Send(buf);
    }
    if (idy > min_move) {
        sprintf(buf, "s:Ms%03d#", idy);Send(buf);
    }
    if (idy < (-min_move)) {
        sprintf(buf, "s:Mn%03d#", -idy);Send(buf);
    }
    usleep((fabs(dx) + fabs(dy)) * 1000000.0);
}

//----------------------------------------------------------------------------------------


void Scope::Stop()
{
	Send("s:Q#");
}

//----------------------------------------------------------------------------------------


void Scope::Done()
{
	Send("s:RT9#");
}

//----------------------------------------------------------------------------------------


void Scope::Siderial()
{
        Send("s:RT2#");
}

//----------------------------------------------------------------------------------------

void Scope::LongFormat()
{
	Send("s:U#");
}

//----------------------------------------------------------------------------------------


double	Scope::ra_dec_to_elevation(double ra, double dec)
{

        double ha = last_st - ra;
 
	CAACoordinateTransformation	tr;

	CAA2DCoordinate c;
	
	c = tr.Equatorial2Horizontal(ha, dec, latitude);
       	
	return c.Y;
 
}

//----------------------------------------------------------------------------------------

double	Scope::ra_dec_to_azimuth(double ra, double dec)
{
       double ha = last_st - ra;


       CAACoordinateTransformation     tr;

       CAA2DCoordinate c;

       c = tr.Equatorial2Horizontal(ha, dec, latitude);

       return c.X;
}

//----------------------------------------------------------------------------------------

double	Scope::el_az_to_dec(double elevation, double azimuth)
{
        CAACoordinateTransformation     tr;
        CAA2DCoordinate c;
        

        c = tr.Horizontal2Equatorial(azimuth, elevation, latitude);


       return c.Y;
}

//----------------------------------------------------------------------------------------

double	Scope::el_az_to_ra(double elevation, double azimuth)
{
       	CAACoordinateTransformation     tr;
       	CAA2DCoordinate c;

 
	c = tr.Horizontal2Equatorial(azimuth, elevation, latitude);

	double ra = last_st - c.X;
	if (ra < 0) ra = (24.0 + ra);

	return ra; 
}

//----------------------------------------------------------------------------------------


void Scope::Log()
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

	double az = ra_dec_to_azimuth(last_ra, last_dec);
	double el = ra_dec_to_elevation(last_ra, last_dec);

	printf("az %f el %f\n", az, el);

		
	printf("ra %f dec %f\n", el_az_to_ra(el, az), el_az_to_dec(el, az));	
	return;
}

//----------------------------------------------------------------------------------------

void  Scope::test_conversions()
{
	double	ra;
	double	dec;
	double	az;
	double	el;

	latitude = 37;
	longitude = 100;
	last_st = 6.0;
	double el1 = ra_dec_to_elevation(15.5, -44);
	double az1 = ra_dec_to_azimuth(15.5, -44);	
	printf("el = %f\n", el1);
	printf("az = %f\n", (az1 + 180.0));

	double ra1 = el_az_to_ra(el1, az1);
	double dec1 = el_az_to_dec(el1, az1);

	printf("ra1 %f, dec1 %f\n", ra1, dec1);
}


//----------------------------------------------------------------------------------------

double Scope::GetF()
{
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

double Scope::GetF_RA()
{
        int     hh, mm;
	float	ss;
        
	sscanf(reply, "%d:%d:%f#", &hh, &mm, &ss);
        
	double v = hh + (mm /60.0) + (ss / 3600.0);
        return v;
}



//----------------------------------------------------------------------------------------

double Scope::Dec()
{	
	Send("r:GD#");
	return(GetF());
}

//----------------------------------------------------------------------------------------

double Scope::RA()
{
        Send("r:GR#");
        return(GetF_RA());
}

//----------------------------------------------------------------------------------------

double Scope::LocalTime()
{
        Send("r:GL#");
        return GetF_RA();
}


//----------------------------------------------------------------------------------------

double Scope::SiderialTime()
{
	Send("r:GS#");
	return GetF_RA();
}

//----------------------------------------------------------------------------------------

double Scope::Azimuth()
{
        Send("r:GZ#");
        return(GetF());
}


//----------------------------------------------------------------------------------------

double Scope::Elevation()
{	
	Send("r:GA#");
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

int Scope::SetRA(double ra)
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

	sprintf(buf, "c:Sr %d:%d:%d.%d#", hour, min, sec, sec10);

	Send(buf);
	GetCC();
	
	return 0; 
}


//----------------------------------------------------------------------------------------

int Scope::SetDec(double dec)
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

        sprintf(buf, "c:Sd %c%d*%d:%d#", sign, d, m, s);
	if (trace) printf("%s\n", buf);
	Send(buf);
	GetCC();

        return 0;
}

//----------------------------------------------------------------------------------------

int Scope::Goto()
{
	if (dec_set == NOT_SET) {
		printf("no dec defined\n");
		return -1;	
	}
	if (ra_set == NOT_SET) {
		printf("no ra defined\n");
		return -1;
	}
	
	Send("c:MS#");
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

int Scope::SetRate(double ra, double dec)
{
	ra = ra;
	dec = dec;

	char	buf[256];
	char	sign;

	if (ra >= 0) {
		sign = '+';
	}
	else {
		sign = '-';	
		ra = -ra;	
	}
	
	sprintf(buf, "c:RR %c%f#", sign, ra);
	Send(buf);

	if (dec >= 0) {
		sign = '+'; 
	}
	else {
		sign = '-';
		dec = -dec;
	}

	sprintf(buf, "c:RD %c%f#", sign, dec);
	Send(buf);
}

//----------------------------------------------------------------------------------------

int Scope::GetCC()
{
    long        total_time = 0;
    char        c;
    int		n;

    if (trace) printf("%c\n", reply[0]);
    return 0;
}


//----------------------------------------------------------------------------------------

int Scope::Reply()
{
    if (trace) printf("%s\n", reply); 
    return strlen(reply);
}
