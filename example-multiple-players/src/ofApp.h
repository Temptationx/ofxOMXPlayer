#pragma once

#include "ofMain.h"
#include "ofxVideoPlayer.h"

class ofApp : public ofBaseApp{

	public:

		void setup();
		void update();
		void draw();
		void keyPressed(int key);
	
	map<int, ofxVideoPlayer*> omxPlayers; 

};

