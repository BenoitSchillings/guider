#include "ASICamera2.h"
#include <sys/time.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <time.h>
#include <opencv2/core/core.hpp> 
#include <opencv2/highgui/highgui.hpp>
#include "./tiny/tinyxml.h"
#include <signal.h>
#include <opencv2/photo.hpp>
bool sim = false;

#include "ser.cpp"
#include "scope.cpp"

Scope *scope;

#include <sys/time.h>

double nanotime()
{
   timespec ts;
   clock_gettime(CLOCK_REALTIME, &ts);

   return (ts.tv_sec + ts.tv_nsec * 1e-9);
}

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
float  g_scale = 0.5;
float  g_crop = 0.25;

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
	int	cx, cy;

	cx = img.cols;	
 	cy = img.rows;
	cx /= 2.0;
	cy /= 2.0;	
        rectangle(img, Point(cx-10, cy-10), Point(cx+10, cy+10), Scalar(9000, 9000, 9000), 1, 8);
        rectangle(img, Point(cx-13, cy-13), Point(cx+13, cy+13), Scalar(9000, 9000, 9000), 1, 8);
        rectangle(img, Point(cx-12, cy-12), Point(cx+12, cy+12), Scalar(9000, 9000, 9000), 1, 8);
        rectangle(img, Point(cx-11, cy-11), Point(cx+11, cy+11), Scalar(9000, 9000, 9000), 1, 8);


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
int bin = 1;


class Planet {
public:	
		Planet();
		~Planet();

	bool 	GetFrame();
        bool    GetFrame1();

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
	float	bias;
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
	bias = background - 500;	
	printf("t = %d bg = %f, dev = %f\n", time(0) % 60, background, dev);
}

//--------------------------------------------------------------------------------------

	Planet::Planet()
{
	width = 3096/bin; 
	height = 2048/bin;
	width *= g_crop;
	height *= g_crop; 
  	width &= 0xffff0;
	height &= 0xffff0;	
	printf("%d %d\n", width, height);	
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
  
 
    CamNum = 0; 
   
    ASI_CAMERA_INFO info;

    ASIGetCameraProperty(&info, 0);
 
    bresult = ASIOpenCamera(0);
    
    if(bresult != 0) {
        printf("could not open camera\n");
        exit(-1);
    }
    
    
    ASIGetCameraProperty(&info, 0);

    int mw = info.MaxWidth;
    int mh = info.MaxHeight;

    int dx = (mw - width) / 2;
    int dy = (mh - height)/ 2;
    printf("resolution %s, %d %d\n", info.Name, mw, mh); 

    //setImageFormat(width, height, bin, IMG_RAW16);
    //setValue(CONTROL_BRIGHTNESS, 100, ASI_FALSE);
    //setValue(CONTROL_GAIN, 0, false);
    //printf("max %d\n", getMax(CONTROL_BANDWIDTHOVERLOAD)); 

    ASISetControlValue(0, ASI_BANDWIDTHOVERLOAD, 70, ASI_FALSE); //lowest transfer speed
    ASISetControlValue(0, ASI_HIGH_SPEED_MODE, 0, ASI_FALSE);
 
    ASISetControlValue(0, ASI_AUTO_MAX_EXP, 10L, ASI_TRUE); 
    //setValue(AUTO_MAX_EXP, 3L, false); 
    //bool foo;

    ASISetControlValue(0, ASI_COOLER_ON, 1, ASI_FALSE);
    ASISetControlValue(0, ASI_TARGET_TEMP,-80, ASI_FALSE); 
    //int temp = getValue(CONTROL_TEMPERATURE, &foo);
    //printf("temp = %d\n", temp); 
    //float temp1 = getSensorTemp();

    //printf("%f\n", temp1); 
   
    if (bin == 1) ASISetStartPos(0, dx, dy); 
    ASISetROIFormat(0, width, height, bin, ASI_IMG_RAW16); 
    printf("init done\n");
}

//--------------------------------------------------------------------------------------

bool Planet::GetFrame1()
{
     	frame++;
 
	bool got_it;
       	int total = 0; 
	
       	double start, end;
	long   temp;
        ASI_BOOL pb;

	ASIGetControlValue(0, ASI_TEMPERATURE, &temp, &pb);

    	printf("temp = %d\n", temp);
    	//float temp1 = getSensorTemp();

	ASISetControlValue(0, ASI_EXPOSURE, exp * 1000*1000, ASI_TRUE);


        ASIStartExposure(0, ASI_FALSE);
       
	start = nanotime();
 
        usleep(10000);

	ASI_EXPOSURE_STATUS status;


	do {
		ASIGetExpStatus(0, &status);	
	} while(status != ASI_EXP_SUCCESS);


        ASIGetDataAfterExp(0, image.ptr<uchar>(0), width * height * sizeof(PTYPE));
 	end = nanotime();
	printf("%f\n", end-start);	

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
	//g_mult = 1.0;	
	if (exp0 != exp || gain0 != gain) {
           
	    if (sim == 0) { 
       	    	ASISetControlValue(0, ASI_GAIN, gain, ASI_FALSE);
            	ASISetControlValue(0, ASI_EXPOSURE, exp * 1000000L, ASI_FALSE);
            	ASISetControlValue(0, ASI_BRIGHTNESS, 200, ASI_FALSE);
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
        createTrackbar("exp", "video", 0, 2999, 0);
        createTrackbar("mult", "video", 0, 100, 0);
        //createTrackbar("Sub", "video", 0, 32500, 0);
        createTrackbar("scale", "video", 0, 200, 0);
        
	setTrackbarPos("gain", "video", g_gain);
        setTrackbarPos("exp", "video", 1000.0 * g_exp);
        setTrackbarPos("mult", "video", 10.0 *g_mult);

        setTrackbarPos("scale", "video", 100.0 * g_scale);
	resizeWindow("video", 1, 1);
}

//--------------------------------------------------------------------------------------


int find_guide()
{
    Planet *g;
    
    g = new Planet();
   
    ui_setup();
    hack_gain_upd(g);
   
    FILE *out;

    char buf[512];
	
    sprintf(buf, "./bias_%ld.ser", time(0));
    out = fopen(buf, "wb"); 
    write_header(out, g->width, g->height, 1000);
    int cnt = 0;

    Mat resized;
 
    while(1) {
	g->GetFrame1();

	ushort *src;

	src = (ushort*)g->image.ptr<uchar>(0);
	
	fwrite(src, 1, g->width*g->height*2, out);	
	cnt++;	
        
        float scale = cvGetTrackbarPos("scale", "video") / 100.0;
	g_scale = scale;
	
	if (g->frame % 1 == 0) { 
		g->MinDev();	
        	g->image = g->image - g->bias;	
		center(g->image);	
			
		resize(g->image, resized, Size(0, 0), scale, scale, INTER_AREA);
		DrawVal(resized, "exp ", g->exp, 0, "sec");
        	DrawVal(resized, "gain", g->gain, 1, "");
        	DrawVal(resized, "frame", g->frame*1.0, 2, "");
 	
	        resized = resized * (0.1 * cvGetTrackbarPos("mult", "video"));
               	//resized = resized - (cvGetTrackbarPos("Sub", "video"));	
		
		center(resized); 
		resized/=256.0;	
		resized.convertTo(resized, CV_8U);	
		//equalizeHist(resized, resized);	
		cv::imshow("video",  resized);
        	char c = cvWaitKey(1);
        	hack_gain_upd(g);
        
		if (g->frame == 3000 || c == 27) {
            		fseek(out, 0, SEEK_SET);
			write_header(out, g->width, g->height, cnt);	
            		ASICloseCamera(0);
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
               	printf("-crop=value (0-1.0)\n"); 
		printf("-exp=value (in sec)\n");
                printf("-mult=value\n");
		printf("complex example\n");
		printf("./planet -exp=0.5 -gain=300 -mult=4 -f\n");
}

//--------------------------------------------------------------------------------------


void intHandler(int dummy=0) {
   	ASICloseCamera(0); 
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
 	g_scale = get_value("scale");	
	g_crop = get_value("crop");	
	while(pos < argc) {
		if (match(argv[pos], "-gain=")) {sscanf(strchr(argv[pos], '=') , "=%f",  &g_gain); argv[pos][0] = 0;}
        	if (match(argv[pos], "-exp="))  {sscanf(strchr(argv[pos], '=') , "=%f",  &g_exp); argv[pos][0] = 0;}
        	if (match(argv[pos], "-mult=")) {sscanf(strchr(argv[pos], '=') , "=%f",  &g_mult); argv[pos][0] = 0;}
	        if (match(argv[pos], "-crop=")) {sscanf(strchr(argv[pos], '=') , "=%f",  &g_crop); argv[pos][0] = 0;}
	
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
       set_value("scale", g_scale);
       set_value("crop", g_crop);
}
