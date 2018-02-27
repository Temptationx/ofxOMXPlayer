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
	virtual ~ofxVideoPlayerPrivate(){}
    static void reloadMovie(ofxVideoPlayerPrivate *d)
    {
    	if(!d){
    		return;
    	}

        if(d->player){
            delete d->player;
        }
        d->player = new ofxOMXPlayer();

#ifndef WIN32
        while(!d->player->setup(d->settings))
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
		thread loadNewMovie(ofxVideoPlayerPrivate::reloadMovie, this);
		loadNewMovie.detach();
	}
	virtual void onVideoLoop(ofxOMXPlayerListenerEventData& e) {}
    ofxOMXPlayer *player = nullptr;
    ofxOMXPlayerSettings settings;

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

void ofxVideoPlayer::setUseTexture(bool use)
{
	useTexture = use;
}

void ofxVideoPlayer::drawInfoText()
{
#ifndef WIN32
    if(d->player->isPlaying()){
        ofPushMatrix();
        ofTranslate(d->player->drawRectangle->x, 0, 0);
        ofDrawBitmapStringHighlight(d->player->getInfo(), 60, 60, ofColor(ofColor::black, 90), ofColor::yellow);
        ofPopMatrix();
    }
#else

#endif
}

bool ofxVideoPlayer::isPlaying()
{
#ifndef WIN32
	return d->player->isPlaying();
#else
	return false;
#endif // !WIN32

}

void ofxVideoPlayer::draw()
{
#ifndef TARGET_WIN32
	if (d->player->isTextureEnabled()) {
		d->player->draw(x, y, width, height);
	}
#endif
}

void ofxVideoPlayer::play()
{
	thread playMe(&ofxVideoPlayer::playAsync, this);
	playMe.detach();

}

void ofxVideoPlayer::playAsync()
{
#ifndef WIN32
	ofxOMXPlayerSettings settings;
	settings.videoPath = source;
	settings.useHDMIForAudio = true;	//default true
	settings.enableLooping = false;		//default true
	settings.enableAudio = false;		//default true, save resources by disabling
	settings.enableTexture = useTexture;		//default true
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
		settings.directDisplayOptions.doForceFill = true;
		settings.directDisplayOptions.noAspectRatio = true;
	}
	d->settings = settings;
	if (!d->player->setup(settings)) {
		auto e = ofxOMXPlayerListenerEventData(nullptr);
		d->onVideoEnd(e);
	}
#else

#endif
}
