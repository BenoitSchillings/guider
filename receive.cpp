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


int main()
{
    char buf[1024];
 
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REP);
    socket.bind ("tcp://*:5556");

    int timeout = 20;
    socket.setsockopt (ZMQ_RCVTIMEO, &timeout, sizeof (int));
 
  
     while(1) {
       	zmq::message_t request;
        int result = socket.recv (&request);
	if (request.size() > 0) {
		printf("got msg %ld\n", request.size());	
		//memcpy(buf, request.data(), request.size());
		//buf[request.size()] = 0;
	
		//printf("%s\n", buf);
		int size;

		size = request.size();
	
                zmq::message_t msg_reply(sizeof(size));
                memcpy (msg_reply.data(), buf, sizeof(size));
                socket.send(msg_reply);

	}

     }

     return 0;
}

