#include "ofxVideoPlayer.h"
#include "ofxOMXPlayer.h"
#include <thread>
#include <chrono>
class ofxVideoPlayerPrivate: public ofxOMXPlayerListener
{
public:
    ofxVideoPlayerPrivate(ofxVideoPlayer *_p){
        p = _p;
        omxPlayer = new ofxOMXPlayer();
    }
    static void reloadMovie(ofxOMXPlayer *player, ofxOMXPlayerSettings settings)
    {
        if(!player){
            return;
        }
        while(!player->setup(setting)){
            this_thread::sleep_for(chrono::seconds(2));
		}
    }
    virtual void onVideoEnd(ofxOMXPlayerListenerEventData& e){
        thread loadNewMovie(ofxVideoPlayerPrivate::reloadMovie, omxPlayer, omxPlayer->getSettings());
        loadNewMovie.detach();
    }
    virtual void onVideoLoop(ofxOMXPlayerListenerEventData& e){}
    ofxVideoPlayer *p = nullptr;
    ofxOMXPlayer *omxPlayer = nullptr;
};

ofxVideoPlayer::ofxVideoPlayer()
{
    d = new ofxVideoPlayerPrivate(this);
}

ofxOMXPlayer::~ofxVideoPlayer()
{
    if(d){
        delete d;
        d = nullptr;
    }
}

void ofxOMXPlayer::setPosition(int x, int y)
{
    this->x = x;
    this->y = y;
}

void ofxOMXPlayer::setSize(int w, int h)
{
    width = w;
    height = h;
}

void ofxOMXPlayer::setSource(std::string _source)
{
    source = _source;
}

void ofxOMXPlayer::play()
{
    ofxOMXPlayerSettings settings;
    settings.videoPath = files[i];
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
        d->onVideoEnd(ofxOMXPlayerListenerEventData());
    }
}