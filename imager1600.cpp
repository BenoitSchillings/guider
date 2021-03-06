#include "ASICamera.h"
#include <sys/time.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <time.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "./tiny/tinyxml.h"
#include <signal.h>
#include <zmq.hpp>
#include <iostream>

bool sim = false;

#include "ser.cpp"
#include "scope.cpp"

Scope *scope;
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
#define BOX_HALF 14 // Centroid box half width/height

//--------------------------------------------------------------------------------------


float  g_gain = 150.0;
float  g_mult = 2.0;
float  g_exp = 0.1;
float  g_bin = 1;

float gain0 = -1;
float exp0 = -1;

float g_crop = 0.9;
char g_fn[256];
//--------------------------------------------------------------------------------------

void Wait(float t)
{
    usleep(t * 1000000.0);
}

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
   	int x, y;

	x = img.cols;
 	y = img.rows;
	x /= 2.0;
	y /= 2.0;
 
	rectangle(img, Point(x-10, y-10), Point(x+10, y+10), Scalar(9000, 9000, 9000), 1, 8);
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


class Guider
{
public:
    Guider();
    ~Guider();

    bool 	GetFrame();

    int 	width;
    int 	height;
    Mat 	image;
    Mat	temp_image;
    int	ref_x;
    int	ref_y;
    int	guide_box_size;
    int	frame;
    float	gain;
    float	exp;
    float	background;
    float	dev;
    float	mount_dx1;
    float	mount_dy1;
    float	mount_dx2;
    float	mount_dy2;
    float	gain_x;
    float	gain_y;
    float	gmax;
    float	gavg; 
    bool	exit_key;
private:
    void 	InitCam(int cx, int cy, int width, int height);

public:
    float 	error_to_tx(float mx, float my);
    float 	error_to_ty(float mx, float my);


public:
    void 	Centroid(float*cx, float*cy, float*total_v);
    bool 	HasGuideStar();
    bool 	FindGuideStar();
    Mat	GuideCrop();
    void	MinDev();
    void	Move(float dx, float dy);
};

//--------------------------------------------------------------------------------------
// guide solving
// http://www.wolframalpha.com/input/?i=solve+m%3D+x*u+%2B+x*y*v;+n+%3D+x*w+%2B+y*z+for+x,y
//--------------------------------------------------------------------------------------
// mx = tx * dx1 + ty * dx2;
// my = tx * dy1 + ty * dy2;
// m = xa+yb
// n = xc+yd
//dx1 is a
//dx2 is b
//dy1 is c
//dy2 is d
//x=tx
//y=ty
//m = mx
//n = my



float Guider::error_to_tx(float mx, float my)
{
    float num = (mount_dy2 * mx) - (mount_dx2*my);
    float den = (mount_dx1*mount_dy2)-(mount_dx2*mount_dy1);

    return(num/den);
}

//--------------------------------------------------------------------------------------

float Guider::error_to_ty(float mx, float my)
{
    float num = (mount_dy1*mx - mount_dx1*my);
    float den = (mount_dx2*mount_dy1 - mount_dx1*mount_dy2);

    return(num/den);
}

//--------------------------------------------------------------------------------------

void Guider::MinDev()
{
    float	count = 0;
    float	sum = 0;
    float	max = 0;
    float	avg = 0;

    background = 1e9;

    for (int y = 1; y < (height-1); y += 20) {
        for (int x = 1; x < (width-1); x += 20) {
            float v = image.at<unsigned short>(y, x);
            //float v1 = image.at<unsigned short>(y, x + 1);
	    avg += v;
	    if (v > max) max = v;
            if (v < background) background = v;
            count += 1;
            //float v2 = (v1-v);
            //sum += (v2*v2);
        }
    }
    //sum = sum / count;
    //sum /= 2.0;
    //dev = sqrt(sum);
    avg = avg / count;
    gavg = avg; 
    printf("max = %f, avg= %f\n", max, avg);
    gmax = max;
}

//--------------------------------------------------------------------------------------

//#define XX  4656
//#define YY  3520
//#define XX 768 
//#define YY 512 
//#define XX 1936
//#define YY 1216
//#define XX 1280
//#define YY 1024
#define XX 3096
#define YY 2080

Guider::Guider()
{
    width = (XX * g_crop) / g_bin;
    height = (YY * g_crop) /g_bin;
    
    width &= 0xfff0;
    height &= 0xfff0;
    frame = 0;
    background = 0;
    dev = 100;

    image = Mat(Size(width, height), CV_16UC1);
    temp_image = Mat(Size(width, height), CV_16UC1);

    guide_box_size = 14;

    ref_x = -1;
    ref_y = -1;

    if (!sim) InitCam(0.5 * XX * (1.0-g_crop), 0.5 * YY * (1.0-g_crop), width, height);


    mount_dx1 = get_value("mount_dx1");
    mount_dx2 = get_value("mount_dx2");
    mount_dy1 = get_value("mount_dy1");
    mount_dy2 = get_value("mount_dy2");
    gain_x = 1.5;
    gain_y = 1.5;
}

//--------------------------------------------------------------------------------------


void	Guider::Move(float dx, float dy)
{
    dx *= gain_x;
    dy *= gain_y;

    printf("%f %f\n", dx, dy);
    scope->Bump(dx, dy);
    return;

    if (fabs(dx) < 0.02) {
        dx = 0;
    }
    if (fabs(dy) < 0.02) {
        dy = 0;
    }

    if (dx > 0) {
        pulseGuide(guideEast, dx*1000.0);
    }

    if (dx < 0) {
        pulseGuide(guideWest, -dx*1000.0);
    }

    if (dy > 0) {
        pulseGuide(guideNorth, dy*1000.0);
    }

    if (dy < 0) {
        pulseGuide(guideSouth, -dy*1000.0);
    }
}

//--------------------------------------------------------------------------------------

bool Guider::HasGuideStar()
{
    return (ref_x > 0);
}

//--------------------------------------------------------------------------------------

bool Guider::FindGuideStar()
{
    MinDev();
    GaussianBlur(image, temp_image, Point(7, 7), 5);

    int	x, y;
    long	max = 0;

    int	cx = width / 2;
    int	cy = height / 2;

    for (y = cy - 300; y < cy + 300; y++) {
        for (x = cx - 400; x < cx + 400; x++) {
            int v = temp_image.at<unsigned short>(y, x) +
                    temp_image.at<unsigned short>(y, x + 1) +
                    temp_image.at<unsigned short>(y + 1, x) +
                    temp_image.at<unsigned short>(y + 1, x + 1);
            if (v > max) {
                max = v;
                ref_x = x;
                ref_y = y;
            }
        }
    }

    if (max < 0) {
        ref_x = -1;
        ref_y = -1;
        return false;
    }
    //printf("max %ld\n", max);
    return true;
}


//--------------------------------------------------------------------------------------


void Guider::InitCam(int cx, int cy, int width, int height)
{
    int CamNum=0;
    bool bresult;

    //cx = 1000;
    //cy = 400;

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

    setImageFormat(width, height, g_bin, IMG_RAW16);
    setValue(CONTROL_BRIGHTNESS, 250, false);
    setValue(CONTROL_GAIN, 0, false);
    printf("max %d\n", getMax(CONTROL_BANDWIDTHOVERLOAD));

    setValue(CONTROL_BANDWIDTHOVERLOAD, 35, true); //lowest transfer speed
    setValue(CONTROL_EXPOSURE, 10, false);
    setValue(CONTROL_HIGHSPEED, 1, false);
    setValue(CONTROL_COOLER_ON, 1, false);
    setValue(CONTROL_TARGETTEMP,-17, false);
    setValue(CONTROL_HARDWAREBIN, 1, false); 
    bool foo; 
    
    float temp1 = getSensorTemp();
    printf("%f\n", temp1);
    setStartPos(cx, cy);
}

//--------------------------------------------------------------------------------------

void Guider::Centroid(float*cx, float*cy, float*total_v)
{
    float bias = 0;
    float cnt;

    MinDev();
    //printf("%f %f\n", background, dev);

    cnt = 0.0;

    for (int j = 0; j < 4; j++)
        for (int i = 0; i < guide_box_size; i++) {
            bias +=  image.at<unsigned short>(ref_y + i - guide_box_size/2, ref_x - guide_box_size/2 - j);
            cnt+= 1.0;
        }
    bias /= cnt;
    bias += dev * 4;
    //printf("%f\n", bias);

    int vx, vy;
    float sum_x = 0;
    float sum_y = 0;
    float total = 0;
    int pcnt = 0;

    for (vy = ref_y - guide_box_size/2; vy <= ref_y + guide_box_size/2; vy++) {
        for (vx = ref_x - guide_box_size/2; vx <= ref_x + guide_box_size/2; vx++) {
            float v = image.at<unsigned short>(vy, vx);
            v -= bias;
            if (v > 0) {
                sum_x = sum_x + (float)vx * v;
                sum_y = sum_y + (float)vy * v;
                total = total + v;
                pcnt++;
            }
        }
    }
    sum_x = sum_x / total;
    sum_y = sum_y / total;
    *cx = sum_x;
    *cy = sum_y;
    //printf("%f %d\n", total, pcnt);
    if (pcnt < 4) total = 0;
    *total_v = total;
}


//--------------------------------------------------------------------------------------

bool Guider::GetFrame()
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

    if (exp > 1.0) {
	float dt = exp;
	while(dt > 0.2) {
		char c = cvWaitKey(1);
		if (c == 27) {
			exit_key = true;
			return 0;
		}

		usleep(200*1000);
		dt -= 0.2;
	}
    }

    got_it = getImageData(image.ptr<uchar>(0), width * height * sizeof(PTYPE), -1);

    if (!got_it) {
        printf("bad cam\n");
        //exit(-1);
    }

    return got_it;
}

//--------------------------------------------------------------------------------------

Mat Guider::GuideCrop()
{
    Mat tmp;

    tmp = Mat(image, Rect(ref_x - guide_box_size, ref_y - guide_box_size, guide_box_size*2, guide_box_size * 2));
    resize(tmp, tmp, Size(0, 0), 6, 6, INTER_NEAREST);
    return tmp;
}

//--------------------------------------------------------------------------------------

void hack_gain_upd(Guider *aguide)
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
    createTrackbar("exp", "video", 0, 60000, 0);
    createTrackbar("mult", "video", 0, 900, 0);
    createTrackbar("Sub", "video", 0, 45500, 0);

    setTrackbarPos("gain", "video", g_gain);
    setTrackbarPos("exp", "video", 1000.0 * g_exp);
    setTrackbarPos("mult", "video", 10.0 *g_mult);
    setTrackbarPos("Sub", "video", 3800);
}

//--------------------------------------------------------------------------------------



int field_center()
{
    Guider *g;

    g_crop = 1.0/6.0;

    g = new Guider();

    ui_setup();
    hack_gain_upd(g);

    startCapture();

    int cnt = 0;

    Mat resized;

    while(1) {
        g->GetFrame();
    float temp1 = getSensorTemp();
    printf("%f\n", temp1);

        cnt++;
        if (g->frame % 1 == 0) {
            g->MinDev();
            
            float k = 2;

            resize(g->image, resized, Size(0, 0), k, k, INTER_AREA);

            center(resized);

	    center(resized);
            DrawVal(resized, "exp ", g->exp, 0, "sec");
            DrawVal(resized, "gain", g->gain, 1, "");
            DrawVal(resized, "frame", g->frame*1.0, 2, "");

	    resized = resized - (cvGetTrackbarPos("Sub", "video"));
            resized = resized * (0.1 * cvGetTrackbarPos("mult", "video"));
 
	    cv::imshow("video", resized);
            char c = cvWaitKey(1);

            hack_gain_upd(g);

            if (c == 27 || g->exit_key) {
                stopCapture();
                closeCamera();
                return 0;
            }
        }
    }
}





int find_field()
{
    Guider *g;
    
    g = new Guider();

    ui_setup();
    hack_gain_upd(g);

    startCapture();

    int cnt = 0;

    while(1) {
        g->GetFrame();

        cnt++;
        if (g->frame % 1 == 0) {
            g->MinDev();
            
            g->image = g->image - (cvGetTrackbarPos("Sub", "video"));
            g->image = g->image * (0.1 * cvGetTrackbarPos("mult", "video"));

            center(g->image);
            
            DrawVal(g->image, "exp ", g->exp, 0, "sec");
            DrawVal(g->image, "gain", g->gain, 1, "");
            DrawVal(g->image, "frame", g->frame*1.0, 2, "");
	
	    if (cnt % 5 == 0) {
		    float temp1 = getSensorTemp();
		    printf("temp=%f\n", temp1);
	    }

	    cv::imshow("video", g->image);
            char c = cvWaitKey(1);
            hack_gain_upd(g);

	    if (c =='a') {
	   	setTrackbarPos("Sub", "video", g->gavg - 1000); 
	    }

            if (c == 27) {
                stopCapture();
                closeCamera();
                return 0;
            }
        }
    }
}




int take()
{
    Guider *g;
    char   buf[512];
   
    g_crop = 1/1.0;
 
    g = new Guider();

    ui_setup();
    hack_gain_upd(g);

    startCapture();
    g->exit_key = false;
    int cnt = 0;

    sprintf(buf, "%s_%ld.ser", g_fn, time(0));
    FILE *out = fopen(buf, "wb");
    write_header(out, g->width, g->height, 1000);

    Mat resized;

    int save = 0;
 
    while(1) {
        printf("frame %d, %d\n", g->frame, save);
        g->GetFrame();
        cnt++;
            
            ushort *src;

            src = (ushort*)g->image.ptr<uchar>(0);

	    char yes = 0;

	    g->MinDev();
	    //if (g->gavg > 8000 && g->gavg < 13000) {
            	fwrite(src, 1, g->width*g->height*2, out);
	   	save++; 
	   	//yes = 1; 
	    //}

            if (g->frame %25 == 0) {

            float temp1 = getSensorTemp();
    	    printf("%f\n", temp1);
 
	    float k = 0.5;
 
            resize(g->image, resized, Size(0, 0), k, k, INTER_AREA);

	    center(resized);
            
            resized = resized - (cvGetTrackbarPos("Sub", "video"));
            resized = resized * (0.1 * cvGetTrackbarPos("mult", "video"));

            DrawVal(resized, "exp ", g->exp, 0, "sec");
            DrawVal(resized, "gain", g->gain, 1, "");
            DrawVal(resized, "frame", g->frame*1.0, 2, "");

 
	    cv::imshow("video", resized);
            char c = cvWaitKey(1);
	    if (c != 27) {
	            c = cvWaitKey(1);
	    }

            hack_gain_upd(g);

	    resized.release();
            if (g->frame == 26000 || c == 27 || g->exit_key) {
                stopCapture();
                closeCamera();
                write_header(out, g->width, g->height, g->frame);

 
		fclose(out);
                return 0;
            }
        }
    }
}
//--------------------------------------------------------------------------------------


int graphic_test()
{
    Guider *g = new Guider();

    ui_setup();
    hack_gain_upd(g);

    startCapture();

    Mat zoom;


    Mat uibm = Mat(Size(1200, 800), CV_16UC1);


    while(1) {
        rectangle(uibm,
                  Point( 0, 0),
                  Point(300, 300),
                  Scalar(0, 0, 0),
                  -1,
                  8);

        blit(0.3 * g->image, uibm, 0, 0, 2300, 2300, 50, 50);

        DrawVal(uibm, "exp ", g->exp, 0, "sec");
        DrawVal(uibm, "gain", g->gain, 1, "");

        cv::imshow("video", uibm);

        char c = cvWaitKey(1);
        hack_gain_upd(g);
    }
}

//--------------------------------------------------------------------------------------

int guide()
{
    float   sum_x;
    float   sum_y;
    int     frame_per_correction = 3;
    int     frame_count;
    int	drizzle_dx = 0;
    int	drizzle_dy = 0;
    int	err = 0;
    FILE	*out;

    Guider *g = new Guider();

    char buf[512];

    sprintf(buf, "./data_%ld.ser", time(0));
    out = fopen(buf, "wb");
    write_header(out, g->width, g->height, 1000);

    ui_setup();
    hack_gain_upd(g);
    Mat uibm = Mat(Size(1200, 800), CV_16UC1);

    startCapture();

    Mat zoom;


    frame_count = 0;
    sum_x = 0;
    sum_y = 0;

    int logger = 0;

    while(1) {
restart:

        g->GetFrame();
        if (!g->HasGuideStar()) {
            blit(g->image   * (0.1 * cvGetTrackbarPos("mult", "video")), uibm, 0, 0, 2300, 2300, 0, 0);
            if (g->FindGuideStar()) {
                uibm = Mat(Size(400, 400), CV_16UC1, cv::Scalar(0));
            }
        }

        ushort *src;

        src = (ushort*)g->image.ptr<uchar>(0);


        //fwrite(src, 1, g->width*g->height*2, out);


        hack_gain_upd(g);


        if (g->HasGuideStar()) {
            float cx;
            float cy;
            float total_v;
            g->Centroid(&cx, &cy, &total_v);
            if (total_v > 0) {
                float dx = cx-g->ref_x + drizzle_dx;
                float dy = cy-g->ref_y + drizzle_dy;
                sum_x += dx;
                sum_y += dy;
                frame_count++;

                if (frame_count == frame_per_correction) {
                    sum_x = sum_x / frame_per_correction;
                    sum_y = sum_y / frame_per_correction;
                    printf("sum %f %f\n", sum_x, sum_y);
                    float tx = g->error_to_tx(sum_x, sum_y);
                    float ty = g->error_to_ty(sum_x, sum_y);
                    sum_x = 0;
                    sum_y = 0;
                    frame_count = 0;
                    //if (scope->XCommand("xneed_guiding") != 0) {
                        g->Move(-tx * 2.0, -ty * 2.0);
                    //}
                }
            }

            if (total_v < 15000) {
                err++;
                if (err > 30) {
                    err = 0;
                    g->ref_x = 0;
                    g->ref_y = 0;
                    goto restart;
                }
            } else {
                err = 0;
            }

            //if (scope->XCommand("xdither") == 1) {
                //drizzle_dx = rand()%4 - 2;
                //drizzle_dy = rand()%4 - 2;
            //}


            if (logger % 50 == 0) {
                scope->Log();
                float temp1 = getSensorTemp();
                printf("temp %f\n", temp1);

            }
            logger++;

            float mult = 0.1 * cvGetTrackbarPos("mult", "video");
            blit(mult * g->GuideCrop(), uibm, 0, 0, 1000, 1000, 150, 150);
            DrawVal(uibm, "tot ", total_v, 2, "adu");
            DrawVal(uibm, "cx ", cx, 3, "");
            DrawVal(uibm, "cy ", cy, 4, "");
        }

        DrawVal(uibm, "exp ", g->exp, 0, "sec");
        DrawVal(uibm, "gain", g->gain, 1, "");


        cv::imshow("video", uibm);

        char c = cvWaitKey(1);
        if (c == 27) {
            fseek(out, 0, SEEK_SET);
            write_header(out, g->width, g->height, frame_count);

            stopCapture();
            closeCamera();
            return 0;
        }
    }
}

//--------------------------------------------------------------------------------------


int calibrate()
{
    Guider *g = new Guider();
    float  x1, y1;
    float  x2, y2;
    float  x3, y3;

    ui_setup();
    hack_gain_upd(g);
    Mat uibm = Mat(Size(1200, 800), CV_16UC1);
    startCapture();
//-----------------------------
//   x1,y1 ------->x(5)-->x2,y2
//                         |
//                         |
//                         |
//                         |
//                        y(5)
//                         |
//                         |
//                        x3,y3
//     then move back x(-5), y(-5)


    {
        for (int i = 0; i < 8; i++) g->GetFrame();
        printf("x4\n");
        g->FindGuideStar();
        printf("x5\n");
        //scope->Log();
        x1 = g->ref_x;
       	printf("x5551\n"); 
 	y1 = g->ref_y;
       	printf("x5552]n"); 
 	printf("v1 %f %f\n", x1, y1);

        for (int i =0; i < 2; i++) g->Move(0.9, 0);
        scope->Log();
        cv::imshow("video", g->image);
	printf("x6\n");
        char c = cvWaitKey(1);
        for (int i =0; i < 8; i++)
            g->GetFrame();

	printf("x7\n");
        g->FindGuideStar();
        x2 = g->ref_x;
        y2 = g->ref_y;
        printf("v2 %f %f\n", x2, y2);

        for (int i =0; i < 2; i++) g->Move(0.0, 0.9);
        scope->Log();
        cv::imshow("video", g->image);
        c = cvWaitKey(1);

        for (int i = 0; i < 8; i++) g->GetFrame();
        g->FindGuideStar();
        x3 = g->ref_x;
        y3 = g->ref_y;
        printf("v3 %f %f\n", x3, y3);

        cv::imshow("video", g->image);
        c = cvWaitKey(1);

        for (int i =0; i < 2; i++) g->Move(-0.9, -0.9);
        scope->Log();
        stopCapture();
        closeCamera();
    }
    if (x1 < 0 || x2 < 0 || x3 < 0) {
        printf("no reference star\n");
        exit(-1);
    }

    g->mount_dx1 = (x2-x1);
    g->mount_dy1 = (y2-y1);
    g->mount_dx2 = (x3-x2);
    g->mount_dy2 = (y3-y2);

    set_value("mount_dx1", g->mount_dx1);
    set_value("mount_dx2", g->mount_dx2);
    set_value("mount_dy1", g->mount_dy1);
    set_value("mount_dy2", g->mount_dy2);

    return 0;
}

//--------------------------------------------------------------------------------------

void test_guide()
{
    Guider *g;

    g = new Guider();


    g->mount_dx1 = 10.0;
    g->mount_dy1 = 0.001;
    g->mount_dx2 = 0.001;
    g->mount_dy2 = 10.0;


    printf("error is -10 pixel in x  %f %f\n", g->error_to_tx(-10, 0), g->error_to_ty(-10, 0));
    printf("error is 10 pixel in y  %f %f\n", g->error_to_tx(0, 10), g->error_to_ty(0, 10));
    printf("error is 10 pixel in x&y  %f %f\n", g->error_to_tx(10, 10), g->error_to_ty(10, 10));
    printf("error is 10 pixel in x and -10 y  %f %f\n", g->error_to_tx(10, -10), g->error_to_ty(10, -10));

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
    printf("%s -center   1x1 bin center 800 pixels\n", argv[0]);
   // printf("%s -g        acquire guide star and guide\n", argv[0]);
   // printf("%s -cal      calibrate mount\n", argv[0]);
    //printf("%s -t        test guide logic\n", argv[0]);
    printf("exta args\n");
    printf("-gain=value\n");
    printf("-exp=value (in sec)\n");
    printf("-mult=value\n");
    printf("-bin=value\n");
    printf("complex example\n");
    printf("./guider -exp=0.5 -gain=300 -mult=4 -cal\n");
}

//--------------------------------------------------------------------------------------


void intHandler(int dummy=0)
{
    closeCamera();
    printf("emergency close\n");
    exit(0);
}

//--------------------------------------------------------------------------------------


int main(int argc, char **argv)
{
    signal(SIGINT, intHandler);

    //scope = new Scope();
    //scope->Init();
    //scope->LongFormat();
    //scope->Siderial();
    //scope->SetRate(0.5 * -0.00553, 0*-0.1*-0.0024);
    //scope->XCommand("xfocus-5");
    //scope->XCommand("xfocus7");

    int i = 0;

    //while(1) {
    	//float ra = scope->RA();
    	//float dec = scope->Dec();
    	//printf("RA is %f\n", ra);
    	//printf("DEC is %f\n", dec);
    //}

    //ra -= 0.01;
    //dec = 20.0;

    if (argc == 1 || strcmp(argv[1], "-h") == 0) {
        help(argv);
        return 0;
    }

    int pos = 1;

    g_exp = get_value("exp");
    g_gain = get_value("gain");
    g_mult = get_value("mult");
    g_bin = 2;
    
    while(pos < argc) {
        if (match(argv[pos], "-gain=")) {
            sscanf(strchr(argv[pos], '=') , "=%f",  &g_gain);
            argv[pos][0] = 0;
        }
        if (match(argv[pos], "-exp="))  {
            sscanf(strchr(argv[pos], '=') , "=%f",  &g_exp);
            argv[pos][0] = 0;
        }
        if (match(argv[pos], "-mult=")) {
            sscanf(strchr(argv[pos], '=') , "=%f",  &g_mult);
            argv[pos][0] = 0;
        }
        if (match(argv[pos], "-bin=")) {
            sscanf(strchr(argv[pos], '=') , "=%f",  &g_bin);
            argv[pos][0] = 0;
        }
        
       if (match(argv[pos], "-o="))  {
	    sscanf(strchr(argv[pos], '=') , "=%s",  (char*)g_fn);
            argv[pos][0] = 0;
       }
 
	pos++;
    }
    pos = 1;
    set_value("exp", g_exp);
    set_value("gain", g_gain);
    set_value("mult", g_mult);

    while(pos < argc) {
        if (match(argv[pos], "-t")) test_guide();
        if (match(argv[pos], "-f")) find_field();
        if (match(argv[pos], "-center")) field_center();
        if (match(argv[pos], "-guide")) guide();
        if (match(argv[pos], "-take")) take(); 
        if (match(argv[pos], "-cal")) calibrate();
        pos++;
    }

    set_value("exp", g_exp);
    set_value("gain", g_gain);
    set_value("mult", g_mult);
}
