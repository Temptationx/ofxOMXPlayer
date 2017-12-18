#include "ofxVideoPlayer.h"
#ifndef WIN32
#include "ofxOMXPlayer.h"
#endif
#include <thread>
#include <chrono>
#ifndef WIN32
class ofxVideoPlayerPrivate: public ofxOMXPlayerListener
#else
class ofxVideoPlayerPrivate
#endif
{
public:
    ofxVideoPlayerPrivate(ofxVideoPlayer *_p){
        p = _p;
#ifndef WIN32
		player = new ofxOMXPlayer();
#else

#endif
    }
    static void reloadMovie(ofxVideoPlayerPrivate *d)
    {
        if(!d || !d->player){
            return;
        }
#ifndef WIN32
        while(!d->player->setup(d->player->getSettings()))
#else
		while(false)
#endif
		{
            this_thread::sleep_for(chrono::seconds(2));
		}
    }

    ofxVideoPlayer *p = nullptr;
#ifndef WIN32
	virtual void onVideoEnd(ofxOMXPlayerListenerEventData& e) {
		thread loadNewMovie(ofxVideoPlayerPrivate::reloadMovie, omxPlayer, omxPlayer->getSettings());
		loadNewMovie.detach();
	}
	virtual void onVideoLoop(ofxOMXPlayerListenerEventData& e) {}
    ofxOMXPlayer *player = nullptr;
#else
	int *player = nullptr;
#endif
};

ofxVideoPlayer::ofxVideoPlayer()
{
    d = new ofxVideoPlayerPrivate(this);
}

ofxVideoPlayer::~ofxVideoPlayer()
{
    if(d){
        delete d;
        d = nullptr;
    }
}

void ofxVideoPlayer::setPosition(int x, int y)
{
    this->x = x;
    this->y = y;
}

void ofxVideoPlayer::setSize(int w, int h)
{
    width = w;
    height = h;
}

void ofxVideoPlayer::setSource(std::string _source)
{
    source = _source;
}

void ofxVideoPlayer::drawInfoText()
{
#ifndef WIN32
    if(d->omxPlayer->isPlaying()){
        ofPushMatrix();
        ofTranslate(d->omxPlayer->drawRectangle->x, 0, 0);
        ofDrawBitmapStringHighlight(d->omxPlayer->getInfo(), 60, 60, ofColor(ofColor::black, 90), ofColor::yellow);
        ofPopMatrix();
    }
#else

#endif
}

bool ofxVideoPlayer::isPlaying()
{
#ifndef WIN32
	return d->omxPlayer->isPlaying();
#else
	return false;
#endif // !WIN32

}

void ofxVideoPlayer::play()
{
#ifndef WIN32
    ofxOMXPlayerSettings settings;
    settings.videoPath = source;
    settings.useHDMIForAudio = true;	//default true
    settings.enableLooping = false;		//default true
    settings.enableAudio = false;		//default true, save resources by disabling
    settings.enableTexture =false;		//default true
    settings.listener = d;
    if (!settings.enableTexture)
    {
        /*
            We have the option to pass in a rectangle
            to be used for a non-textured player to use (as opposed to the default full screen)
            */
        
        settings.directDisplayOptions.drawRectangle.x = x;
        settings.directDisplayOptions.drawRectangle.y = y;
        
        settings.directDisplayOptions.drawRectangle.width = width;
        settings.directDisplayOptions.drawRectangle.height = height;
    }
    if(!d->omxPlayer->setup(settings)){
        auto e = ofxOMXPlayerListenerEventData(nullptr);
        d->onVideoEnd(e);
    }
#else

#endif
}