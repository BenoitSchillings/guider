ver = release 
platform = x64 

CC = g++
#INCLIB = /usr/local/include
#LDLIB = /usr/local/lib
TINY = ./tinyobj/tinyxmlerror.cpp.o ./tinyobj/tinystr.cpp.o ./tinyobj/tinyxmlparser.cpp.o ./tinyobj/tinyxml.cpp.o
#OPENCV = $(shell pkg-config --cflags opencv) $(shell pkg-config --libs opencv)
OPENCV0 = -I/usr/local/include/opencv -I/usr/local/include -L/usr/local/lib -lopencv_calib3d -lopencv_core -lopencv_features2d -lopencv_flann -lopencv_highgui -lopencv_imgproc -lopencv_ml -lopencv_objdetect -lopencv_photo -lopencv_stitching -lopencv_superres -lopencv_ts -lopencv_video -lopencv_videostab -L/usr/local/lib -lzmq 
OPENCV = -I/usr/local/include/opencv  -lopencv_calib3d -lopencv_core -lopencv_features2d -lopencv_flann -lopencv_highgui -lopencv_imgproc -lopencv_ml -lopencv_objdetect -lopencv_photo -lopencv_stitching -lopencv_superres -lopencv_ts -lopencv_video -lopencv_videostab  -lzmq 

USB =  -I./libusb/include  -L./libusb/$(platform) -lusb-1.0  



LIBSPATH = -L./lib/$(platform) -I./include



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
CFLAGS += -D_MAC -m64 -O3
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


all:guide174 planet ap_server

guide174:  AACoordinateTransformation.cpp  ser.cpp ap.cpp guide174.cpp util.cpp ./tinyobj/tinystr.cpp.o ./tinyobj/tinyxmlparser.cpp.o ./tinyobj/tinyxml.cpp.o ./tinyobj/tinyxmlerror.cpp.o
	$(CC) -march=native -O3 guide174.cpp  AACoordinateTransformation.cpp util.cpp $(TINY) -o guide174 $(CFLAGS)  $(OPENCV) -lASICamera

ap_server:  ap_server.cpp util.cpp ./tinyobj/tinystr.cpp.o ./tinyobj/tinyxmlparser.cpp.o ./tinyobj/tinyxml.cpp.o ./tinyobj/tinyxmlerror.cpp.o
	$(CC) -march=native -O3 -lzmq ap_server.cpp util.cpp $(TINY) -lzmq -o ap_server $(CFLAGS)

planet: ser.cpp   AACoordinateTransformation.cpp  ap.cpp planet.cpp util.cpp ./tinyobj/tinystr.cpp.o ./tinyobj/tinyxmlparser.cpp.o ./tinyobj/tinyxml.cpp.o ./tinyobj/tinyxmlerror.cpp.o
	$(CC) -march=native -O3 planet.cpp   AACoordinateTransformation.cpp   util.cpp $(TINY) -o planet $(CFLAGS)  $(OPENCV) -lASICamera

./tinyobj/tinystr.cpp.o: ./tiny/tinystr.cpp
	$(CC)  -c ./tiny/tinystr.cpp -o ./tinyobj/tinystr.cpp.o $(CFLAGS)

./tinyobj/tinyxml.cpp.o: ./tiny/tinyxml.cpp
	$(CC)  -c ./tiny/tinyxml.cpp -o ./tinyobj/tinyxml.cpp.o $(CFLAGS)

./tinyobj/tinyxmlparser.cpp.o: ./tiny/tinyxmlparser.cpp
	$(CC)  -c ./tiny/tinyxmlparser.cpp -o ./tinyobj/tinyxmlparser.cpp.o $(CFLAGS)

./tinyobj/tinyxmlerror.cpp.o: ./tiny/tinyxmlerror.cpp
	$(CC)  -c ./tiny/tinyxmlerror.cpp -o ./tinyobj/tinyxmlerror.cpp.o $(CFLAGS)

clean:
	rm -f guide174 planet ap_server
#pkg-config libusb-1.0 --cflags --libs
#pkg-config opencv --cflags --libs

