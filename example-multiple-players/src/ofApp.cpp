#include "ofApp.h"
#include <thread>
//--------------------------------------------------------------
void ofApp::setup()
{	
	
	vector<string> files;
	auto buffer = ofBufferFromFile("t.txt");
	for(auto line:buffer.getLines()){
		files.push_back(line);
		cout<<"Read URL from t.txt:"<<line<<endl;
	}
	
	for (int i=0; i<files.size(); i++) 
	{
		ofxOMXPlayerSettings settings;
                settings.videoPath = "rtsp://192.168.0.101:554/stream";
		settings.useHDMIForAudio = true;	//default true
		settings.enableLooping = false;		//default true
		settings.enableAudio = false;		//default true, save resources by disabling
		settings.enableTexture =false;		//default true
		if (!settings.enableTexture)
		{
			/*
				We have the option to pass in a rectangle
				to be used for a non-textured player to use (as opposed to the default full screen)
				*/
			
			settings.directDisplayOptions.drawRectangle.x = 40+(640*i);
			settings.directDisplayOptions.drawRectangle.y = 200;
			
			settings.directDisplayOptions.drawRectangle.width = 640;
			settings.directDisplayOptions.drawRectangle.height = 480;
			
			
		}
		
		ofxOMXPlayer* player = new ofxOMXPlayer();
		if(player->setup(settings)){

		}
		omxPlayers[i] = player;
	}
}

//--------------------------------------------------------------
void ofApp::update()
{
	
	
	
}
void loadMovie(ofxOMXPlayer *player, ofxOMXPlayerSettings setting){
	if(!player){
		return;
	}
	if(!player->setup(setting)){
		
	}
}
void ofApp::onVideoEnd(ofxOMXPlayerListenerEventData& e){
	auto engine = (ofxOMXPlayerEngine*)e.listener;
	for(int i=0;i<omxPlayers.size();i++){
		if(omxPlayers[i]->engine == engine){
			auto setting = omxPlayers[i]->settings;
			delete omxPlayers[i];
			auto newPlayer = new ofxOMXPlayer();
			omxPlayers[i] = newPlayer;
			thread loadNewMovie(loadMovie, newPlayer, setting);
			loadNewMovie.detach();
		}
	}
}
void ofApp::onVideoLoop(ofxOMXPlayerListenerEventData& e){

}

//--------------------------------------------------------------
void ofApp::draw(){
	ofBackgroundGradient(ofColor::red, ofColor::black, OF_GRADIENT_BAR);
	for (int i=0; i<omxPlayers.size(); i++) 
	{
		ofxOMXPlayer* player = omxPlayers[i];
		if (player->isPlaying())
		{
			ofPushMatrix();
				ofTranslate(player->drawRectangle->x, 0, 0);
				ofDrawBitmapStringHighlight(player->getInfo(), 60, 60, ofColor(ofColor::black, 90), ofColor::yellow);
			ofPopMatrix();
		}		
		if(player->isPlaying()&&false)
			player->draw(0,0,640,480);
	}
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

