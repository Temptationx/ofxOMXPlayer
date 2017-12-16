#pragma once
#include "ofxOMXPlayer.h"
#include "ofBaseTypes.h"
class ofxVideoPlayerPrivate;
class ofxVideoPlayer
{
public: 
    ofxVideoPlayer();
    virtual ~ofxVideoPlayer();
    void setPosition(int x, int y);
    void setSize(int w, int h);
    void setSource(std::string _source);
    void play();
private:
    ofxVideoPlayerPrivate *d = nullptr;
    friend ofxVideoPlayerPrivate;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    std::string source;
};