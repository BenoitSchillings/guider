ver = release 
platform = x64 

CC = g++
TINY = ./tinyobj/tinyxmlerror.cpp.o ./tinyobj/tinystr.cpp.o ./tinyobj/tinyxmlparser.cpp.o ./tinyobj/tinyxml.cpp.o
OPENCV0 = -I/usr/local/include/opencv -I/usr/local/include -L/usr/local/lib -lopencv_calib3d -lopencv_core -lopencv_features2d -lopencv_flann -lopencv_highgui -lopencv_imgproc -lopencv_ml -lopencv_objdetect -lopencv_photo -lopencv_stitching -lopencv_superres -lopencv_ts -lopencv_video -lopencv_videostab -L/usr/local/lib -lzmq 
OPENCV = -I/usr/local/include/opencv  -lopencv_calib3d -lopencv_core -lopencv_features2d -lopencv_flann -lopencv_highgui -lopencv_imgproc -lopencv_ml -lopencv_objdetect -lopencv_photo -lopencv_stitching -lopencv_superres  -lopencv_video -lopencv_videostab  -lzmq 

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


all:guide174 guide174ao imager1600 monitor  scope_server vbox_to_ap focus scopestop joystick receive fake_serial

guide174:  AACoordinateTransformation.cpp  ser.cpp scope.cpp guide174.cpp util.cpp ./tinyobj/tinystr.cpp.o ./tinyobj/tinyxmlparser.cpp.o ./tinyobj/tinyxml.cpp.o ./tinyobj/tinyxmlerror.cpp.o
	$(CC) -march=native -O3 guide174.cpp  AACoordinateTransformation.cpp util.cpp $(TINY) -o guide174 $(CFLAGS)  $(OPENCV) -lASICamera

	
guide174ao:  AACoordinateTransformation.cpp  ser.cpp scope.cpp guide174.cpp util.cpp ./tinyobj/tinystr.cpp.o ./tinyobj/tinyxmlparser.cpp.o ./tinyobj/tinyxml.cpp.o ./tinyobj/tinyxmlerror.cpp.o
	$(CC) -march=native -O3 guide174ao.cpp  AACoordinateTransformation.cpp util.cpp ~/skyx_tcp/skyx.cpp ~/skyx_tcp/sky_ip.cpp $(TINY) -o guide174ao $(CFLAGS)  $(OPENCV) -lASICamera


	
	
imager1600:  AACoordinateTransformation.cpp  ser.cpp scope.cpp imager1600.cpp util.cpp ./tinyobj/tinystr.cpp.o ./tinyobj/tinyxmlparser.cpp.o ./tinyobj/tinyxml.cpp.o ./tinyobj/tinyxmlerror.cpp.o
	$(CC) -march=native -O3 imager1600.cpp  AACoordinateTransformation.cpp util.cpp $(TINY) -o imager1600 $(CFLAGS)  $(OPENCV) -lASICamera

scopestop:  AACoordinateTransformation.cpp  scope.cpp scopestop.cpp util.cpp ./tinyobj/tinystr.cpp.o ./tinyobj/tinyxmlparser.cpp.o ./tinyobj/tinyxml.cpp.o ./tinyobj/tinyxmlerror.cpp.o
	$(CC) -march=native -O3 scopestop.cpp  AACoordinateTransformation.cpp util.cpp $(TINY) -o scopestop $(CFLAGS)  $(OPENCV)

focus:  AACoordinateTransformation.cpp  scope.cpp focus.cpp util.cpp ./tinyobj/tinystr.cpp.o ./tinyobj/tinyxmlparser.cpp.o ./tinyobj/tinyxml.cpp.o ./tinyobj/tinyxmlerror.cpp.o
	$(CC) -march=native -O3 focus.cpp  AACoordinateTransformation.cpp util.cpp $(TINY) -o focus $(CFLAGS)  $(OPENCV)


joystick:  AACoordinateTransformation.cpp  scope.cpp joystick.cpp util.cpp ./tinyobj/tinystr.cpp.o ./tinyobj/tinyxmlparser.cpp.o ./tinyobj/tinyxml.cpp.o ./tinyobj/tinyxmlerror.cpp.o
	$(CC) -march=native -O3 joystick.cpp  AACoordinateTransformation.cpp util.cpp $(TINY) -o joystick $(CFLAGS)  $(OPENCV)


vbox_to_ap:  AACoordinateTransformation.cpp  scope.cpp vbox_to_ap.cpp util.cpp ./tinyobj/tinystr.cpp.o ./tinyobj/tinyxmlparser.cpp.o ./tinyobj/tinyxml.cpp.o ./tinyobj/tinyxmlerror.cpp.o
	$(CC) -march=native -O3 vbox_to_ap.cpp  AACoordinateTransformation.cpp util.cpp $(TINY) -o vbox_to_ap $(CFLAGS)  $(OPENCV)

fake_serial:  AACoordinateTransformation.cpp  scope.cpp fake_serial.cpp util.cpp ./tinyobj/tinystr.cpp.o ./tinyobj/tinyxmlparser.cpp.o ./tinyobj/tinyxml.cpp.o ./tinyobj/tinyxmlerror.cpp.o
	$(CC) -march=native -O3 fake_serial.cpp  AACoordinateTransformation.cpp util.cpp $(TINY) -o fake_serial $(CFLAGS)  $(OPENCV)

scope_server:  scope_server.cpp util.cpp ./tinyobj/tinystr.cpp.o ./tinyobj/tinyxmlparser.cpp.o ./tinyobj/tinyxml.cpp.o ./tinyobj/tinyxmlerror.cpp.o
	$(CC) -march=native -O3 -lzmq scope_server.cpp util.cpp $(TINY) -lzmq -o scope_server $(CFLAGS)

receive:  receive.cpp util.cpp ./tinyobj/tinystr.cpp.o ./tinyobj/tinyxmlparser.cpp.o ./tinyobj/tinyxml.cpp.o ./tinyobj/tinyxmlerror.cpp.o
	$(CC) -march=native -O3 -lzmq receive.cpp util.cpp $(TINY) -lzmq -o receive $(CFLAGS)


planet: ser.cpp   AACoordinateTransformation.cpp  scope.cpp planet.cpp util.cpp ./tinyobj/tinystr.cpp.o ./tinyobj/tinyxmlparser.cpp.o ./tinyobj/tinyxml.cpp.o ./tinyobj/tinyxmlerror.cpp.o
	$(CC) -march=native -O3 planet.cpp   AACoordinateTransformation.cpp   util.cpp $(TINY) -o planet $(CFLAGS)  $(OPENCV) -lASICamera2

monitor: ser.cpp   AACoordinateTransformation.cpp  scope.cpp monitor.cpp util.cpp ./tinyobj/tinystr.cpp.o ./tinyobj/tinyxmlparser.cpp.o ./tinyobj/tinyxml.cpp.o ./tinyobj/tinyxmlerror.cpp.o
	$(CC) -march=native -O3 monitor.cpp   AACoordinateTransformation.cpp   util.cpp $(TINY) -o monitor $(CFLAGS)  $(OPENCV) -lASICamera

./tinyobj/tinystr.cpp.o: ./tiny/tinystr.cpp
	$(CC)  -c ./tiny/tinystr.cpp -o ./tinyobj/tinystr.cpp.o $(CFLAGS)

./tinyobj/tinyxml.cpp.o: ./tiny/tinyxml.cpp
	$(CC)  -c ./tiny/tinyxml.cpp -o ./tinyobj/tinyxml.cpp.o $(CFLAGS)

./tinyobj/tinyxmlparser.cpp.o: ./tiny/tinyxmlparser.cpp
	$(CC)  -c ./tiny/tinyxmlparser.cpp -o ./tinyobj/tinyxmlparser.cpp.o $(CFLAGS)

./tinyobj/tinyxmlerror.cpp.o: ./tiny/tinyxmlerror.cpp
	$(CC)  -c ./tiny/tinyxmlerror.cpp -o ./tinyobj/tinyxmlerror.cpp.o $(CFLAGS)

clean:
	rm -f vbox_to_ap guide174 planet scope_server fake_serial joystick imager1600
#pkg-config libusb-1.0 --cflags --libs
#pkg-config opencv --cflags --libs

