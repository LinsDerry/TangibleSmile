#pragma once

#include "ofMain.h"
#include "ofxFaceTracker.h"
#include "ofSerial.h"
#include "ofTimer.h"

class ofApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	void keyPressed(int key);
	
	ofVideoGrabber cam;
	ofxFaceTracker tracker;
	ExpressionClassifier classifier;
     ofSerial serial;
     
     unsigned long actualTime, startTime;
     unsigned int successTimeDelta;
     int delay;
     int cycle;
};
