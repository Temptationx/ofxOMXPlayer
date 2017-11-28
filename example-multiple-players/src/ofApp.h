#pragma once

#include "ofMain.h"
#include "ofxOMXPlayer.h"

class ofApp : public ofBaseApp, public ofxOMXPlayerListener{

	public:

		void setup();
		void update();
		void draw();
		void keyPressed(int key);
	    virtual void onVideoEnd(ofxOMXPlayerListenerEventData& e);
        virtual void onVideoLoop(ofxOMXPlayerListenerEventData& e);
	
	map<int, ofxOMXPlayer*> omxPlayers; 

};

