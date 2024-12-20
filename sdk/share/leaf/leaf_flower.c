#include "iniparser/iniparser.h"

#include "leaf_flower.h"
#include "castor3player.h"

LeafCall* leaf_init(void) {    
	LeafCall *call = (LeafCall*)ite_new0(LeafCall,1);

    call->stream_flow = IteStreamInit();
    
	return call;
}

uint8_t leaf_uninit(LeafCall *call) {
    ite_flower_deinit();
    if(call->stream_flow)
        free(call->stream_flow);
    if(call)
        free(call); 
    return 0;
}

uint8_t leaf_init_video_streams (LeafCall *call, unsigned short port) {
	return 0;
}

uint8_t leaf_init_audio_streams (LeafCall *call, unsigned short port) {
	return 0;
}

uint8_t leaf_start_video_stream(LeafCall *call, Call_info *call_list, unsigned short port) {
	VideoStreamDir dir=VideoStreamSendRecv;

	if(call->dir == CallOutgoing)
		dir = VideoStreamSendOnly;
	else
		dir = VideoStreamRecvOnly;

    call->video_port = port;
    flow_start_udp_videostream(call->stream_flow, call_list, port, dir);
    return 0;
}

uint8_t leaf_start_audio_stream(LeafCall *call, Call_info *call_list, unsigned short port) {
    call->audio_port = port;
	flow_start_udp_audiostream(call->stream_flow, 8000, ULAW, call_list, port);
    return 0;
}

uint8_t leaf_stop_media_streams(LeafCall *call) {
	if (call->stream_flow->audiostream != NULL) {
		flow_stop_audioflow(call->stream_flow);
		call->stream_flow->audiostream = NULL;
	}
	if (call->stream_flow->videostream != NULL){
        VideoStreamDir dir=VideoStreamSendRecv;
        if(call->dir == CallOutgoing)
		    dir = VideoStreamSendOnly;
	    else
		    dir = VideoStreamRecvOnly;
		flow_stop_udp_videostream(call->stream_flow, dir);
		call->stream_flow->videostream = NULL;
	}
    return 0;
}

uint8_t leaf_take_video_snapshot(LeafCall *call, char *file,FfilewriterCallback func ){
	filewriter_callback = func;   
    if (call->stream_flow->videostream !=NULL){
		ite_filter_call_method(call->stream_flow->videostream->Fjpegwriter, ITE_FILTER_JPEG_SNAPSHOT, (void*)file);
		return 1;
    }
	else
    {
		printf("Cannot take snapshot: no currently running video stream on this call.\n");
		return 0;
    }
}

bool leaf_video_memo_is_recording(LeafCall *call)
{
    return call->VideomemoRecording;
}

void leaf_start_video_memo_record(LeafCall *call, char *file, int width, int height, int fps)
{
    if (call->stream_flow->videostream !=NULL)
    {
        if(!call->VideomemoRecording)
        {
        	//pthread_t tid;
			VideoMemoInfo *info = (VideoMemoInfo*)ite_new0(VideoMemoInfo,1);
            strcpy(info->videomemo_file, file);
			info->video_width = width;
			info->video_height = height;
			info->video_fps = fps;
			ite_filter_call_method(call->stream_flow->videostream->Frec_avi, ITE_FILTER_REC_AVI_OPEN, (void*)info);
			if(info)
				free(info);
			call->VideomemoRecording = true;			
        } 		
    }
	else
	{
		printf("Cannot record video: no currently running video stream on this call.\n");
    }
}

void leaf_stop_video_memo_record(LeafCall *call)
{
    if (call->stream_flow->videostream !=NULL)
    {
        if(call->VideomemoRecording)
        {
        	ite_filter_call_method(call->stream_flow->videostream->Frec_avi, ITE_FILTER_REC_AVI_CLOSE, NULL);
    		call->VideomemoRecording = false;
        }  
    }
	else
	{
		printf("Cannot record video: no currently running video stream on this call.\n");
    }
}

void leaf_start_ipcam_stream(LeafCall *call, const char *addr, int port) {
#ifdef CFG_RTSP_CLIENT_ENABLE
	flow_start_recv_ipcamstream(call->stream_flow);
    SetRTSPClientMode(IPCAM_MODE);
    startRTSPClient(addr, port, NULL, NULL);
#endif    
} 

void leaf_stop_ipcam_stream(LeafCall *call) {
#ifdef CFG_RTSP_CLIENT_ENABLE
    stopRTSPClient();
	if (call->stream_flow->videostream != NULL){
	flow_stop_recv_ipcamstream(call->stream_flow);
		call->stream_flow->videostream = NULL;
	}
#endif    
}

