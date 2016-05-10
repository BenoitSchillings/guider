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

#include "scope.cpp"

Scope *scope;


int main(int argc, char **argv)
{
    	scope = new Scope();
	scope->Init();
	scope->Stop();	
	scope->Done();
}
	
