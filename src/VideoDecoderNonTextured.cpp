#include "VideoDecoderNonTextured.h"



VideoDecoderNonTextured::VideoDecoderNonTextured()
{

	doDeinterlace       = false;
	doHDMISync   = false;
	frameCounter = 0;
	frameOffset = 0;
}


VideoDecoderNonTextured::~VideoDecoderNonTextured()
{
	ofRemoveListener(ofEvents().update, this, &VideoDecoderNonTextured::onUpdate);
	//ofLogVerbose(__func__) << "removed update listener";
}

bool VideoDecoderNonTextured::open(OMXStreamInfo& streamInfo, OMXClock *clock, float display_aspect, bool deinterlace, bool hdmi_clock_sync)
{
	OMX_ERRORTYPE error   = OMX_ErrorNone;

	m_codingType            = OMX_VIDEO_CodingUnused;

	videoWidth  = streamInfo.width;
	videoHeight = streamInfo.height;

	doHDMISync = hdmi_clock_sync;

	if(!videoWidth || !videoHeight)
	{
		return false;
	}

	if(streamInfo.extrasize > 0 && streamInfo.extradata != NULL)
	{
		extraSize = streamInfo.extrasize;
		extraData = (uint8_t *)malloc(extraSize);
		memcpy(extraData, streamInfo.extradata, streamInfo.extrasize);
	}

	processCodec(streamInfo);

	if(deinterlace)
	{
		ofLog(OF_LOG_VERBOSE, "enable deinterlace\n");
		doDeinterlace = true;
	}
	else
	{
		doDeinterlace = false;
	}

	std::string componentName = "OMX.broadcom.video_decode";
	if(!decoderComponent.init(componentName, OMX_IndexParamVideoInit))
	{
		return false;
	}

	componentName = "OMX.broadcom.video_render";
	if(!renderComponent.init(componentName, OMX_IndexParamVideoInit))
	{
		return false;
	}

	componentName = "OMX.broadcom.video_scheduler";
	if(!schedulerComponent.init(componentName, OMX_IndexParamVideoInit))
	{
		return false;
	}

	if(doDeinterlace)
	{
		componentName = "OMX.broadcom.image_fx";
		if(!m_omx_image_fx.init(componentName, OMX_IndexParamImageInit))
		{
			return false;
		}
	}

	if(clock == NULL)
	{
		return false;
	}

	omxClock = clock;
	clockComponent = omxClock->getComponent();

	if(clockComponent->getHandle() == NULL)
	{
		omxClock = NULL;
		clockComponent = NULL;
		return false;
	}

	if(doDeinterlace)
	{
		decoderTunnel.init(&decoderComponent, decoderComponent.getOutputPort(), &m_omx_image_fx, m_omx_image_fx.getInputPort());
		m_omx_tunnel_image_fx.init(&m_omx_image_fx, m_omx_image_fx.getOutputPort(), &schedulerComponent, schedulerComponent.getInputPort());
	}
	else
	{
		decoderTunnel.init(&decoderComponent, decoderComponent.getOutputPort(), &schedulerComponent, schedulerComponent.getInputPort());
	}

	schedulerTunnel.init(&schedulerComponent, schedulerComponent.getOutputPort(), &renderComponent, renderComponent.getInputPort());
	clockTunnel.init(clockComponent, clockComponent->getInputPort() + 1, &schedulerComponent, schedulerComponent.getOutputPort() + 1);

	error = clockTunnel.Establish(false);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;

	error = decoderComponent.setState(OMX_StateIdle);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;

	OMX_VIDEO_PARAM_PORTFORMATTYPE formatType;
	OMX_INIT_STRUCTURE(formatType);
	formatType.nPortIndex = decoderComponent.getInputPort();
	formatType.eCompressionFormat = m_codingType;

	if (streamInfo.fpsscale > 0 && streamInfo.fpsrate > 0)
	{
		formatType.xFramerate = (long long)(1<<16)*streamInfo.fpsrate / streamInfo.fpsscale;
	}
	else
	{
		formatType.xFramerate = 25 * (1<<16);
	}
    
	error = decoderComponent.setParameter(OMX_IndexParamVideoPortFormat, &formatType);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;

	OMX_PARAM_PORTDEFINITIONTYPE portParam;
	OMX_INIT_STRUCTURE(portParam);
	portParam.nPortIndex = decoderComponent.getInputPort();

	error = decoderComponent.getParameter(OMX_IndexParamPortDefinition, &portParam);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;

	portParam.nPortIndex = decoderComponent.getInputPort();
	int videoBuffers = 60;
	portParam.nBufferCountActual = videoBuffers;

	portParam.format.video.nFrameWidth  = videoWidth;
	portParam.format.video.nFrameHeight = videoHeight;

	error = decoderComponent.setParameter(OMX_IndexParamPortDefinition, &portParam);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;

	OMX_PARAM_BRCMVIDEODECODEERRORCONCEALMENTTYPE concanParam;
	OMX_INIT_STRUCTURE(concanParam);
	concanParam.bStartWithValidFrame = OMX_FALSE;

	error = decoderComponent.setParameter(OMX_IndexParamBrcmVideoDecodeErrorConcealment, &concanParam);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;

	if (doDeinterlace)
	{
		// the deinterlace component requires 3 additional video buffers in addition to the DPB (this is normally 2).
		OMX_PARAM_U32TYPE extra_buffers;
		OMX_INIT_STRUCTURE(extra_buffers);
		extra_buffers.nU32 = 3;

		error = decoderComponent.setParameter(OMX_IndexParamBrcmExtraBuffers, &extra_buffers);
        OMX_TRACE(error);
        if(error != OMX_ErrorNone) return false;
	}

	// broadcom omx entension:
	// When enabled, the timestamp fifo mode will change the way incoming timestamps are associated with output images.
	// In this mode the incoming timestamps get used without re-ordering on output images.
	
    // recent firmware will actually automatically choose the timestamp stream with the least variance, so always enable

    OMX_CONFIG_BOOLEANTYPE timeStampMode;
    OMX_INIT_STRUCTURE(timeStampMode);
    timeStampMode.bEnabled = OMX_TRUE;
    error = decoderComponent.setParameter((OMX_INDEXTYPE)OMX_IndexParamBrcmVideoTimestampFifo, &timeStampMode);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;


	if(NaluFormatStartCodes(streamInfo.codec, extraData, extraSize))
	{
		OMX_NALSTREAMFORMATTYPE nalStreamFormat;
		OMX_INIT_STRUCTURE(nalStreamFormat);
		nalStreamFormat.nPortIndex = decoderComponent.getInputPort();
		nalStreamFormat.eNaluFormat = OMX_NaluFormatStartCodes;

		error = decoderComponent.setParameter((OMX_INDEXTYPE)OMX_IndexParamNalStreamFormatSelect, &nalStreamFormat);
        OMX_TRACE(error);
        if(error != OMX_ErrorNone) return false;
	}

	if(doHDMISync)
	{
		OMX_CONFIG_LATENCYTARGETTYPE latencyTarget;
		OMX_INIT_STRUCTURE(latencyTarget);
		latencyTarget.nPortIndex = renderComponent.getInputPort();
		latencyTarget.bEnabled = OMX_TRUE;
		latencyTarget.nFilter = 2;
		latencyTarget.nTarget = 4000;
		latencyTarget.nShift = 3;
		latencyTarget.nSpeedFactor = -135;
		latencyTarget.nInterFactor = 500;
		latencyTarget.nAdjCap = 20;

		error = renderComponent.setConfig(OMX_IndexConfigLatencyTarget, &latencyTarget);
        OMX_TRACE(error);
        if(error != OMX_ErrorNone) return false;
	}

	// Alloc buffers for the omx input port.
	error = decoderComponent.allocInputBuffers();
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;

	error = decoderTunnel.Establish(false);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;

	
	error = decoderComponent.setState(OMX_StateExecuting);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;

	if(doDeinterlace)
	{
		OMX_CONFIG_IMAGEFILTERPARAMSTYPE image_filter;
		OMX_INIT_STRUCTURE(image_filter);

		image_filter.nPortIndex = m_omx_image_fx.getOutputPort();
		image_filter.nNumParams = 1;
		image_filter.nParams[0] = 3;
		image_filter.eImageFilter = OMX_ImageFilterDeInterlaceAdvanced;

		error = m_omx_image_fx.setConfig(OMX_IndexConfigCommonImageFilterParameters, &image_filter);
        OMX_TRACE(error);
        if(error != OMX_ErrorNone) return false;

		error = m_omx_tunnel_image_fx.Establish(false);
        OMX_TRACE(error);
        if(error != OMX_ErrorNone) return false;

		error = m_omx_image_fx.setState(OMX_StateExecuting);
        OMX_TRACE(error);
        if(error != OMX_ErrorNone) return false;

		m_omx_image_fx.disablePort(m_omx_image_fx.getInputPort());
		m_omx_image_fx.disablePort(m_omx_image_fx.getOutputPort());

	}

	error = schedulerTunnel.Establish(false);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;

	
	error = schedulerComponent.setState(OMX_StateExecuting);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;
	
	
	
	error = renderComponent.setState(OMX_StateExecuting);
    OMX_TRACE(error);
    if(error != OMX_ErrorNone) return false;
    
	ofAddListener(ofEvents().update, this, &VideoDecoderNonTextured::onUpdate);
	if(!sendDecoderConfig())
	{
		return false;
	}

	isOpen           = true;
	doSetStartTime      = true;
	OMX_CONFIG_DISPLAYREGIONTYPE configDisplay;
	OMX_INIT_STRUCTURE(configDisplay);
	configDisplay.nPortIndex = renderComponent.getInputPort();
	
    display.setup(renderComponent, streamInfo, displayRect);
#if 0
	//we provided a rectangle but returned early as we were not ready
	if (displayRect.getWidth()>0) 
	{
		configureDisplay();
	}else 
	{
		
		float fAspect = (float)streamInfo.aspect / (float)videoWidth * (float)videoHeight;
        
        float par = 0.0f;
        if(streamInfo.aspect)
        {
            par = fAspect/display_aspect;
        }
		//float par = streamInfo.aspect ? fAspect/display_aspect : 0.0f;
		// only set aspect when we have a aspect and display doesn't match the aspect
		bool doDisplayChange = true;
		if(doDisplayChange)
		{
			if(par != 0.0f && fabs(par - 1.0f) > 0.01f)
			{
				
				
				AVRational aspect;
				aspect = av_d2q(par, 100);
				configDisplay.set      = OMX_DISPLAY_SET_PIXEL;
				configDisplay.pixel_x  = aspect.num;
				configDisplay.pixel_y  = aspect.den;
				ofLog(OF_LOG_VERBOSE, "Aspect : num %d den %d aspect %f pixel aspect %f\n", aspect.num, aspect.den, streamInfo.aspect, par);
				error = renderComponent.setConfig(OMX_IndexConfigDisplayRegion, &configDisplay);
                OMX_TRACE(error);
                if(error != OMX_ErrorNone) return false;
			}
			
		}
	}
#endif
	ofLog(OF_LOG_VERBOSE,
	      "%s::%s - decoder_component(0x%p), input_port(0x%x), output_port(0x%x) deinterlace %d hdmiclocksync %d\n",
	      "VideoDecoderNonTextured", __func__, decoderComponent.getHandle(), decoderComponent.getInputPort(), decoderComponent.getOutputPort(),
	      doDeinterlace, doHDMISync);

	isFirstFrame   = true;

	// start from assuming all recent frames had valid pts
	validHistoryPTS = ~0;
	return true;
}

void VideoDecoderNonTextured::onUpdate(ofEventArgs& args)
{
    //TODO: seems to cause hang on exit
    
	//updateFrameCount();
}
void VideoDecoderNonTextured::updateFrameCount()
{
	if (!isOpen) {
		return;
	}
	OMX_ERRORTYPE error;
	OMX_CONFIG_BRCMPORTSTATSTYPE stats;
	
	OMX_INIT_STRUCTURE(stats);
	
	stats.nPortIndex = renderComponent.getInputPort();
	
	error = renderComponent.getParameter(OMX_IndexConfigBrcmPortStats, &stats);
    OMX_TRACE(error);
	if (error == OMX_ErrorNone)
	{
		/*OMX_U32 nImageCount;
		 OMX_U32 nBufferCount;
		 OMX_U32 nFrameCount;
		 OMX_U32 nFrameSkips;
		 OMX_U32 nDiscards;
		 OMX_U32 nEOS;
		 OMX_U32 nMaxFrameSize;
		 
		 OMX_TICKS nByteCount;
		 OMX_TICKS nMaxTimeDelta;
		 OMX_U32 nCorruptMBs;*/
		//ofLogVerbose(__func__) << "nFrameCount: " << stats.nFrameCount;
		frameCounter = stats.nFrameCount;
	}else
	{
		ofLogError(__func__) << "renderComponent OMX_CONFIG_BRCMPORTSTATSTYPE fail: ";
	}
}

int VideoDecoderNonTextured::getCurrentFrame()
{
	return frameCounter - frameOffset;
}

void VideoDecoderNonTextured::resetFrameCounter()
{
	frameOffset = frameCounter;
}


bool VideoDecoderNonTextured::decode(uint8_t *pData, int iSize, double pts)
{
	SingleLock lock (m_critSection);
	OMX_ERRORTYPE error;

	if(!isOpen )
	{
		return true;
	}
	
	unsigned int demuxer_bytes = (unsigned int)iSize;
	uint8_t *demuxer_content = pData;

	if (demuxer_content && demuxer_bytes > 0)
	{
		while(demuxer_bytes)
		{
			// 500ms timeout
			OMX_BUFFERHEADERTYPE *omxBuffer = decoderComponent.getInputBuffer(500);
			if(omxBuffer == NULL)
			{
				ofLogError(__func__) << "Decode timeout";
				return false;
			}

			omxBuffer->nFlags = 0;
			omxBuffer->nOffset = 0;

			if(doSetStartTime)
			{
				omxBuffer->nFlags |= OMX_BUFFERFLAG_STARTTIME;
				ofLog(OF_LOG_VERBOSE, "VideoDecoderNonTextured::Decode VDec : setStartTime %f\n", (pts == DVD_NOPTS_VALUE ? 0.0 : pts) / DVD_TIME_BASE);
				doSetStartTime = false;
			}
			else if(pts == DVD_NOPTS_VALUE)
			{
				omxBuffer->nFlags |= OMX_BUFFERFLAG_TIME_UNKNOWN;
			}

			omxBuffer->nTimeStamp = ToOMXTime((uint64_t)(pts == DVD_NOPTS_VALUE) ? 0 : pts);
			omxBuffer->nFilledLen = (demuxer_bytes > omxBuffer->nAllocLen) ? omxBuffer->nAllocLen : demuxer_bytes;
			memcpy(omxBuffer->pBuffer, demuxer_content, omxBuffer->nFilledLen);

			demuxer_bytes -= omxBuffer->nFilledLen;
			demuxer_content += omxBuffer->nFilledLen;

			if(demuxer_bytes == 0)
			{
				//ofLogVerbose(__func__) << "OMX_BUFFERFLAG_ENDOFFRAME";
				omxBuffer->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
			}

			int nRetry = 0;
			while(true)
			{
				error = decoderComponent.EmptyThisBuffer(omxBuffer);
                OMX_TRACE(error);
				if (error == OMX_ErrorNone)
				{
					//ofLog(OF_LOG_VERBOSE, "VideD:  pts:%.0f size:%d)\n", pts, iSize);
					
					break;
				}
				else
				{
					ofLogError(__func__) << "OMX_EmptyThisBuffer() FAIL: ";
					nRetry++;
				}
				if(nRetry == 5)
				{
					ofLogError(__func__) << "OMX_EmptyThisBuffer() FAILED 5 TIMES";
					return false;
				}
				
			}
			
		}
		
		return true;
	}

	return false;
}

void VideoDecoderNonTextured::configureDisplay()
{
#if 0
	OMX_CONFIG_DISPLAYREGIONTYPE configDisplay;
	OMX_INIT_STRUCTURE(configDisplay);
	configDisplay.nPortIndex = renderComponent.getInputPort();
	
	
	
	configDisplay.set     = OMX_DISPLAY_SET_FULLSCREEN;
	configDisplay.fullscreen = OMX_FALSE;
	
	renderComponent.setConfig(OMX_IndexConfigDisplayRegion, &configDisplay);
	
	configDisplay.set     = OMX_DISPLAY_SET_DEST_RECT;
	configDisplay.dest_rect.x_offset  = displayRect.x;
	configDisplay.dest_rect.y_offset  = displayRect.y;
	configDisplay.dest_rect.width     = displayRect.getWidth();
	configDisplay.dest_rect.height    = displayRect.getHeight();
	
	renderComponent.setConfig(OMX_IndexConfigDisplayRegion, &configDisplay);
#endif
}
void VideoDecoderNonTextured::setDisplayRect(ofRectangle& rectangle)
{
	
	bool hasChanged = (displayRect != rectangle);
    
    if (hasChanged) 
	{
		displayRect = rectangle;
	}
	
	if(!isOpen)
	{
		return;
	}
	if (hasChanged) 
	{
		
		configureDisplay();
	}	
}



