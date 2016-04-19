#include "ASICamera.h"
#include <sys/time.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <time.h>
#include <opencv2/core/core.hpp> 
#include <opencv2/highgui/highgui.hpp>
#include "./tiny/tinyxml.h"
#include <signal.h>

bool sim = false;

#include "ser.cpp"
#include "ap.cpp"

AP *ap;
//--------------------------------------------------------------------------------------

float get_value(const char *name);
void  set_value(const char *name, float value);

//--------------------------------------------------------------------------------------

using namespace cv;
using namespace std;

//--------------------------------------------------------------------------------------

#define ushort unsigned short
#define uchar unsigned char
#define PTYPE unsigned short

//--------------------------------------------------------------------------------------


float  g_gain = 150.0;
float  g_mult = 2.0;
float  g_exp = 0.1;

float gain0 = -1;
float exp0 = -1;

//--------------------------------------------------------------------------------------

void blit(Mat from, Mat to, int x1, int y1, int w, int h, int dest_x, int dest_y)
{
	ushort *source = (ushort*)(from.data);
	ushort *dest = (ushort*)(to.data);
	
	int	rowbyte_source = from.step/2;
	int	rowbyte_dest = to.step/2;

	if ((dest_x + w) >= to.cols) {
		w = -1 + to.cols - dest_x;	
	}
	
	if ((dest_y + h) >= to.rows) {
		h = -1 + to.rows - dest_y;
	}

	if ((x1 + w) >= from.cols) {
		w = -1 + from.cols - x1;	
	}
	
	if ((y1 + h) >= from.rows) {
		h = -1 + from.rows - y1;	
	}
	
	for (int y = y1; y < y1 + h; y++) {
		int	dest_yc = dest_y + y;
	
		int off_y_dst = rowbyte_dest * dest_yc + dest_x + x1;
		int src_y_dst = rowbyte_source * y + x1;
		
		memcpy(dest + off_y_dst, source + src_y_dst, w*2);	
	}
}

//--------------------------------------------------------------------------------------

void cvText(Mat img, const char* text, int x, int y)
{
        putText(img, text, Point2f(x, y), FONT_HERSHEY_PLAIN, 1, CV_RGB(62000, 62000, 62000), 1, 8);
}

//--------------------------------------------------------------------------------------

void center(Mat img)
{
	rectangle(img, Point(700-10, 500-10), Point(700+10, 500+10), Scalar(9000, 9000, 9000), 1, 8);
}

void DrawVal(Mat img, const char* title, float value, int y, const char *units)
{
        char    buf[512];

        y *= 30;
        y += 35;
        int x = 60;

        rectangle(img,
                  Point(x - 7, y - 16), Point(x + 300, y + 6),
                  Scalar(9000, 9000, 9000),
                  -1,
                  8);

        rectangle(img,
                  Point(x - 7, y - 16), Point(x + 300, y + 6),
                  Scalar(20000, 20000, 20000),
                  1,
                  8);

        sprintf(buf, "%s=%3.3f %s", title, value, units);
        cvText(img, buf, x, y);
}

//--------------------------------------------------------------------------------------


class Planet {
public:	
		Planet();
		~Planet();

	bool 	GetFrame();

    	int 	width;
    	int 	height;
	Mat 	image;
	Mat	temp_image;	
	int	ref_x;
	int	ref_y;
	int	frame;
	float	gain;
	float	exp;
	float	background;
	float	dev;
private:
	void 	InitCam(int cx, int cy, int width, int height);
	
public:
	bool 	FindGuideStar();
	Mat	GuideCrop();
	void	MinDev();
	void	Move(float dx, float dy);
};

//--------------------------------------------------------------------------------------

void Planet::MinDev()
{
	float	count = 0;
	float	sum = 0;

	background = 1e9;

	for (int y = 1; y < (height-1); y += 20) {
		for (int x = 1; x < (width-1); x += 20) { 
            		float v = image.at<unsigned short>(y, x);
			float v1 = image.at<unsigned short>(y, x + 1);

			if (v < background) background = v;
			count += 1;
			float v2 = (v1-v);
			sum += (v2*v2);
		}
	}
	sum = sum / count;
	sum /= 2.0;	
	dev = sqrt(sum);
	printf("t = %d bg = %f, dev = %f\n", time(0) % 60, background, dev);
}

//--------------------------------------------------------------------------------------

	Planet::Planet()
{
	width = 2048; 
	height = 2048;
	frame = 0;
	background = 0;
	dev = 100;

   	image = Mat(Size(width, height), CV_16UC1);
	temp_image = Mat(Size(width, height), CV_16UC1);


	ref_x = -1;
	ref_y = -1;

	if (!sim) InitCam(0, 0, width, height);
}

//--------------------------------------------------------------------------------------

void Planet::InitCam(int cx, int cy, int width, int height)
{
    int CamNum=0;
    bool bresult;
  
 
    int numDevices = getNumberOfConnectedCameras();
    if(numDevices <= 0) {
       	printf("no device\n"); 
	return;	
	exit(-1);
    }
    
    CamNum = 0;
    
    bresult = openCamera(CamNum);
    if(!bresult) {
        printf("could not open camera\n");
        exit(-1);
    }
    
    
    initCamera(); //this must be called before camera operation. and it only need init once
    printf("resolution %d %d\n", getMaxWidth(), getMaxHeight()); 

    setImageFormat(width, height, 1, IMG_RAW16);
    setValue(CONTROL_BRIGHTNESS, 100, false);
    setValue(CONTROL_GAIN, 0, false);
    printf("max %d\n", getMax(CONTROL_BANDWIDTHOVERLOAD)); 

    //setValue(CONTROL_BANDWIDTHOVERLOAD, 76, false); //lowest transfer speed
    setValue(CONTROL_EXPOSURE, 10, false);
    //setValue(CONTROL_HIGHSPEED, 1, false);
    setStartPos(0, 0);
    printf("init done\n");
}

//--------------------------------------------------------------------------------------

bool Planet::GetFrame()
{
     	frame++;
 
	if (sim) {
	
               rectangle(image,
                          Point( 0, 0),
                          Point(1000, 1000),
                          Scalar(0, 0, 0),
                          -1,
                          8);
	
       		char buf[512];

        	for (int y = 0; y < height; y+=30) {
                	for (int x = 0; x < width; x+=100) {
                        	sprintf(buf, "%d.%d", frame, x);
                        	cvText(image, buf, x, y);
                	}
        	}
	
		return true; 
	
	}
 
	bool got_it;
       	int total = 0; 
	
	//getImageAfterExp
        got_it = getImageData(image.ptr<uchar>(0), width * height * sizeof(PTYPE), -1);
	

	if (!got_it) {
		printf("bad cam\n");
		//exit(-1);	
	}
	
	return got_it;
}

//--------------------------------------------------------------------------------------


void hack_gain_upd(Planet *aguide)
{
        float gain = cvGetTrackbarPos("gain", "video");
        float exp = cvGetTrackbarPos("exp", "video");
        exp = exp / 1000.0;
        g_exp = exp;
	g_gain = gain;
	g_mult = cvGetTrackbarPos("mult", "video")/10.0; 
	if (exp0 != exp || gain0 != gain) {
           
	    if (sim == 0) { 
       	    	setValue(CONTROL_GAIN, gain, false);
            	setValue(CONTROL_EXPOSURE, exp*1000000, false);
            	setValue(CONTROL_BRIGHTNESS, 200, false);
            } 
	    
	    gain0 = gain;
            exp0 = exp;
    	    aguide->gain = gain;
	    aguide->exp = exp;
        }
}

//--------------------------------------------------------------------------------------

void ui_setup()
{
        namedWindow("video", 1);
        createTrackbar("gain", "video", 0, 600, 0);
        createTrackbar("exp", "video", 0, 1000, 0);
        createTrackbar("mult", "video", 0, 100, 0);
        createTrackbar("Sub", "video", 0, 500, 0);
        
	setTrackbarPos("gain", "video", g_gain);
        setTrackbarPos("exp", "video", 1000.0 * g_exp);
        setTrackbarPos("mult", "video", 10.0 *g_mult);
}

//--------------------------------------------------------------------------------------


int find_guide()
{
    Planet *g;
    
    g = new Planet();
    
    ui_setup();
    hack_gain_upd(g);
    
    startCapture();
   
    FILE *out;

    char buf[512];
	
    sprintf(buf, "/media/benoit/18A6395AA6393A18/video/out%ld.ser", time(0));
    out = fopen(buf, "wb"); 
    write_header(out, g->height, g->width, 1000);
    int cnt = 0;
 
    while(1) {
        //ap->Log(); 
	g->GetFrame();
	//fwrite(g->image.ptr<uchar>(0), 1, g->width*g->height*2, out);	
	cnt++;	
	if (g->frame % 20 == 0) { 
		g->MinDev();	
		center(g->image);	
        	DrawVal(g->image, "exp ", g->exp, 0, "sec");
        	DrawVal(g->image, "gain", g->gain, 1, "");
        	DrawVal(g->image, "frame", g->frame*1.0, 2, "");
 	
	        g->image = g->image * (0.1 * cvGetTrackbarPos("mult", "video"));
 	
		cv::imshow("video", g->image);
        	char c = cvWaitKey(1);
        	hack_gain_upd(g);
        
		if (c == 27) {
            		fseek(out, 0, SEEK_SET);
			write_header(out, g->height, g->width, cnt);	
			stopCapture();
            		closeCamera(); 
	    		return 0; 
        	}
   	} 
    }	
}

//--------------------------------------------------------------------------------------



bool match(char *s1, const char *s2)
{
	return(strncmp(s1, s2, strlen(s2)) == 0);
}

//--------------------------------------------------------------------------------------

void help(char **argv)
{
                printf("%s -h        print this help\n", argv[0]);
                printf("%s -f        full field find star\n", argv[0]);
		printf("exta args\n");
                printf("-gain=value\n");
                printf("-exp=value (in sec)\n");
                printf("-mult=value\n");
		printf("complex example\n");
		printf("./planet -exp=0.5 -gain=300 -mult=4 -f\n");
}

//--------------------------------------------------------------------------------------


void intHandler(int dummy=0) {
   	closeCamera(); 
	printf("emergency close\n");    exit(0);
}

//--------------------------------------------------------------------------------------


int main(int argc, char **argv)
{
	signal(SIGINT, intHandler);
	
        if (argc == 1 || strcmp(argv[1], "-h") == 0) {
               	help(argv);
       		return 0; 
	}

	int pos = 1;

	g_exp = get_value("exp");
	g_gain = get_value("gain");
	g_mult = get_value("mult");
	
	while(pos < argc) {
		if (match(argv[pos], "-gain=")) {sscanf(strchr(argv[pos], '=') , "=%f",  &g_gain); argv[pos][0] = 0;}
        	if (match(argv[pos], "-exp="))  {sscanf(strchr(argv[pos], '=') , "=%f",  &g_exp); argv[pos][0] = 0;}
        	if (match(argv[pos], "-mult=")) {sscanf(strchr(argv[pos], '=') , "=%f",  &g_mult); argv[pos][0] = 0;}
		pos++;
	}
        pos = 1;
	set_value("exp", g_exp);
	set_value("gain", g_gain);
	set_value("mult", g_mult);

        while(pos < argc) {
		if (match(argv[pos], "-f")) find_guide();
		pos++;
        }

       set_value("exp", g_exp);
       set_value("gain", g_gain);
       set_value("mult", g_mult);
}