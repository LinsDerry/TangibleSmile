/* File: ofApp.cpp
 *
 * Programmer: Lins Derry. Instructor: Hiroshi Ishii. TA: Joao Henrique Wilbert
 *
 * MIT Media Lab MAS.834/Tangible Interfaces Project 1: "Tangible Smile" (29 October 2019)
 * This program is an adaptation of the openFrameworks addon ofxFaceTracker's "example-expression" code.
 * To best compile this project, use openFrameworks v0.10.1 and xCode v10.1. In addition to including the
 * ofxFaceTracker addon, include the ofxCv, ofxDelaunay, ofxOpenCv, and ofxTiming addons. This program
 * compliments a program written in Arduino that utilizes the Pneuduino library. In essence, the main (OF)
 * program connects to the FaceTime camera and then uses the user's smile as its main input. When the
 * probability that the user is smiling reaches 0.6, a byte or char 's' is sent over the serial port. The
 * special "Pneuduino" board receives that byte and begins the inflation process of the silicone pneumatic
 * wearable that it is attached too. The main program then waits several seconds as determined by the delay
 * variable before it sends an 'i' over the serial port to trigger the deflation process. The loop then begins
 * again, measuring smiles and sending bytes. Please see https://www.youtube.com/watch?v=exZU8tjQ1zc for a
 * video of the whole project in action.
 *
 * Note to user: When running the program, enter on the keyboard 'l' to load the "expressions" data to be used
 * for measuring your smile (data is included in the ofxFaceTracker library). The program can only assess one
 * user's mouth/smile at a time. A red border will appear around your mouth if your mouth is properly detected.
 * The program is slightly "delayed" according to the delay variable initialized in setup(), so please be patient
 * when first running. The delay is necessary to control the inflating and deflating timing later.
 *
 * Sources: I refered frequently to the ofxFaceTracker on GitHub. Joao was especially helpful in figuring out which
 * OF and xCode versions to use as well as how to best implement sending bytes in tandem with running a timer.
 */

#include "ofApp.h"

     using namespace ofxCv;
     using namespace cv;

     void ofApp::setup() {

         // Check console and select the port that your FaceTime or webcam shows up on.
         cam.listDevices();
         cam.setDeviceID(1); //My FaceTime port in 1

          ofSetVerticalSync(true);
          cam.setup(640, 480);
          
          tracker.setup();
          tracker.setRescale(.5);
          
          //Print list of open ports then setup serial communication by selecting the appropriate port.
          serial.listDevices();
          vector <ofSerialDeviceInfo> deviceList = serial.getDeviceList();
          /* Open port 1 and set the baud rate to 9600. The baud rate should match the one used in
               your Arduino code. */
          serial.setup(1, 9600);
          
          /* The delay covers the entire cycle from when a smile in detected according to the probability
           threshhold to after the wearable is deflated */
          startTime = ofGetElapsedTimeMillis();
          delay = 9000;
          successTimeDelta = delay;
          cycle = 0;
     }

     void ofApp::update() {
          cam.update();
          if(cam.isFrameNew()) {
               if(tracker.update(toCv(cam))) {
                    classifier.classify(tracker);
               }
          }
          int n = classifier.size();
          int primary = classifier.getPrimaryExpression();
          char myByte;
          bool byteWasWritten;
          actualTime = ofGetElapsedTimeMillis();
          for(int i = 0; i < n; i++){
               /* Test current user expression against codified smile expression (i == 0).
                Send a byte to Pnueduino if current expression meets or exceeds the threshold probability
                for being a smile. */
               if (i == 0 && (actualTime - startTime) > successTimeDelta) {
                    float smileProbability = classifier.getProbability(i) * 100; // Threshold
                    cout <<classifier.getDescription(i) << " probability: " << smileProbability << endl;
                    if (smileProbability > 60.0) {
                         myByte = 's';
                         byteWasWritten = serial.writeByte(myByte);
                         if (byteWasWritten) {
                              startTime += delay;
                              cycle += 1;
                              cout << "s for smile was written to serial port" << endl;
                         }
                         else {
                              cout << "s for smile was NOT written to serial port" << endl;
                         }
                    }
               }
          }
          if (cycle == 1 && (actualTime - startTime) > successTimeDelta / 2) {
               myByte = 'i';
               byteWasWritten = serial.writeByte(myByte);
               if (byteWasWritten) {
                    cycle += 0;
                    cout << "i for deflate was written to serial port"<< endl;
               }
               else {
                    cout << "i for deflate was NOT written to serial port" << endl;
               }
               cycle = 0;
          }
     }

     void ofApp::draw() {
          ofSetColor(255);
          cam.draw(0, 0);
          // Draw mouth features only
          ofPolyline innerMouth = tracker.getImageFeature(ofxFaceTracker::INNER_MOUTH);
          ofPolyline outerMouth = tracker.getImageFeature(ofxFaceTracker::OUTER_MOUTH);
          outerMouth.close();
          ofSetColor(ofColor::red);
          innerMouth.draw();
          outerMouth.draw();
          
          int w = 100, h = 12;
          ofPushStyle();
          ofPushMatrix();
          ofTranslate(5, 10);

          int n = classifier.size();
          int primary = classifier.getPrimaryExpression();
          for(int i = 0; i < n; i++){
               ofSetColor(i == primary ? ofColor::red : ofColor::pink); //Color scale rep. how aligned expression is to primary
               ofDrawRectangle(0, 0, w * classifier.getProbability(i) + .5, h); // Length of bar, how probable the expression is primary
               ofSetColor(255); //Color of text
               ofDrawBitmapString(classifier.getDescription(i), 5, 9); // "smile", "eye-brows raised", "neutral"
               ofTranslate(0, h + 5);
          }
          ofPopMatrix();
          ofPopStyle();

          ofDrawBitmapString(ofToString((int) ofGetFrameRate()), ofGetWidth() - 20, ofGetHeight() - 10);
          ofDrawBitmapStringHighlight(
               string() +
               "l - load expressions",
               14, ofGetHeight() - 7 * 12);
     }

     void ofApp::keyPressed(int key) {
          if(key == 'l') {
               classifier.load("expressions");
          }
          // Wizard of Oz key to inflate Pneuduino
          if(key == 's') {
               char myByte = 's';
               bool byteWasWritten = serial.writeByte(myByte);
               if (byteWasWritten)
                    cout << "myByte for smile was written to serial port as " << myByte << endl;
               else {
                    cout << "myByte for smile was NOT written to serial port" << endl;
               }
          }
          // Wizard of Oz key to deflate Pneuduino
          if(key == 'i') {
               char myByte = 'i';
               bool byteWasWritten = serial.writeByte(myByte);
               if (byteWasWritten)
                    cout << "myByte for smile was written to serial port as " << myByte << endl;
               else {
                    cout << "myByte for smile was NOT written to serial port" << endl;
               }
          }
     }

     /* Unused measurements that may be of interest for future program adaptations
      
     // Get pixel measurements of mouth gestures.
     float mouthWidth = tracker.getGesture(ofxFaceTracker::MOUTH_WIDTH);
     float mouthHeight = tracker.getGesture(ofxFaceTracker::MOUTH_HEIGHT);

     // Get perimeter and area of smile
     float mouthArea = innerMouth.getArea();
     float mouthPerim = outerMouth.getPerimeter();

     // Write all measurements to console
     cout << "Mouth Width: " << mouthWidth << endl;
     cout << "Mouth Height: " << mouthHeight << endl;
     cout << "Mouth Area: " << mouthArea << endl;
     cout << "Mouth Perimeter: " << mouthPerim << endl;
      */
