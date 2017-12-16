#include "ofApp.h"
#include <thread>
#include <chrono>
#include "Poco/FileStream.h"
//--------------------------------------------------------------

void ofApp::setup()
{	
    Poco::FileInputStream stream(string("t.txt"));
	std::string line;
	vector<string> files;
	while (std::getline(stream, line)) { 	files.push_back(line);	}
	
	for (int i=0; i<files.size(); i++) 
	{
        auto videoPath = files[i];

		auto x = 40+(640*(i %3));
		auto y = 200 + 480 * (i/3);
		
		auto width = 640;
		auto height = 480;	
		
		
		auto* player = new ofxVideoPlayer();
		player->setSize(width, height);
		player->setPosition(x, y);
		player->setSource(videoPath);
		player->play();
		omxPlayers[i] = player;
	}
}

//--------------------------------------------------------------
void ofApp::update()
{
	
	
	
}


//--------------------------------------------------------------
void ofApp::draw(){
	// ofBackgroundGradient(ofColor::red, ofColor::black, OF_GRADIENT_BAR);
	// for (int i=0; i<omxPlayers.size(); i++) 
	// {
	// 	ofxOMXPlayer* player = omxPlayers[i];
	// 	if (player->isPlaying())
	// 	{
	// 		ofPushMatrix();
	// 			ofTranslate(player->drawRectangle->x, 0, 0);
	// 			ofDrawBitmapStringHighlight(player->getInfo(), 60, 60, ofColor(ofColor::black, 90), ofColor::yellow);
	// 		ofPopMatrix();
	// 	}		
	// 	if(player->isPlaying()&&false)
	// 		player->draw(0,0,640,480);
	// }
	stringstream fpsInfo;
	fpsInfo <<"\n" <<  "APP FPS: "+ ofToString(ofGetFrameRate());
	ofDrawBitmapStringHighlight(fpsInfo.str(), 60, 20, ofColor::black, ofColor::yellow);
}

//--------------------------------------------------------------
void ofApp::keyPressed  (int key){
	switch (key) 
	{
		case 'c':
		{
			break;
		}
	}
	
}

