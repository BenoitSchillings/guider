#include "stdio.h"
#include "ASICamera.h"
#include <sys/time.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <time.h>
#include <opencv2/core/core.hpp> 
#include <opencv2/highgui/highgui.hpp>

//--------------------------------------------------------------------------------------

using namespace cv;
using namespace std;

//--------------------------------------------------------------------------------------

#define ushort unsigned short
#define uchar unsigned char
#define PTYPE unsigned short
#define BOX_HALF 7 // Centroid box half width/height
//--------------------------------------------------------------------------------------

float gain0 = -1;
float exp0 = -1;

//--------------------------------------------------------------------------------------

void blit(Mat from, Mat to, int x1, int y1, int w, int h, int dest_x, int dest_y)
{
	ushort *source = (ushort*)(from.data);
	ushort *dest = (ushort*)(to.data);
	
	int	rowbyte_source = from.step/2;
	int	rowbyte_dest = to.step/2;

	if ((dest_x + w) > to.cols) {
		w = to.cols - dest_x;	
	}
	if ((dest_y + h) > to.rows) {
		h = to.rows - dest_y;
	}

	if ((x1 + w) > from.cols) {
		w = from.cols - x1;	
	}
	
	if ((y1 + h) > from.rows) {
		h = from.rows - y1;	
	}

	for (int y = y1; y < y1 + w; y++) {
		int	dest_yc = dest_y + y;
	
		int off_y_dst = rowbyte_dest * dest_yc + dest_x + x1;
		int src_y_dst = rowbyte_source * y + x1;
		
		memcpy(dest + off_y_dst, source + src_y_dst, w*2);	
	}
}

//--------------------------------------------------------------------------------------


class Guider {
public:	
		Guider();
		~Guider();

	void 	GetFrame();

    	int 	width;
    	int 	height;
	Mat 	image;
	Mat	temp_image;	
	int	ref_x;
	int	ref_y;
	int	guide_box_size;
private:
	void 	InitCam(int cx, int cy, int width, int height);
	
public:
	void 	Centroid(float*cx, float*cy, float*total_v);
	bool 	HasGuideStar();
	void	InitGuideStar();
	Mat	GuideCrop();
};



//--------------------------------------------------------------------------------------

void cvText(Mat img, const char* text, int x, int y)
{
    putText(img, text, Point2f(x, y), FONT_HERSHEY_PLAIN, 1, CV_RGB(62000, 62000, 62000), 1, 8);
}

//--------------------------------------------------------------------------------------

void DrawVal(Mat img, const char* title, float value, int x, int y, const char *units)
{
	char	buf[512];

	sprintf(buf, "%s=%f %s", title, value, units);
	cvText(img, buf, x, y);
}


//--------------------------------------------------------------------------------------

	Guider::Guider()
{
	width = 1000;
	height = 1000;

   	image = Mat(Size(width, height), CV_16UC1);
	temp_image = Mat(Size(width, height), CV_16UC1);

	char buf[512];

	for (int y = 0; y < height; y+=30) {
		for (int x = 0; x < width; x+=100) {
			sprintf(buf, "%d-%d", x, y);	
			cvText(image, buf, x, y); 	
		}
	}

	guide_box_size = 14;

	ref_x = -1;
	ref_y = -1;
	
	InitCam(0, 0, width, height);
}


//--------------------------------------------------------------------------------------

bool Guider::HasGuideStar()
{
	return (ref_x > 0);
}

//--------------------------------------------------------------------------------------

void Guider::InitGuideStar()
{
	GaussianBlur(image, temp_image, Point(7, 7), 5);	

	int	x, y;
	int	max = 50000;


	printf("start\n");
	for (y = 2*guide_box_size; y < height - 2*guide_box_size; y++) {
		for (x = 2*guide_box_size; x < width - 2*guide_box_size; x++) {
			int v = image.at<unsigned short>(y, x);
 			if (v > max) {
				max = v;
				ref_x = x;
				ref_y = y;
			}	
		}
	}
	printf("end\n");
}


//--------------------------------------------------------------------------------------


void Guider::InitCam(int cx, int cy, int width, int height)
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
    

  
    setImageFormat(width, height, 1, IMG_RAW16);
    setValue(CONTROL_BRIGHTNESS, 100, false);
    setValue(CONTROL_GAIN, 0, false);
    setValue(CONTROL_BANDWIDTHOVERLOAD, 0, true); //lowest transfer speed
    setValue(CONTROL_EXPOSURE, 50, false);
    setValue(CONTROL_HIGHSPEED, 1, false);
    setStartPos(cx - width/2, cy-height/2);
    printf("got it\n");
}

//--------------------------------------------------------------------------------------

void Guider::Centroid(float*cx, float*cy, float*total_v)
{
   float bias = 0;
   float cnt;


   cnt = 0.0;

   for (int j = 0; j < 4; j++) 
   for (int i = 0; i < guide_box_size; i++) { 
   	bias +=  image.at<unsigned short>(ref_y + i - guide_box_size/2, ref_x - guide_box_size/2 - j);
  	cnt+= 1.0; 
   } 
   bias /= cnt;
   
    int vx, vy;
    float sum_x = 0;
    float sum_y = 0;
    float total = 0;

    for (vy = ref_y - guide_box_size/2; vy <= ref_y + guide_box_size/2; vy++) {
        for (vx = ref_x - guide_box_size/2; vx <= ref_x + guide_box_size/2; vx++) {
            float v = image.at<unsigned short>(vy, vx);
	    v -= bias; 
	    if (v > 0) {
                sum_x = sum_x + (float)vx * v; 
                sum_y = sum_y + (float)vy * v;
                total = total + v;
            }
        }
    }
    sum_x = sum_x / total;
    sum_y = sum_y / total;
    *cx = sum_x;
    *cy = sum_y;
    *total_v = total;
}


//--------------------------------------------------------------------------------------

void Guider::GetFrame()
{
        bool got_it;
        do {
            got_it = getImageData(image.ptr<uchar>(0), width * height * sizeof(PTYPE), 20);
        } while(!got_it);
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

void hack_gain_upd()
{
        float gain = cvGetTrackbarPos("gain", "video");
        float exp = cvGetTrackbarPos("exp", "video");
        exp = exp / 128.0;
        exp = exp * exp;
        
	if (exp0 != exp || gain0 != gain) {
            setValue(CONTROL_GAIN, gain, false);
            setValue(CONTROL_EXPOSURE, exp*1000000, false);
            setValue(CONTROL_BRIGHTNESS, 200, false);
            gain0 = gain;
            exp0 = exp;
        }
}

//--------------------------------------------------------------------------------------

void ui_setup()
{
        namedWindow("video", 1);
        createTrackbar("gain", "video", 0, 600, 0);
        createTrackbar("exp", "video", 0, 256, 0);
        createTrackbar("mult", "video", 0, 100, 0);
        createTrackbar("Sub", "video", 0, 500, 0);
        setTrackbarPos("gain", "video", 130);
        setTrackbarPos("exp", "video", 8);
        setTrackbarPos("mult", "video", 10.0);
}

//--------------------------------------------------------------------------------------

int main()
{
	Guider *g;

	ui_setup();

	g = new Guider();

	startCapture();

	Mat zoom;
	

        Mat uibm = Mat(Size(1200, 800), CV_16UC1);


	while(1) {
		rectangle(uibm,
           		  Point( 0, 0),
           		  Point(300,300),
           		  Scalar( 3000, 1130, 1130),
           		  -1,
           		  8);

		blit(g->image, uibm, 0, 0, 300, 300, 50, 50);
	
		//g->image(Rect(0, 0, 200, 200)).copyTo(uibm);	
		DrawVal(uibm, "exp", exp0, 20, 20, "msec");
		

		cv::imshow("video", uibm);	
		//cv::imshow("video", g->image * (0.1 + cvGetTrackbarPos("mult", "video")));	
		char c = cvWaitKey(1);	
		hack_gain_upd();	
	}

	
	while(1) {
		g->GetFrame();
		if (!g->HasGuideStar()) {
			g->InitGuideStar();
		}

		hack_gain_upd();

                DrawVal(g->image, "exp", exp0, 20, 20, "msec");
	
 		cv::imshow("video", g->image  * (0.1 + cvGetTrackbarPos("mult", "video")));
          	char c = cvWaitKey(1);
		hack_gain_upd();

		if (g->HasGuideStar()) {
			float cx;
			float cy;
			float total_v;
	
			g->Centroid(&cx, &cy, &total_v);
			if (total_v > 0) {
			}
			float mult = 0.1 + cvGetTrackbarPos("mult", "video");	
			cv::imshow("guide", mult * g->GuideCrop());	
		}


		if (c == 27) {
			stopCapture();
			return 0;
		}
	}
}

