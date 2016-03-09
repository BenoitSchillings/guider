ver = debug
platform = mac64 

CC = g++
#INCLIB = /usr/local/include
#LDLIB = /usr/local/lib
TINY = tinyxmlerror.cpp.o tinystr.cpp.o tinyxmlparser.cpp.o tinyxml.cpp.o
OPENCV = $(shell pkg-config --cflags opencv) $(shell pkg-config --libs opencv)
#OPENCV = -I/usr/local/include/opencv -I/usr/local/include -L/usr/local/lib -lopencv_calib3d -lopencv_contrib -lopencv_core -lopencv_features2d -lopencv_flann -lopencv_gpu -lopencv_highgui -lopencv_imgproc -lopencv_legacy -lopencv_ml -lopencv_nonfree -lopencv_objdetect -lopencv_photo -lopencv_stitching -lopencv_superres -lopencv_ts -lopencv_video -lopencv_videostab 

USB =  -I../libusb/include  -L../libusb/$(platform) -lusb-1.0  



LIBSPATH = -L../lib/$(platform) -I../include



ifeq ($(ver), debug)
DEFS = -D_LIN -D_DEBUG 
CFLAGS = -g  -I $(INCLIB) -L $(LDLIB) $(DEFS) $(COMMON) $(LIBSPATH)  -lpthread  $(USB) -DGLIBC_20
else
DEFS = -D_LIN 
CFLAGS =  -O3 -I $(INCLIB) -L $(LDLIB) $(DEFS) $(COMMON) $(LIBSPATH)  -lpthread  $(USB) -DGLIBC_20
endif

ifeq ($(platform), mac32)
CC = g++
CFLAGS += -D_MAC -m32
OPENCV = -lopencv_calib3d -lopencv_contrib -lopencv_core -lopencv_features2d -lopencv_flann -lopencv_highgui -lopencv_imgproc -lopencv_legacy -lopencv_ml -lopencv_objdetect -lopencv_photo -lopencv_stitching -lopencv_ts -lopencv_video -lopencv_videostab -I/usr/local/include/opencv
endif

ifeq ($(platform), mac64)
CC = g++
CFLAGS += -D_MAC -m64 -O4
OPENCV = -lopencv_calib3d -lopencv_contrib -lopencv_core -lopencv_features2d -lopencv_flann -lopencv_highgui -lopencv_imgproc -lopencv_legacy -lopencv_ml -lopencv_objdetect -lopencv_photo -lopencv_stitching -lopencv_ts -lopencv_video -lopencv_videostab -I/usr/local/include/opencv
endif

ifeq ($(platform), x86)
CFLAGS += -m32
CFLAGS += -lrt
endif


ifeq ($(platform), x64)
CFLAGS += -m64
CFLAGS += -lrt
endif

ifeq ($(platform), armv5)
CC = arm-none-linux-gnueabi-g++
AR= arm-nonelinux-gnueabi-ar
CFLAGS += -march=armv5
CFLAGS += -lrt
endif


ifeq ($(platform), armv6)
CC = arm-bcm2708hardfp-linux-gnueabi-g++
AR= arm-bcm2708hardfp-linux-gnueabi-ar
CFLAGS += -march=armv6
CFLAGS += -lrt
endif

#ifeq ($(platform), armhf)
#CC = arm-linux-gnueabihf-g++
#AR= arm-linux-gnueabihf-ar
#CFLAGS += -march=armv5
#LDLIB += -lrt
#endif


all:guide174
guide174: guide174.cpp util.cpp tinystr.cpp.o tinyxmlparser.cpp.o tinyxml.cpp.o tinyxmlerror.cpp.o
	$(CC) -march=native -O4 guide174.cpp util.cpp $(TINY) -o guide174 $(CFLAGS)  $(OPENCV) -lASICamera

tinystr.cpp.o: ./tiny/tinystr.cpp
	$(CC)  -c ./tiny/tinystr.cpp -o tinystr.cpp.o $(CFLAGS)

tinyxml.cpp.o: ./tiny/tinyxml.cpp
	$(CC)  -c ./tiny/tinyxml.cpp -o tinyxml.cpp.o $(CFLAGS)

tinyxmlparser.cpp.o: ./tiny/tinyxmlparser.cpp
	$(CC)  -c ./tiny/tinyxmlparser.cpp -o tinyxmlparser.cpp.o $(CFLAGS)

tinyxmlerror.cpp.o: ./tiny/tinyxmlerror.cpp
	$(CC)  -c ./tiny/tinyxmlerror.cpp -o tinyxmlerror.cpp.o $(CFLAGS)

clean:
	rm -f guide174 
#pkg-config libusb-1.0 --cflags --libs
#pkg-config opencv --cflags --libs

