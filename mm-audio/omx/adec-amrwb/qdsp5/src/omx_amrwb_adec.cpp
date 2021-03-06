/*============================================================================
                            O p e n M A X   w r a p p e r s
                             O p e n  M A X   C o r e

*//** @file omx_amrwb_adec.cpp
  This module contains the implementation of the OpenMAX core & component.

Copyright (c) 2006-2008 Qualcomm Technologies, Inc.
All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
*//*========================================================================*/

//////////////////////////////////////////////////////////////////////////////
//                             Include Files
//////////////////////////////////////////////////////////////////////////////

//#define LOG_NDEBUG 0
#include<string.h>
#include <fcntl.h>
#include <omx_amrwb_adec.h>
#include <sys/ioctl.h>

#include <linux/msm_audio.h>
#include <semaphore.h>
#include <errno.h>
using namespace std;


/* The frame length look-up table */
unsigned char audamrwb_frm_data_len_table[] = {
  /* FT=0, AMR-WB 6.60 kbit/s */
  17,
  /* FT=1, AMR-WB 8.85 kbit/s */
  23,
  /* FT=2, AMR-WB 12.65 kbit/s */
  32,
  /* FT=3, AMR-WB 14.25 kbit/s */
  36,
  /* FT=4, AMR-WB 15.85 kbit/s */
  40,
  /* FT=5, AMR-WB 18.25 kbit/s */
  46,
  /* FT=6, AMR-WB 19.85 kbit/s */
  50,
  /* FT=7, AMR-WB 23.05 kbit/s */
  58,
  /* FT=8, AMR-WB 23.85 kbit/s */
  60,
  /* FT=9, AMR-WB SID */
  5,
  /* FT=10, AMR-WB+ 13.6 kbit/s mono */
  34,
  /* FT=11, AMR-WB+ 18 kbit/s stereo */
  45,
  /* FT=12, AMR-WB+ 24 kbit/s mono */
  60,
  /* FT=13, AMR-WB+ 24 kbit/s stereo */
  60,
  /* FT=14, FRAME_ERASURE */
  0,
  /* FT=15, NO_DATA */
  0,
  /* FT=16, AMR-WB+ 10.4 kbit/s */
  26,
  /* FT=17, AMR-WB+ 12.0 kbit/s */
  30,
  /* FT=18, AMR-WB+ 13.6 kbit/s */
  34,
  /* FT=19, AMR-WB+ 15.2 kbit/s */
  38,
  /* FT=20, AMR-WB+ 16.8 kbit/s */
  42,
  /* FT=21, AMR-WB+ 19.2 kbit/s */
  48,
  /* FT=22, AMR-WB+ 20.8 kbit/s */
  52,
  /* FT=23, AMR-WB+ 24.0 kbit/s */
  60,
  /* FT=24, AMR-WB+ 12.4 kbit/s */
  31,
  /* FT=25, AMR-WB+ 12.8 kbit/s */
  32,
  /* FT=26, AMR-WB+ 14 kbit/s */
  35,
  /* FT=27, AMR-WB+ 14.4 kbit/s */
  36,
  /* FT=28, AMR-WB+ 15.2 kbit/s */
  38,
  /* FT=29, AMR-WB+ 16 kbit/s */
  40,
  /* FT=30, AMR-WB+ 16.4 kbit/s */
  41,
  /* FT=31, AMR-WB+ 17.2 kbit/s */
  43,
  /* FT=32, AMR-WB+ 18 kbit/s */
  45,
  /* FT=33, AMR-WB+ 18.4 kbit/s */
  46,
  /* FT=34, AMR-WB+ 19.2 kbit/s */
  48,
  /* FT=35, AMR-WB+ 20 kbit/s */
  50,
  /* FT=36, AMR-WB+ 20.4 kbit/s */
  51,
  /* FT=37, AMR-WB+ 21.2 kbit/s */
  53,
  /* FT=38, AMR-WB+ 22.4 kbit/s */
  56,
  /* FT=39, AMR-WB+ 23.2 kbit/s */
  58,
  /* FT=40, AMR-WB+ 24 kbit/s */
  60,
  /* FT=41, AMR-WB+ 25.6 kbit/s */
  64,
  /* FT=42, AMR-WB+ 26 kbit/s */
  65,
  /* FT=43, AMR-WB+ 26.8 kbit/s */
  67,
  /* FT=44, AMR-WB+ 28.8 kbit/s */
  72,
  /* FT=45, AMR-WB+ 29.6 kbit/s */
  74,
  /* FT=46, AMR-WB+ 30 kbit/s */
  75,
  /* FT=47, AMR-WB+ 32 kbit/s */
  80
};


OMX_U32 omx_amr_adec::AMRTranscode
(
 OMX_U8        *dst,
 OMX_U8        *src
 )
{
    OMX_U32    trans_length = 0;

    /*fill the 1st and 2nd bytes with the sync words 0xFF and 0xFF*/
    dst[0] = 0xFF;
    dst[1] = 0xFF;

    /*make the next 6 bits as 0. Calculate the length of the super frame
      and fill th 7th, 8th bit of 3rd byte and 8 bits of the 4th byte */

    dst[2] = ((frameLength & FRMLEN_MASK) >> SHIFT_TWO) & FRMLEN_2BITS_MASK;
    dst[3] = frameLength & FRMLEN_8BITS_MASK;

    if(!amrwb)
    {
        /* First 4 bytes of the frame is made, now copy the data present
    in the input buffer for this frame */
        memcpy(dst + HEADER_LENGTH, src, frameSize);
    }
    else
    {
    dst[4] = frameType; //mode ranges from 0 to 9, 14 and 15
    dst[5] = 0; //ISF is 0 for AMR WB
    /* First 6 bytes of the frame is made, now copy the data present
        in the input buffer for this frame */
        memcpy(dst + 6, src + 1, frameLength);//skip one byte of source buffer.
    }

    trans_length = 6 + frameLength;
    if(frameLength%2)
    {
        dst[frameLength + 6] = 0; //zero padding to make length even
        trans_length += 1;
    }

    return(trans_length);
}



/*===========================================================================

FUNCTION FillSrcFrame

DESCRIPTION
  This function fills the frame with the data to be transcoded.

DEPENDENCIES
  Caller needs to make sure destination buffer has at least one full 12.2 kbit/s
  frame worth of empty space left to hold the transcoded AMR frame.
  First byte of source buffer must be the frame header containg the frame type
  and quality bit information.

RETURN VALUE
  Returns TRUE or FALSE depending upon the success or failure of the function

SIDE EFFECTS
  None

===========================================================================*/
OMX_U8 omx_amr_adec::FillSrcFrame(OMX_U8 *srcFrame,
                                  OMX_U8 **src,
                                  OMX_U32 srcDataLen,
                                  OMX_U32 *srcDataConsumed)
{
    OMX_U32 ISFindex = 0;

    if (!m_residual_data_len)
    {
    if(!amrwb)
    {
        /*calculate frame type from the input source buffer.*/
            frameType   = src[0][0];
            ISFindex    = src[0][1];
    }
    else
    {
        frameType = ((**src & AMR_FRAME_TYPE_MASK) >>
                    AMR_FRAME_TYPE_SHIFT);
    }
    }
    else
    {
    if(!amrwb)
    {
        /*calculate frame type here from the residual buffer.*/
            frameType   = m_residual_buffer[0];
        ISFindex    = m_residual_buffer[1];
    }
    else
    {
        frameType = ((*(m_residual_buffer) & AMR_FRAME_TYPE_MASK) >>
                        AMR_FRAME_TYPE_SHIFT);
    }
    }

    /* check whether frame type is wb or wb+. modes 0 to 9 is wb and
       10 to 47 is wb+. Frame Length is calculated based on the mode.
       If it is wb, then length is same as that of one frame, if wb+,
       length is 4 times length of one frame. this is a super frame. */


    if(!amrwb)
    {
    if(frameType <= AMRWB_MODE_MAX)
        {
            frameLength = audamrwb_frm_data_len_table[frameType];
        frameSize = frameLength + FRMHDR_LEN;
    }
        else if(AMRWBP_MODE_MIN <= frameType && frameType <= AMRWBP_MODE_MAX)
        {
        frameLength = (MAX_FPS * audamrwb_frm_data_len_table[frameType]);
        frameSize = frameLength + FRMHDR_LEN;
    }
    else
    {
        DEBUG_PRINT_ERROR("ERROR : WRONG FRAMETYPE\n");
        return FALSE;
    }
    }
    else if((frameType > AMRWB_MODE_MAX) && (frameType != 14 && frameType != 15))
    {
        DEBUG_PRINT_ERROR("Mode not supported\n");
        return FALSE;
    }
    else
    {
    frameLength = audamrwb_frm_data_len_table[frameType];
    frameSize = frameLength + 1;
    }

    if(!m_residual_data_len)
    {
        if((srcDataLen - *srcDataConsumed) >= frameSize)
        {
            memcpy(srcFrame, *src, frameSize);
            *srcDataConsumed += frameSize;
            *src += frameSize;
            m_is_full_frame = TRUE;
        }
        else
        {
            memcpy(m_residual_buffer, *src, (srcDataLen - *srcDataConsumed));
            m_residual_data_len += (srcDataLen - *srcDataConsumed);
            *srcDataConsumed = 0;
            m_is_full_frame = FALSE;
        }
    }
    else
    {
        if(m_residual_data_len >= frameSize)
        {
            memcpy(srcFrame, m_residual_buffer, frameSize);
            m_residual_buffer += frameSize;
            m_residual_data_len -= frameSize;
            m_is_full_frame = TRUE;
        }
        else
        {
            memcpy(srcFrame, m_residual_buffer, m_residual_data_len);
            srcFrame += m_residual_data_len;
            if(srcDataLen >= frameSize - m_residual_data_len )
            {
                memcpy(srcFrame, *src, (frameSize - m_residual_data_len));
                *src += (frameSize - m_residual_data_len);
                *srcDataConsumed += (frameSize - m_residual_data_len);
                m_residual_data_len = 0;
                m_residual_buffer = m_residual_buffer_start;
                memset(m_residual_buffer, 0,
                             sizeof(OMX_U8) * RESIDUALDATA_BUFFER_SIZE);
                m_is_full_frame = TRUE;
            }
            else
            {
                m_is_full_frame = FALSE;
                DEBUG_PRINT("Insufficient data\n");
                return FALSE;
            }
        }
    }
    return TRUE;
}

// omx_cmd_queue destructor
omx_amr_adec::omx_cmd_queue::~omx_cmd_queue()
{
    // Nothing to do
}

// omx cmd queue constructor
omx_amr_adec::omx_cmd_queue::omx_cmd_queue(): m_read(0),m_write(0),m_size(0)
{
    memset(m_q,      0,sizeof(omx_event)*OMX_CORE_CONTROL_CMDQ_SIZE);
}

// omx cmd queue insert
bool omx_amr_adec::omx_cmd_queue::insert_entry(unsigned p1,
                                               unsigned p2,
                                               unsigned id)
{
    bool ret = true;
    if ( m_size < OMX_CORE_CONTROL_CMDQ_SIZE )
    {
        m_q[m_write].id       = id;
        m_q[m_write].param1   = p1;
        m_q[m_write].param2   = p2;
        m_write++;
        m_size ++;
        if ( m_write >= OMX_CORE_CONTROL_CMDQ_SIZE )
        {
            m_write = 0;
        }
    }
    else
    {
        ret = false;
        DEBUG_PRINT_ERROR("ERROR!!! Command Queue Full");
    }
    return ret;
}

/*=============================================================================
FUNCTION:
  pop_entry

DESCRIPTION:
  reads the commands that are inserted in the queue

INPUT/OUTPUT PARAMETERS:
  [OUT] p1
  [OUT] p2
  [OUT] id

RETURN VALUE:
  FALSE Failure
  TRUE Success

Dependency:
  None

SIDE EFFECTS:
   None
=============================================================================*/
bool omx_amr_adec::omx_cmd_queue::pop_entry(unsigned *p1,
                                            unsigned *p2,
                                            unsigned *id)
{
    bool ret = true;
    if ( m_size > 0 )
    {
        *id = m_q[m_read].id;
        *p1 = m_q[m_read].param1;
        *p2 = m_q[m_read].param2;
        // Move the read pointer ahead
        ++m_read;
        --m_size;
        if ( m_read >= OMX_CORE_CONTROL_CMDQ_SIZE )
        {
            m_read = 0;

        }
    }
    else
    {
        ret = false;
        DEBUG_PRINT_ERROR("ERROR Delete!!! Command Queue Empty");
    }
    return ret;
}

// factory function executed by the core to create instances
void *get_omx_component_factory_fn(void)
{
    return(new omx_amr_adec);
}
bool omx_amr_adec::omx_cmd_queue::get_msg_id(unsigned *id)
{
   if(m_size > 0)
   {
       *id = m_q[m_read].id;
       DEBUG_PRINT("get_msg_id=%d\n",*id);
   }
   else{
       return false;
   }
   return true;
}
void omx_amr_adec::process_event_cb(void *client_data, unsigned char id)
{
    (void)id;
    (void)client_data;
    DEBUG_PRINT("PE:Waiting for event's...");
    omx_amr_adec  *pThis = (omx_amr_adec *) client_data;
    pThis->process_events(pThis);
}
void omx_amr_adec::process_events(omx_amr_adec *client_data)
{
    OMX_STATETYPE          state;
    struct msm_audio_event tcxo_event;
    int rc = 0;
    (void)client_data;
    // This loop waits indefintely till an EVENT is recieved.
    while(1)
    {
        rc = ioctl(m_drv_fd,AUDIO_GET_EVENT,&tcxo_event);
        DEBUG_PRINT("PE: Event Thread %d errno=%d",rc,errno);
        if((rc == -1) )
        {
            DEBUG_PRINT("PE:Event Thread exiting %d",rc);
            return;
        }
        get_state(&m_cmp, &state);
        if((state != OMX_StatePause) ||
           (suspensionPolicy != OMX_SuspensionEnabled))
        {
            DEBUG_PRINT("PE:Ignoring Event state[%d] suspension_policy[%d]\
                            event[%d] ",state, suspensionPolicy,tcxo_event.event_type);
            // Ignore events if not Pause state;
            continue;
        }
        pthread_mutex_lock(&m_suspendresume_lock);
        if(getSuspendFlg() && getResumeFlg())
        {
            DEBUG_PRINT("PE:Ignoring Event Already in Suspended state[%d] suspension_policy[%d] \
                         event[%d] suspendflg[%d] resumeflg[%d]\n",state, suspensionPolicy,
                         tcxo_event.event_type,getSuspendFlg(),getResumeFlg());
            pthread_mutex_unlock(&m_suspendresume_lock);
            // Ignore events if not Pause state;
            continue;
        }
        pthread_mutex_unlock(&m_suspendresume_lock);
        DEBUG_PRINT("PE:state[%d] suspensionpolicy[%d] event[%d]",state,
                     suspensionPolicy, tcxo_event.event_type);
        switch(tcxo_event.event_type)
        {
            case AUDIO_EVENT_SUSPEND:
            {
                DEBUG_PRINT("PE: Recieved AUDIO_EVENT_SUSPEND");
                m_timer->stopTimer();

                if(getSuspendFlg())
                {
                    DEBUG_PRINT("PE:Suspend event already in process\n");
                    break;
                }
                post_command(0,0,OMX_COMPONENT_SUSPEND);
            }
            break;
            case AUDIO_EVENT_RESUME:
            {
                if ( getSuspendFlg() && !getResumeFlg() )
                {
                    DEBUG_PRINT("PE: Recieved AUDIO_EVENT_RESUME");
                    // signal the output thread that RESUME has happened
                    post_command(0,0,OMX_COMPONENT_RESUME);
                }
            }
            break;
            default:
                DEBUG_PRINT("PE: Recieved Invalid Event");
                break;
        }
    }
    return;
}
/*=============================================================================
FUNCTION:
  wait_for_event

DESCRIPTION:
  waits for a particular event

INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE:
  None

Dependency:
  None

SIDE EFFECTS:
   None
=============================================================================*/
void omx_amr_adec::wait_for_event()
{
    pthread_mutex_lock(&m_event_lock);
    while ( m_is_event_done == 0 )
    {
        pthread_cond_wait(&cond, &m_event_lock);
    }
    m_is_event_done = 0;
    pthread_mutex_unlock(&m_event_lock);
}

/*=============================================================================
FUNCTION:
  event_complete

DESCRIPTION:
  informs about the occurance of an event

INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE:
  None

Dependency:
  None

SIDE EFFECTS:
   None
=============================================================================*/
void omx_amr_adec::event_complete()
{
    pthread_mutex_lock(&m_event_lock);
    if ( 0 == m_is_event_done )
    {
        m_is_event_done = 1;
        pthread_cond_signal(&cond);
    }
    pthread_mutex_unlock(&m_event_lock);
}

// All this non-sense because of a single amr object
void omx_amr_adec::in_th_goto_sleep()
{
    pthread_mutex_lock(&m_in_th_lock);
    while ( m_is_in_th_sleep == 0 )
    {
        pthread_cond_wait(&in_cond, &m_in_th_lock);
    }
    m_is_in_th_sleep = 0;
    pthread_mutex_unlock(&m_in_th_lock);
}

void omx_amr_adec::in_th_wakeup()
{
    pthread_mutex_lock(&m_in_th_lock);
    if ( 0 == m_is_in_th_sleep )
    {
        m_is_in_th_sleep = 1;
        pthread_cond_signal(&in_cond);
    }
    pthread_mutex_unlock(&m_in_th_lock);
}

void omx_amr_adec::out_th_goto_sleep()
{

    pthread_mutex_lock(&m_out_th_lock);
    while ( m_is_out_th_sleep == 0 )
    {
        pthread_cond_wait(&out_cond, &m_out_th_lock);
    }
    m_is_out_th_sleep = 0;
    pthread_mutex_unlock(&m_out_th_lock);
}

void omx_amr_adec::out_th_wakeup()
{
    pthread_mutex_lock(&m_out_th_lock);
    if ( 0 == m_is_out_th_sleep )
    {
        m_is_out_th_sleep = 1;
        pthread_cond_signal(&out_cond);
    }
    pthread_mutex_unlock(&m_out_th_lock);
}

/*=============================================================================
FUNCTION:
  omx_amr_adec

DESCRIPTION:
  Constructor

INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE:
  None

Dependency:
  None

SIDE EFFECTS:
   None
=============================================================================*/
omx_amr_adec::omx_amr_adec(): m_timer(NULL),
                              m_tmp_meta_buf(NULL),
			      m_tmp_out_meta_buf(NULL),
                              m_flush_cnt(255),
			      m_comp_deinit(0),
			      m_trans_buffer_start(NULL),
                              m_residual_buffer_start(NULL),
			      m_bufMgr(NULL),
                              m_is_alloc_buf(0),
                              m_eos_bm(0),
			      m_residual_data_len(0),
			      m_app_data(NULL),
                              m_drv_fd(-1),
			      bFlushinprogress(0),
                              is_in_th_sleep(false),
                              is_out_th_sleep(false),
			      m_pause_to_exe(false),
			      waitForSuspendCmplFlg(false),
			      m_flags(0),
			      nTimestamp(0),
                              output_buffer_size(OMX_AMR_OUTPUT_BUFFER_SIZE),
			      m_inp_act_buf_count (OMX_CORE_NUM_INPUT_BUFFERS),
                              m_out_act_buf_count (OMX_CORE_NUM_OUTPUT_BUFFERS),
                              m_inp_current_buf_count(0),
                              m_out_current_buf_count(0),
                              input_buffer_size(OMX_CORE_INPUT_BUFFER_SIZE),
			      frameLength(0),
                              frameSize(0),
                              frameType(0),
                              amrwb(0),
			      m_first(0),
                              m_inp_bEnabled(OMX_TRUE),
                              m_out_bEnabled(OMX_TRUE),
                              m_inp_bPopulated(OMX_FALSE),
                              m_out_bPopulated(OMX_FALSE),
			      m_is_event_done(0),
                              m_state(OMX_StateInvalid),
                              m_ipc_to_in_th(NULL),
                              m_ipc_to_out_th(NULL),
                              m_ipc_to_cmd_th(NULL),
                              m_ipc_to_event_th(0),
                              mime_type(NULL)
{
    int cond_ret = 0;
    memset(&m_cmp,       0,     sizeof(m_cmp));
    memset(&m_cb,        0,      sizeof(m_cb));

    pthread_mutexattr_init(&m_lock_attr);
    pthread_mutex_init(&m_lock, &m_lock_attr);
    pthread_mutexattr_init(&m_commandlock_attr);
    pthread_mutex_init(&m_commandlock, &m_commandlock_attr);

    pthread_mutexattr_init(&m_outputlock_attr);
    pthread_mutex_init(&m_outputlock, &m_outputlock_attr);

    pthread_mutexattr_init(&m_state_attr);
    pthread_mutex_init(&m_state_lock, &m_state_attr);

    pthread_mutexattr_init(&m_event_attr);
    pthread_mutex_init(&m_event_lock, &m_event_attr);

    pthread_mutexattr_init(&m_flush_attr);
    pthread_mutex_init(&m_flush_lock, &m_flush_attr);

    pthread_mutexattr_init(&m_event_attr);
    pthread_mutex_init(&m_event_lock, &m_event_attr);

    pthread_mutexattr_init(&m_in_th_attr);
    pthread_mutex_init(&m_in_th_lock, &m_in_th_attr);

    pthread_mutexattr_init(&m_out_th_attr);
    pthread_mutex_init(&m_out_th_lock, &m_out_th_attr);

    pthread_mutexattr_init(&m_in_th_attr_1);
    pthread_mutex_init(&m_in_th_lock_1, &m_in_th_attr_1);

    pthread_mutexattr_init(&m_out_th_attr_1);
    pthread_mutex_init(&m_out_th_lock_1, &m_out_th_attr_1);
    pthread_mutexattr_init(&out_buf_count_lock_attr);
    pthread_mutex_init(&out_buf_count_lock, &out_buf_count_lock_attr);
    pthread_mutexattr_init(&in_buf_count_lock_attr);
    pthread_mutex_init(&in_buf_count_lock, &in_buf_count_lock_attr);
    pthread_mutexattr_init(&m_suspendresume_lock_attr);
    pthread_mutex_init(&m_suspendresume_lock, &m_suspendresume_lock_attr);
    pthread_mutexattr_init(&m_WaitForSuspendCmpl_lock_attr);
    pthread_mutex_init(&m_WaitForSuspendCmpl_lock, &m_WaitForSuspendCmpl_lock_attr);
    if ((cond_ret = pthread_cond_init (&cond, NULL)) != 0)
    {
       DEBUG_PRINT_ERROR("pthread_cond_init returns non zero for cond\n");
       if (cond_ret == EAGAIN)
         DEBUG_PRINT_ERROR("The system lacked necessary resources(other than mem)\n");
       else if (cond_ret == ENOMEM)
          DEBUG_PRINT_ERROR("Insufficient memory to initialise condition variable\n");
    }
    if ((cond_ret = pthread_cond_init (&in_cond, NULL)) != 0)
    {
       DEBUG_PRINT_ERROR("pthread_cond_init returns non zero for in_cond\n");
       if (cond_ret == EAGAIN)
         DEBUG_PRINT_ERROR("The system lacked necessary resources(other than mem)\n");
       else if (cond_ret == ENOMEM)
          DEBUG_PRINT_ERROR("Insufficient memory to initialise condition variable\n");
    }
    if ((cond_ret = pthread_cond_init (&out_cond, NULL)) != 0)
    {
       DEBUG_PRINT_ERROR("pthread_cond_init returns non zero for out_cond\n");
       if (cond_ret == EAGAIN)
         DEBUG_PRINT_ERROR("The system lacked necessary resources(other than mem)\n");
       else if (cond_ret == ENOMEM)
          DEBUG_PRINT_ERROR("Insufficient memory to initialise condition variable\n");
    }

    sem_init(&sem_States,0, 0);
    sem_init(&sem_read_msg,0, 0);
    sem_init(&sem_write_msg,0, 0);
    sem_init(&sem_States,0, 0);
    sem_init(&sem_WaitForSuspendCmpl_states,0, 0);
    m_timer = new timer(this);
    if(m_timer == NULL)
    {
        DEBUG_PRINT_ERROR("Not able to allocate memory for timer obj\n");
    }
    m_bufMgr = new omxBufMgr;
    if(m_bufMgr == NULL)
    {
        DEBUG_PRINT_ERROR("Not able to allocate memory for Buffer Manager\n");
    }
    return;
}


/* ======================================================================
FUNCTION
  omx_amr_adec::~omx_amr_adec

DESCRIPTION
  Destructor

PARAMETERS
  None

RETURN VALUE
  None.
========================================================================== */
omx_amr_adec::~omx_amr_adec()
{
    DEBUG_PRINT("INSIDE DESTRUCTOR comp-deinit=%d\n",m_comp_deinit);

    if ( !m_comp_deinit )
    {
        deinit_decoder();
    }
    if (mime_type)
    {
         free(mime_type);
         mime_type = NULL;
    }
    pthread_mutexattr_destroy(&m_lock_attr);
    pthread_mutex_destroy(&m_lock);
    pthread_mutexattr_destroy(&m_suspendresume_lock_attr);
    pthread_mutex_destroy(&m_suspendresume_lock);

    pthread_mutexattr_destroy(&m_commandlock_attr);
    pthread_mutex_destroy(&m_commandlock);

    pthread_mutexattr_destroy(&m_outputlock_attr);
    pthread_mutex_destroy(&m_outputlock);

    pthread_mutexattr_destroy(&m_state_attr);
    pthread_mutex_destroy(&m_state_lock);

    pthread_mutexattr_destroy(&m_event_attr);
    pthread_mutex_destroy(&m_event_lock);

    pthread_mutexattr_destroy(&m_flush_attr);
    pthread_mutex_destroy(&m_flush_lock);

    pthread_mutexattr_destroy(&m_in_th_attr);
    pthread_mutex_destroy(&m_in_th_lock);

    pthread_mutexattr_destroy(&m_out_th_attr);
    pthread_mutex_destroy(&m_out_th_lock);

    pthread_mutexattr_destroy(&m_in_th_attr_1);
    pthread_mutex_destroy(&m_in_th_lock_1);

    pthread_mutexattr_destroy(&m_out_th_attr_1);
    pthread_mutex_destroy(&m_out_th_lock_1);

    pthread_mutex_destroy(&out_buf_count_lock);
    pthread_mutex_destroy(&in_buf_count_lock);
    pthread_mutex_destroy(&m_WaitForSuspendCmpl_lock);
    pthread_mutexattr_destroy(&m_WaitForSuspendCmpl_lock_attr);
    pthread_mutexattr_destroy(&out_buf_count_lock_attr);
    pthread_mutexattr_destroy(&in_buf_count_lock_attr);
    pthread_cond_destroy(&cond);
    pthread_cond_destroy(&in_cond);
    pthread_cond_destroy(&out_cond);
    sem_destroy (&sem_read_msg);
    sem_destroy (&sem_write_msg);
    sem_destroy (&sem_States);
    sem_destroy (&sem_WaitForSuspendCmpl_states);
DEBUG_PRINT_ERROR("AMRWB component destroyed\n");
    return;
}

/**
  @brief memory function for sending EmptyBufferDone event
   back to IL client

  @param bufHdr OMX buffer header to be passed back to IL client
  @return none
 */
void omx_amr_adec::buffer_done_cb(OMX_BUFFERHEADERTYPE *bufHdr)
{
    if ( m_cb.EmptyBufferDone )
    {
        PrintFrameHdr(OMX_COMPONENT_GENERATE_BUFFER_DONE,bufHdr);
        bufHdr->nFilledLen = 0;

        m_cb.EmptyBufferDone(&m_cmp, m_app_data, bufHdr);
        pthread_mutex_lock(&in_buf_count_lock);
        m_amr_pb_stats.ebd_cnt++;
        nNumInputBuf--;
        DEBUG_DETAIL("EBD CB:: in_buf_len=%lu nNumInputBuf=%d m_amr_pb_stats.ebd_cnt=%lu\n",\
                     m_amr_pb_stats.tot_in_buf_len,
                     nNumInputBuf, m_amr_pb_stats.ebd_cnt);
        pthread_mutex_unlock(&in_buf_count_lock);
    }

    return;
}

/*=============================================================================
FUNCTION:
  flush_ack

DESCRIPTION:


INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE:
  None

Dependency:
  None

SIDE EFFECTS:
  None
=============================================================================*/
void omx_amr_adec::flush_ack()
{
    // Decrement the FLUSH ACK count and notify the waiting recepients
    pthread_mutex_lock(&m_flush_lock);
    --m_flush_cnt;
    if ( 0 ==  m_flush_cnt )
    {
        event_complete();
    }
    DEBUG_PRINT("Rxed FLUSH ACK cnt=%d\n",m_flush_cnt);
    pthread_mutex_unlock(&m_flush_lock);
}
void omx_amr_adec::frame_done_cb(OMX_BUFFERHEADERTYPE *bufHdr)
{
    if ( m_cb.FillBufferDone )
    {
        PrintFrameHdr(OMX_COMPONENT_GENERATE_FRAME_DONE,bufHdr);
        m_amr_pb_stats.fbd_cnt++;
        pthread_mutex_lock(&out_buf_count_lock);
        nNumOutputBuf--;
        DEBUG_PRINT("FBD CB:: nNumOutputBuf=%d out_buf_len=%lu\n bd_cnt = %lu",\
                    nNumOutputBuf,
                    m_amr_pb_stats.tot_out_buf_len,
                    m_amr_pb_stats.fbd_cnt);
        m_amr_pb_stats.tot_out_buf_len += bufHdr->nFilledLen;
        m_amr_pb_stats.tot_pb_time     = bufHdr->nTimeStamp;
        DEBUG_PRINT("FBD:in_buf_len=%lu out_buf_len=%lu\n",
                    m_amr_pb_stats.tot_in_buf_len,
                    m_amr_pb_stats.tot_out_buf_len);
        pthread_mutex_unlock(&out_buf_count_lock);
        m_cb.FillBufferDone(&m_cmp, m_app_data, bufHdr);
    }
    return;
}

/*=============================================================================
FUNCTION:
  process_out_port_msg

DESCRIPTION:
  Function for handling all commands from IL client
IL client commands are processed and callbacks are generated through
this routine  Audio Command Server provides the thread context for this routine

INPUT/OUTPUT PARAMETERS:
  [INOUT] client_data
  [IN] id

RETURN VALUE:
  None

Dependency:
  None

SIDE EFFECTS:
  None
=============================================================================*/
void omx_amr_adec::process_out_port_msg(void *client_data, unsigned char id)
{
    unsigned      p1; // Parameter - 1
    unsigned      p2; // Parameter - 2
    unsigned      ident;
    unsigned      qsize     = 0; // qsize
    unsigned      tot_qsize = 0;
    omx_amr_adec  *pThis    = (omx_amr_adec *) client_data;
    OMX_STATETYPE state;

loopback_out:
    pthread_mutex_lock(&pThis->m_state_lock);
    pThis->get_state(&pThis->m_cmp, &state);
    pthread_mutex_unlock(&pThis->m_state_lock);
    if ( state == OMX_StateLoaded )
    {
        DEBUG_PRINT(" OUT: IN LOADED STATE RETURN\n");
        return;
    }
    pthread_mutex_lock(&pThis->m_outputlock);
    qsize = pThis->m_output_ctrl_cmd_q.m_size;
    tot_qsize = pThis->m_output_ctrl_cmd_q.m_size;
    tot_qsize += pThis->m_output_ctrl_fbd_q.m_size;
    tot_qsize += pThis->m_output_q.m_size;

    if ( 0 == tot_qsize )
    {
        pthread_mutex_unlock(&pThis->m_outputlock);
        DEBUG_DETAIL("OUT-->BREAK FROM LOOP...%d\n",tot_qsize);
        return;
    }
    if ( (state != OMX_StateExecuting) && !qsize )
    {
        pthread_mutex_unlock(&pThis->m_outputlock);
        pthread_mutex_lock(&pThis->m_state_lock);
        pThis->get_state(&pThis->m_cmp, &state);
        pthread_mutex_unlock(&pThis->m_state_lock);
        if ( state == OMX_StateLoaded )
            return;

        DEBUG_DETAIL("OUT:1.SLEEPING OUT THREAD\n");
        pthread_mutex_lock(&pThis->m_out_th_lock_1);
        pThis->is_out_th_sleep = true;
        pthread_mutex_unlock(&pThis->m_out_th_lock_1);
        pThis->out_th_goto_sleep();
        pthread_mutex_lock(&pThis->m_state_lock);
        pThis->get_state(&pThis->m_cmp, &state);
        pthread_mutex_unlock(&pThis->m_state_lock);
    }

    if ( ((!pThis->m_output_ctrl_cmd_q.m_size) && !pThis->m_out_bEnabled) )
    {
        // case where no port reconfig and nothing in the flush q
        DEBUG_DETAIL("No flush/port reconfig qsize=%d tot_qsize=%d",\
                     qsize,tot_qsize);
        pthread_mutex_unlock(&pThis->m_outputlock);
        pthread_mutex_lock(&pThis->m_state_lock);
        pThis->get_state(&pThis->m_cmp, &state);
        pthread_mutex_unlock(&pThis->m_state_lock);
        if ( state == OMX_StateLoaded )
            return;

        if(pThis->m_output_ctrl_cmd_q.m_size || !(pThis->bFlushinprogress))
        {
        DEBUG_PRINT("OUT:2. SLEEPING OUT THREAD \n");
        pthread_mutex_lock(&pThis->m_out_th_lock_1);
        pThis->is_out_th_sleep = true;
        pthread_mutex_unlock(&pThis->m_out_th_lock_1);
        pThis->out_th_goto_sleep();
        }
        /* Get the updated state */
        pthread_mutex_lock(&pThis->m_state_lock);
        pThis->get_state(&pThis->m_cmp, &state);
        pthread_mutex_unlock(&pThis->m_state_lock);
    }
    else if(state == OMX_StatePause)
    {
        DEBUG_PRINT ("\n OUT Thread in the pause state");
        if(!(pThis->m_output_ctrl_cmd_q.m_size))
        {

           // to take care of some race condition, where the output
           // thread is working on the old state;
           pThis->get_state(&pThis->m_cmp, &state);
           DEBUG_PRINT("OUT: pause state =%d m_pause_to_exe=%d\n",state,pThis->m_pause_to_exe);


            if (state == OMX_StatePause && !pThis->m_pause_to_exe )
            {

                DEBUG_DETAIL("OUT: SLEEPING OUT THREAD\n");
                pthread_mutex_lock(&pThis->m_out_th_lock_1);
                pThis->is_out_th_sleep = true;
                pthread_mutex_unlock(&pThis->m_out_th_lock_1);
                pThis->out_th_goto_sleep();
            }
            else
              DEBUG_PRINT("OUT--> In pause if() check, but now state changed\n");
        }
    }

    qsize = pThis->m_output_ctrl_cmd_q.m_size;
    tot_qsize = pThis->m_output_ctrl_cmd_q.m_size;
    tot_qsize += pThis->m_output_ctrl_fbd_q.m_size;
    tot_qsize += pThis->m_output_q.m_size;
    pthread_mutex_lock(&pThis->m_state_lock);
    pThis->get_state(&pThis->m_cmp, &state);
    pthread_mutex_unlock(&pThis->m_state_lock);
    DEBUG_DETAIL("OUT-->QSIZE-flush=%d,fbd=%d QSIZE=%d state=%d\n",\
                 pThis->m_output_ctrl_cmd_q.m_size,
                 pThis->m_output_ctrl_fbd_q.m_size,
                 pThis->m_output_q.m_size,state);


    if ( qsize )
    {
        // process FLUSH message
        pThis->m_output_ctrl_cmd_q.pop_entry(&p1,&p2,&ident);
    }
    else if ( (qsize = pThis->m_output_ctrl_fbd_q.m_size) &&
              (pThis->m_out_bEnabled) && (state == OMX_StateExecuting) )
    {
        // then process EBD's
        pThis->m_output_ctrl_fbd_q.pop_entry(&p1,&p2,&ident);
    }
    else if ( (qsize = pThis->m_output_q.m_size) &&
              (pThis->m_out_bEnabled) && (state == OMX_StateExecuting) )
    {
        // if no FLUSH and FBD's then process FTB's
        pThis->m_output_q.pop_entry(&p1,&p2,&ident);
    }
    else if ( state == OMX_StateLoaded )
    {
        pthread_mutex_unlock(&pThis->m_outputlock);
        DEBUG_PRINT("IN: ***in OMX_StateLoaded so exiting\n");
        return ;
    }
    else
    {
        qsize = 0;
        DEBUG_PRINT("OUT--> Empty Queue state=%d %d %d %d\n",state,
                     pThis->m_output_ctrl_cmd_q.m_size,pThis->m_output_ctrl_fbd_q.m_size,
                     pThis->m_output_q.m_size);

        if(state == OMX_StatePause)
        {
            DEBUG_DETAIL("OUT: SLEEPING AGAIN OUT THREAD\n");
            pthread_mutex_lock(&pThis->m_out_th_lock_1);
            pThis->is_out_th_sleep = true;
            pthread_mutex_unlock(&pThis->m_out_th_lock_1);
            pthread_mutex_unlock(&pThis->m_outputlock);
            pThis->out_th_goto_sleep();
            goto loopback_out;
        }
    }
    pthread_mutex_unlock(&pThis->m_outputlock);

    if ( qsize > 0 )
    {
        id = ident;
        ident = 0;
        DEBUG_DETAIL("OUT->state[%d]ident[%d]flushq[%d]fbd[%d]dataq[%d]\n",\
                     pThis->m_state,
                     ident,
                     pThis->m_output_ctrl_cmd_q.m_size,
                     pThis->m_output_ctrl_fbd_q.m_size,
                     pThis->m_output_q.m_size);

        if ( OMX_COMPONENT_GENERATE_FRAME_DONE == id )
        {
            pThis->frame_done_cb((OMX_BUFFERHEADERTYPE *)p2);
        }
        else if ( OMX_COMPONENT_GENERATE_FTB == id )
        {
            pThis->fill_this_buffer_proxy((OMX_HANDLETYPE)p1,
                                          (OMX_BUFFERHEADERTYPE *)p2);
        }
        else if ( OMX_COMPONENT_GENERATE_EOS == id )
        {
            pThis->m_cb.EventHandler(&pThis->m_cmp,
                                     pThis->m_app_data,
                                     OMX_EventBufferFlag,
                                     1, 1, NULL );
        }
        else if(id == OMX_COMPONENT_SUSPEND)
        {
            DEBUG_PRINT("OUT: Rxed SUSPEND event_eos_flag=%d\n",pThis->m_eos_bm);
            if( pThis->m_eos_bm != IP_OP_PORT_BITMASK)
            {
               pThis->append_data_to_temp_buf();
            }
        }
        else if(id == OMX_COMPONENT_RESUME)
        {
            DEBUG_PRINT("RESUMED...\n");
        }
        else if ( OMX_COMPONENT_GENERATE_COMMAND == id )
        {
            // Execute FLUSH command
            if ( OMX_CommandFlush == p1 )
            {
                DEBUG_DETAIL("Executing FLUSH command on Output port\n");
                pThis->execute_output_omx_flush();
            }
            else
            {
                DEBUG_DETAIL("Invalid command[%d]\n",p1);
            }
        }
        else
        {
            DEBUG_PRINT_ERROR("ERROR:OUT-->Invalid Id[%d]\n",id);
        }
    }
    else
    {
        DEBUG_DETAIL("ERROR: OUT--> Empty OUTPUTQ\n");
    }

    return;
}

/*=============================================================================
FUNCTION:
  process_command_msg

DESCRIPTION:


INPUT/OUTPUT PARAMETERS:
  [INOUT] client_data
  [IN] id

RETURN VALUE:
  None

Dependency:
  None

SIDE EFFECTS:
  None
=============================================================================*/
void omx_amr_adec::process_command_msg(void *client_data, unsigned char id)
{
    unsigned     p1;         // Parameter - 1
    unsigned     p2;         // Parameter - 2
    unsigned     ident;
    unsigned     qsize  = 0;
    omx_amr_adec *pThis = (omx_amr_adec*)client_data;

    pthread_mutex_lock(&pThis->m_commandlock);

    qsize = pThis->m_command_q.m_size;
    DEBUG_DETAIL("CMD-->QSIZE=%d state=%d\n",pThis->m_command_q.m_size,
                 pThis->m_state);

    if ( !qsize )
    {
        DEBUG_DETAIL("CMD-->BREAKING FROM LOOP\n");
        pthread_mutex_unlock(&pThis->m_commandlock);
        return;
    }
    else
    {
        pThis->m_command_q.pop_entry(&p1,&p2,&ident);
    }
    pthread_mutex_unlock(&pThis->m_commandlock);

    id = ident;
    DEBUG_DETAIL("CMD->state[%d]id[%d]cmdq[%d]n",\
                 pThis->m_state,ident, \
                 pThis->m_command_q.m_size);

    if ( OMX_COMPONENT_GENERATE_EVENT == id )
    {
        if ( pThis->m_cb.EventHandler )
        {
            if ( OMX_CommandStateSet == p1 )
            {
                pthread_mutex_lock(&pThis->m_state_lock);
                pThis->m_state = (OMX_STATETYPE) p2;
                pthread_mutex_unlock(&pThis->m_state_lock);
                DEBUG_PRINT("CMD:Process->state set to %d \n", \
                            pThis->m_state);

                if ( pThis->m_state == OMX_StateExecuting ||
                     pThis->m_state == OMX_StateLoaded )
                {
                    pthread_mutex_lock(&pThis->m_in_th_lock_1);
                    if ( pThis->is_in_th_sleep )
                    {
                        pThis->is_in_th_sleep = false;
                        DEBUG_DETAIL("CMD:WAKING UP IN THREADS\n");
                        pThis->in_th_wakeup();
                    }
                    pthread_mutex_unlock(&pThis->m_in_th_lock_1);

                    pthread_mutex_lock(&pThis->m_out_th_lock_1);
                    if ( pThis->is_out_th_sleep )
                    {
                        DEBUG_DETAIL("CMD:WAKING UP OUT THREADS\n");
                        pThis->is_out_th_sleep = false;
                        pThis->out_th_wakeup();
                    }
                    pthread_mutex_unlock(&pThis->m_out_th_lock_1);
                    if((pThis->m_state == OMX_StateExecuting))
                      pThis->m_pause_to_exe=false;
                }
            }
            if ( OMX_StateInvalid == pThis->m_state )
            {
                pThis->m_cb.EventHandler(&pThis->m_cmp,
                                         pThis->m_app_data,
                                         OMX_EventError,
                                         OMX_ErrorInvalidState,
                                         0, NULL );
            }
            else if ( p2 == (unsigned)OMX_ErrorPortUnpopulated )
            {
                pThis->m_cb.EventHandler(&pThis->m_cmp,
                                         pThis->m_app_data,
                                         OMX_EventError,
                                         p2,
                                         NULL,
                                         NULL );
            }
            else
            {
                pThis->m_cb.EventHandler(&pThis->m_cmp,
                                         pThis->m_app_data,
                                         OMX_EventCmdComplete,
                                         p1, p2, NULL );
            }
        }
        else
        {
            DEBUG_PRINT_ERROR("ERROR:CMD-->EventHandler NULL \n");
        }
    }
    else if ( OMX_COMPONENT_GENERATE_COMMAND == id )
    {
        pThis->send_command_proxy(&pThis->m_cmp,
                                  (OMX_COMMANDTYPE)p1,
                                  (OMX_U32)p2,(OMX_PTR)NULL);
    }
    else if ( OMX_COMPONENT_PORTSETTINGS_CHANGED == id )
    {
        DEBUG_DETAIL("CMD-->RXED PORTSETTINGS_CHANGED");
        pThis->m_cb.EventHandler(&pThis->m_cmp,
                                 pThis->m_app_data,
                                 OMX_EventPortSettingsChanged,
                                 1, 1, NULL );
    }
    else if(OMX_COMPONENT_SUSPEND == id)
    {
        pthread_mutex_lock(&pThis->m_suspendresume_lock);
        pThis->setSuspendFlg();
        pthread_mutex_unlock(&pThis->m_suspendresume_lock);
        DEBUG_PRINT("CMD-->Suspend event rxed suspendflag=%d \n",\
                            pThis->getSuspendFlg());

        // signal the output thread to process suspend
        pThis->post_output(0,0,OMX_COMPONENT_SUSPEND);
        //signal the input thread to process suspend
        pThis->post_input(0,0,OMX_COMPONENT_SUSPEND);
        pthread_mutex_lock(&pThis->m_in_th_lock_1);
        if(pThis->is_in_th_sleep)
        {
                DEBUG_DETAIL("CMD:WAKING UP IN THREADS\n");
                pThis->in_th_wakeup();
                pThis->is_in_th_sleep = false;
        }
        pthread_mutex_unlock(&pThis->m_in_th_lock_1);
        pthread_mutex_lock(&pThis->m_out_th_lock_1);
        if(pThis->is_out_th_sleep)
        {
            DEBUG_DETAIL("CMD:WAKING UP OUT THREADS\n");
            pThis->out_th_wakeup();
            pThis->is_out_th_sleep = false;
        }
        pthread_mutex_unlock(&pThis->m_out_th_lock_1);
    }
    else if(id == OMX_COMPONENT_RESUME)
    {
        // signal the output thread that RESUME has happened
        pthread_mutex_lock(&pThis->m_suspendresume_lock);
        pThis->setResumeFlg();
        pthread_mutex_unlock(&pThis->m_suspendresume_lock);
        pThis->post_output(0,0,OMX_COMPONENT_RESUME);
        if (pThis->getWaitForSuspendCmplFlg())
        {
            DEBUG_PRINT_ERROR("Release P-->Executing context to IL client.\n");
            pThis->release_wait_for_suspend();
        }

        pthread_mutex_lock(&pThis->m_out_th_lock_1);
        if(pThis->is_out_th_sleep)
        {
            DEBUG_DETAIL("CMD:WAKING UP OUT THREADS\n");
            pThis->out_th_wakeup();
            pThis->is_out_th_sleep = false;
        }
        pthread_mutex_unlock(&pThis->m_out_th_lock_1);
    }
    else
    {
       DEBUG_PRINT_ERROR("CMD->state[%d]id[%d]\n",pThis->m_state,ident);
    }
    return;
}

/*=============================================================================
FUNCTION:
  process_in_port_msg

DESCRIPTION:


INPUT/OUTPUT PARAMETERS:
  [INOUT] client_data
  [IN] id

RETURN VALUE:
  None

Dependency:
  None

SIDE EFFECTS:
  None
=============================================================================*/
void omx_amr_adec::process_in_port_msg(void *client_data, unsigned char id)
{
    unsigned      p1;       // Parameter - 1
    unsigned      p2;       // Parameter - 2
    unsigned      ident;
    unsigned      qsize     = 0;
    unsigned      tot_qsize = 0;
    omx_amr_adec  *pThis    = (omx_amr_adec *) client_data;
    OMX_STATETYPE state;

    if ( !pThis )
    {
        DEBUG_PRINT_ERROR("ERROR:IN--> Invalid Obj \n");
        return;
    }
loopback_in:
    pthread_mutex_lock(&pThis->m_state_lock);
    pThis->get_state(&pThis->m_cmp, &state);
    pthread_mutex_unlock(&pThis->m_state_lock);

    if ( state == OMX_StateLoaded )
    {
        DEBUG_PRINT(" IN: IN LOADED STATE RETURN\n");
        return;
    }
    // Protect the shared queue data structure
    pthread_mutex_lock(&pThis->m_lock);
    qsize = pThis->m_input_ctrl_cmd_q.m_size;
    tot_qsize = qsize;
    tot_qsize += pThis->m_input_ctrl_ebd_q.m_size;
    tot_qsize += pThis->m_input_q.m_size;

    if ( 0 == tot_qsize )
    {
        DEBUG_DETAIL("IN-->BREAKING FROM IN LOOP");
        pthread_mutex_unlock(&pThis->m_lock);
        return;
    }

    if ( (state != OMX_StateExecuting) && ! (pThis->m_input_ctrl_cmd_q.m_size))
    {
        pthread_mutex_unlock(&pThis->m_lock);
        DEBUG_DETAIL("SLEEPING IN THREAD\n");
        pthread_mutex_lock(&pThis->m_in_th_lock_1);
        pThis->is_in_th_sleep = true;
        pthread_mutex_unlock(&pThis->m_in_th_lock_1);
        pThis->in_th_goto_sleep();
        /* Read the state, it might have been modfied by the client */
        pthread_mutex_lock(&pThis->m_state_lock);
        pThis->get_state(&pThis->m_cmp, &state);
        pthread_mutex_unlock(&pThis->m_state_lock);
    }
    else if ((state == OMX_StatePause))
    {
        if(!(pThis->m_input_ctrl_cmd_q.m_size))
        {
           pthread_mutex_unlock(&pThis->m_lock);

           DEBUG_DETAIL("IN: SLEEPING IN THREAD\n");
           pthread_mutex_lock(&pThis->m_in_th_lock_1);
           pThis->is_in_th_sleep = true;
           pthread_mutex_unlock(&pThis->m_in_th_lock_1);
           pThis->in_th_goto_sleep();

           pthread_mutex_lock(&pThis->m_state_lock);
           pThis->get_state(&pThis->m_cmp, &state);
           pthread_mutex_unlock(&pThis->m_state_lock);
        }
    }

    qsize = pThis->m_input_ctrl_cmd_q.m_size;
    tot_qsize = qsize;
    tot_qsize += pThis->m_input_ctrl_ebd_q.m_size;
    tot_qsize += pThis->m_input_q.m_size;

    DEBUG_DETAIL("Input-->QSIZE-flush=%d,ebd=%d QSIZE=%d state=%d\n",\
                 pThis->m_input_ctrl_cmd_q.m_size,
                 pThis->m_input_ctrl_ebd_q.m_size,
                 pThis->m_input_q.m_size, state);



    if ( qsize )
    {
        // process FLUSH message
        pThis->m_input_ctrl_cmd_q.pop_entry(&p1,&p2,&ident);
    }
    else if ( (qsize = pThis->m_input_ctrl_ebd_q.m_size) &&
              (state == OMX_StateExecuting) )
    {
        // then process EBD's
        pThis->m_input_ctrl_ebd_q.pop_entry(&p1,&p2,&ident);
    }
    else if ( (qsize = pThis->m_input_q.m_size) &&
              (state == OMX_StateExecuting) && !(pThis->m_bufMgr->getBufFilledSpace()))
    {
        // if no FLUSH and EBD's then process ETB's
        pThis->m_input_q.pop_entry(&p1, &p2, &ident);
    }
    else if ( state == OMX_StateLoaded )
    {
        pthread_mutex_unlock(&pThis->m_lock);
        DEBUG_PRINT("IN: ***in OMX_StateLoaded so exiting\n");
        return ;
    }
    else
    {
        qsize = 0;
        DEBUG_PRINT("IN-->state[%d]cmdq[%d]ebdq[%d]in[%d]temp_buf[%lu]\n",\
                             state,pThis->m_input_ctrl_cmd_q.m_size,
                             pThis->m_input_ctrl_ebd_q.m_size,pThis->m_input_q.m_size,
                             pThis->m_bufMgr->getBufFilledSpace());

        if(state == OMX_StatePause || pThis->m_bufMgr->getBufFilledSpace())
        {
            DEBUG_DETAIL("IN: SLEEPING AGAIN IN THREAD\n");
            pthread_mutex_lock(&pThis->m_in_th_lock_1);
            pThis->is_in_th_sleep = true;
            pthread_mutex_unlock(&pThis->m_in_th_lock_1);
            pthread_mutex_unlock(&pThis->m_lock);
            pThis->in_th_goto_sleep();
            goto loopback_in;
        }
    }
    pthread_mutex_unlock(&pThis->m_lock);

    if ( qsize > 0 )
    {
        id = ident;
        DEBUG_DETAIL("Input->state[%d]id[%d]flushq[%d]ebdq[%d]dataq[%d]\n",\
                     pThis->m_state,
                     ident,
                     pThis->m_input_ctrl_cmd_q.m_size,
                     pThis->m_input_ctrl_ebd_q.m_size,
                     pThis->m_input_q.m_size);
        if ( OMX_COMPONENT_GENERATE_BUFFER_DONE == id )
        {
            pThis->buffer_done_cb((OMX_BUFFERHEADERTYPE *)p2);
        }
        else if(id == OMX_COMPONENT_GENERATE_EOS)
        {
                pThis->m_cb.EventHandler(&pThis->m_cmp, pThis->m_app_data,
                                     OMX_EventBufferFlag, 0, 1, NULL );
        }
        else if ( OMX_COMPONENT_GENERATE_ETB == id )
        {
            pThis->empty_this_buffer_proxy((OMX_HANDLETYPE)p1,
                                           (OMX_BUFFERHEADERTYPE *)p2);
        }
        else if ( OMX_COMPONENT_GENERATE_COMMAND == id )
        {
            // Execute FLUSH command
            if ( OMX_CommandFlush == p1 )
            {
                DEBUG_DETAIL(" Executing FLUSH command on Input port\n");
                pThis->execute_input_omx_flush();
            }
            else
            {
                DEBUG_DETAIL("Invalid command[%d]\n",p1);
            }
        }
          else if(id == OMX_COMPONENT_SUSPEND)
          {
              DEBUG_PRINT("IN:FAKING EOS TO KERNEL : m_eos_bm=%d", pThis->m_eos_bm);
              // dont trigger fake EOS when actual EOS has already been sent
              if(!(pThis->m_eos_bm & IP_OP_PORT_BITMASK))
              {
                  pThis->omx_amr_fake_eos();
              }
            else if((pThis->m_eos_bm & IP_OP_PORT_BITMASK) ==
                                                     IP_OP_PORT_BITMASK)
            {
                pThis->setSuspendFlg();
                pThis->setResumeFlg();
                //EOS already reached, dont trigger suspend/resume,
                // but set the flag so that one more suspend/event doesnt
                // get triggered from timeout/driver
                DEBUG_PRINT("IN--> AUDIO_STOP %d %d \n",
                                pThis->getSuspendFlg(),
                                pThis->getResumeFlg());
                ioctl(pThis->m_drv_fd, AUDIO_STOP, 0);
                if (pThis->getWaitForSuspendCmplFlg())
                {
                    DEBUG_PRINT_ERROR("Release P-->Executing context to IL client.\n");
                    pThis->release_wait_for_suspend();
                }
            }
            else
            {
                // suspend event after input eos sent
                DEBUG_PRINT("IN--> Do nothing, m_eos_bm=%d\n",pThis->m_eos_bm);
            }
        }
        else
        {
            DEBUG_PRINT_ERROR("ERROR:IN-->Invalid Id[%d]\n",id);
        }
    } else
    {
        DEBUG_DETAIL("ERROR:IN-->Empty INPUT Q\n");
    }
    return;
}

/**
 @brief member function for performing component initialization

 @param role C string mandating role of this component
 @return Error status
 */
OMX_ERRORTYPE omx_amr_adec::component_init(OMX_STRING role)
{

    OMX_ERRORTYPE eRet = OMX_ErrorNone;

    m_state            = OMX_StateLoaded;
    m_pause_to_exe     = false;
    m_eos_bm         =0x00;
    m_first = 0;
    m_timer->resetTimerExpiry();
    memset(&m_amr_param, 0, sizeof(m_amr_param));
    memset(&m_amr_pb_stats,0,sizeof(AMR_PB_STATS));
    m_amr_param.nSize = sizeof(m_amr_param);
    m_volume = 25;
    m_amr_param.nChannels = AMRWB_DEFAULT_CH;
    m_amr_param.eAMRDTXMode = OMX_AUDIO_AMRDTXModeOnVAD1;
    m_amr_param.eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatFSF;

    memset(&m_amrwbPlus_param, 0, sizeof(m_amrwbPlus_param));
    m_amrwbPlus_param.nSize = sizeof(m_amrwbPlus_param);

    m_amrwbPlus_param.nChannels   = AMRWB_DEFAULT_CH;
    m_amrwbPlus_param.nSampleRate = AMRWB_DEFAULT_SF;

    m_pcm_param.nChannels = AMRWB_DEFAULT_CH;
    m_pcm_param.eNumData = OMX_NumericalDataSigned;
    m_pcm_param.bInterleaved = OMX_TRUE;
    m_pcm_param.nBitPerSample = 16;
    m_pcm_param.nSamplingRate = AMRWB_DEFAULT_SF;
    m_pcm_param.ePCMMode = OMX_AUDIO_PCMModeLinear;
    m_pcm_param.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
    m_pcm_param.eChannelMapping[1] = OMX_AUDIO_ChannelRF;

    nTimestamp = 0;
    DEBUG_PRINT(" Enabling Non-Tunnel mode \n");

    nNumInputBuf = 0;
    nNumOutputBuf = 0;
    m_ipc_to_in_th = NULL;  // Command server instance
    m_ipc_to_out_th = NULL;  // Client server instance
    m_ipc_to_cmd_th = NULL;  // command instance

    m_is_out_th_sleep = 0;
    m_is_in_th_sleep = 0;
    is_out_th_sleep= false;
    suspensionPolicy= OMX_SuspensionDisabled;
    is_in_th_sleep=false;

    resetSuspendFlg();
    resetResumeFlg();;
    fake_eos_recieved = false;
    toc_cnt = 0;

    memset(&m_priority_mgm, 0, sizeof(m_priority_mgm));
    m_priority_mgm.nGroupID =0;
    m_priority_mgm.nGroupPriority=0;

    memset(&m_buffer_supplier, 0, sizeof(m_buffer_supplier));
    m_buffer_supplier.nPortIndex=OMX_BufferSupplyUnspecified;
    mime_type = (OMX_STRING) malloc(sizeof("audio/amrwb"));
    if (mime_type)
    {
        strcpy(mime_type, "audio/amrwb");
        DEBUG_PRINT("MIME type: %s\n", mime_type);
    } else {
        DEBUG_PRINT_ERROR("error allocating mime type string\n");
    }

    DEBUG_PRINT_ERROR(" component init: role = %s\n",role);
    if ( !strcmp(role,"OMX.qcom.audio.decoder.amrwb") )
    {
    amrwb = 1;
        pcm_feedback = 1;
        DEBUG_PRINT("\ncomponent_init: Component %s LOADED \n", role);
    }
    else if ( !strcmp(role,"OMX.qcom.audio.decoder.tunneled.amrwb") )
    {
    amrwb = 1;
        pcm_feedback = 0;
        DEBUG_PRINT("\ncomponent_init: Component %s LOADED \n", role);
    }
    else if ( !strcmp(role,"OMX.qcom.audio.decoder.amrwbp") )
    {
    amrwb = 0;
        pcm_feedback = 1;
        DEBUG_PRINT("\ncomponent_init: Component %s LOADED \n", role);
    }
    else if ( !strcmp(role,"OMX.qcom.audio.decoder.tunneled.amrwbp") )
    {
    amrwb = 0;
        pcm_feedback = 0;
        DEBUG_PRINT("\ncomponent_init: Component %s LOADED \n", role);
    }
    else
    {
        DEBUG_PRINT("\ncomponent_init: Component %s LOADED is invalid\n", role);
    }

    if(0 == pcm_feedback)
    {
        m_drv_fd = open("/dev/msm_amrwb",O_WRONLY);
    }
    else
    {
        m_drv_fd = open("/dev/msm_amrwb",O_RDWR);
    }
    if ( m_drv_fd < 0 )
    {
        DEBUG_PRINT_ERROR("component_init-->Dev Open Failed[%d] errno[%d]",\
                        m_drv_fd,errno);
        return OMX_ErrorInsufficientResources;
    }
    if(ioctl(m_drv_fd, AUDIO_GET_SESSION_ID,&m_session_id) == -1)
    {
    DEBUG_PRINT("AUDIO_GET_SESSION_ID FAILED\n");
    }
    m_trans_buffer_start =
    (OMX_U8 *)malloc(sizeof(OMX_U8) * OMX_AMR_TRANSCODE_BUFFER_SIZE);
    if ( NULL == m_trans_buffer_start )
    {
        return OMX_ErrorInsufficientResources;
    }
    memset(m_trans_buffer_start,
           0, sizeof(OMX_U8) * OMX_AMR_TRANSCODE_BUFFER_SIZE);
    m_trans_buffer = m_trans_buffer_start;

    m_residual_buffer_start =
        (OMX_U8 *)malloc(sizeof(OMX_U8) * RESIDUALDATA_BUFFER_SIZE);
    if ( NULL == m_residual_buffer_start )
    {
        return OMX_ErrorInsufficientResources;
    }
    memset(m_residual_buffer_start,
           0, sizeof(OMX_U8) * RESIDUALDATA_BUFFER_SIZE);
    m_residual_buffer = m_residual_buffer_start;

    if (pcm_feedback)
    {
        m_tmp_out_meta_buf =
            (OMX_U8*)malloc(sizeof(OMX_U8)*
                       (OMX_AMR_OUTPUT_BUFFER_SIZE+ sizeof(META_OUT)));
        if ( m_tmp_out_meta_buf == NULL )
            DEBUG_PRINT("Mem alloc failed for out meta buf\n");
        if(!m_tmp_meta_buf)
        {
            m_tmp_meta_buf =
            (OMX_U8*)malloc( sizeof(OMX_U8)* (OMX_AMR_TRANSCODE_BUFFER_SIZE +
                             sizeof(META_IN)));
        }
        if ( m_tmp_meta_buf == NULL )
        {
            DEBUG_PRINT("Mem alloc failed for in meta buf\n");
        }
    }
    if ( !m_ipc_to_in_th )
    {
        m_ipc_to_in_th =
        omx_amr_thread_create(process_in_port_msg, this,(char*)"INPUT_THREAD");
        if ( !m_ipc_to_in_th )
        {
            DEBUG_PRINT_ERROR("ERROR!!! Failed to start Input port thread\n");
            return OMX_ErrorInsufficientResources;
        }

    }

    if ( !m_ipc_to_cmd_th )
    {
        m_ipc_to_cmd_th =
        omx_amr_thread_create(process_command_msg, this,(char*)"CMD_THREAD");
        if ( !m_ipc_to_cmd_th )
        {
            DEBUG_PRINT_ERROR("ERROR!!!Failed to start command message thread\n");
            return OMX_ErrorInsufficientResources;
        }
    }

    if ( pcm_feedback )
    {
        if ( !m_ipc_to_out_th )
        {
            m_ipc_to_out_th = omx_amr_thread_create(process_out_port_msg,
                                                    this,(char*)"OUTPUT_THREAD");
            if ( !m_ipc_to_out_th )
            {
                DEBUG_PRINT_ERROR("ERROR!!! Failed to start output port thread\n");
                return OMX_ErrorInsufficientResources;
            }
        }
    }
    return eRet;
}

/**

 @brief member function to retrieve version of component



 @param hComp handle to this component instance
 @param componentName name of component
 @param componentVersion  pointer to memory space which stores the
       version number
 @param specVersion pointer to memory sapce which stores version of
        openMax specification
 @param componentUUID
 @return Error status
 */
OMX_ERRORTYPE  omx_amr_adec::get_component_version
(
    OMX_IN OMX_HANDLETYPE               hComp,
    OMX_OUT OMX_STRING          componentName,
    OMX_OUT OMX_VERSIONTYPE* componentVersion,
    OMX_OUT OMX_VERSIONTYPE*      specVersion,
    OMX_OUT OMX_UUIDTYPE*       componentUUID
)
{
    (void)hComp;
    (void)componentName ;
    (void)componentVersion ;
    (void)specVersion ;
    (void)componentUUID;
    if ( OMX_StateInvalid == m_state )
    {
        DEBUG_PRINT_ERROR("Get Comp Version in Invalid State\n");
        return OMX_ErrorInvalidState;
    }
    /* TBD -- Return the proper version */
    return OMX_ErrorNone;
}
/**
  @brief member function handles command from IL client

  This function simply queue up commands from IL client.
  Commands will be processed in command server thread context later

  @param hComp handle to component instance
  @param cmd type of command
  @param param1 parameters associated with the command type
  @param cmdData
  @return Error status
*/
OMX_ERRORTYPE  omx_amr_adec::send_command(OMX_IN OMX_HANDLETYPE hComp,
                                          OMX_IN OMX_COMMANDTYPE  cmd,
                                          OMX_IN OMX_U32       param1,
                                          OMX_IN OMX_PTR      cmdData)
{
    int portIndex = (int)param1;
    if(hComp == NULL)
    {
        cmdData = NULL;
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if ( OMX_StateInvalid == m_state )
    {
        return OMX_ErrorInvalidState;
    }
    if ( (cmd == OMX_CommandFlush) && (portIndex > 1) )
    {
        return OMX_ErrorBadPortIndex;
    }
    if((m_state == OMX_StatePause) )
    {
        DEBUG_PRINT("Send Command-->State=%d cmd=%d param1=%lu sus=%d res=%d\n",
                             m_state,cmd,param1,getSuspendFlg(),getResumeFlg());
        if((cmd == OMX_CommandStateSet) )
        {
            if(getSuspendFlg() && !getResumeFlg())
            {
                DEBUG_PRINT_ERROR("Send Command, waiting for suspend/resume procedure to complete\n");
                wait_for_suspend_cmpl();
            }
        }
    }
    post_command((unsigned)cmd,
                 (unsigned)param1,OMX_COMPONENT_GENERATE_COMMAND);
    DEBUG_PRINT("Send Command : returns with OMX_ErrorNone \n");

    DEBUG_PRINT("send_command : recieved state before semwait= %lu\n",param1);
    sem_wait (&sem_States);
    DEBUG_PRINT("send_command : recieved state after semwait\n");

    return OMX_ErrorNone;
}

/**
 @brief member function performs actual processing of commands excluding
  empty buffer call

 @param hComp handle to component
 @param cmd command type
 @param param1 parameter associated with the command
 @param cmdData

 @return error status
*/


OMX_ERRORTYPE  omx_amr_adec::send_command_proxy(OMX_IN OMX_HANDLETYPE hComp,
                                                OMX_IN OMX_COMMANDTYPE  cmd,
                                                OMX_IN OMX_U32       param1,
                                                OMX_IN OMX_PTR      cmdData)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    //   Handle only IDLE and executing
    OMX_STATETYPE eState = (OMX_STATETYPE) param1;
    int rc = 0;
    int bFlag = 1;
    nState = eState;
    if(hComp == NULL)
    {
        cmdData = NULL;
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if ( OMX_CommandStateSet == cmd )
    {
        /***************************/
        /* Current State is Loaded */
        /***************************/
        if ( OMX_StateLoaded == m_state )
        {
            if ( OMX_StateIdle == eState )
            {

                    if ( allocate_done() ||
                         (m_inp_bEnabled == OMX_FALSE &&
                          m_out_bEnabled == OMX_FALSE) )
                    {
                        DEBUG_PRINT("SCP-->Allocate Done Complete\n");
                    }
                    else
                    {
                        DEBUG_PRINT("SCP-->Loaded to Idle-Pending\n");
                        BITMASK_SET(&m_flags, OMX_COMPONENT_IDLE_PENDING);
                        bFlag = 0;
                    }

            }
            else if ( eState == OMX_StateLoaded )
            {
                DEBUG_PRINT("OMXCORE-SM: Loaded-->Loaded\n");
                m_cb.EventHandler(&this->m_cmp,
                                  this->m_app_data,
                                  OMX_EventError,
                                  OMX_ErrorSameState,
                                  0, NULL );
                eRet = OMX_ErrorSameState;
            }

            else if ( eState == OMX_StateWaitForResources )
            {
                DEBUG_PRINT("OMXCORE-SM: Loaded-->WaitForResources\n");
                eRet = OMX_ErrorNone;
            }

            else if ( eState == OMX_StateExecuting )
            {
                DEBUG_PRINT("OMXCORE-SM: Loaded-->Executing\n");
                m_cb.EventHandler(&this->m_cmp,
                                  this->m_app_data,
                                  OMX_EventError,
                                  OMX_ErrorIncorrectStateTransition,
                                  0, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            }

            else if ( eState == OMX_StatePause )
            {
                DEBUG_PRINT("OMXCORE-SM: Loaded-->Pause\n");
                m_cb.EventHandler(&this->m_cmp,
                                  this->m_app_data,
                                  OMX_EventError,
                                  OMX_ErrorIncorrectStateTransition,
                                  0, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            }

            else if ( eState == OMX_StateInvalid )
            {
                DEBUG_PRINT("OMXCORE-SM: Loaded-->Invalid\n");
                m_cb.EventHandler(&this->m_cmp,
                                  this->m_app_data,
                                  OMX_EventError,
                                  OMX_ErrorInvalidState,
                                  0, NULL );
                m_state = OMX_StateInvalid;
                eRet = OMX_ErrorInvalidState;
            }
            else
            {
                DEBUG_PRINT_ERROR("SCP-->Loaded to Invalid(%d))\n",eState);
                eRet = OMX_ErrorBadParameter;
            }
        }

        /***************************/
        /* Current State is IDLE */
        /***************************/
        else if ( OMX_StateIdle == m_state )
        {
            if ( OMX_StateLoaded == eState )
            {
                if ( release_done(-1) )
                {
                    if(suspensionPolicy == OMX_SuspensionEnabled)
                    {
                        ioctl(m_drv_fd,AUDIO_ABORT_GET_EVENT,NULL);
                    }

                    if ( ioctl(m_drv_fd, AUDIO_STOP, 0) == -1 )
                    {
                        DEBUG_PRINT("SCP:Idle->Loaded,ioctl stop failed %d\n",\
                                    errno);
                    }

                    nTimestamp = 0;
                    m_first = 0;

                    DEBUG_PRINT("SCP-->Idle to Loaded\n");
                }
                else
                {
                    DEBUG_PRINT("SCP--> Idle to Loaded-Pending\n");
                    BITMASK_SET(&m_flags, OMX_COMPONENT_LOADING_PENDING);
                    // Skip the event notification
                    bFlag = 0;
                }
            }
            else if ( OMX_StateExecuting == eState )
            {
                DEBUG_PRINT("CREATING EVENT THREAD -->GNG TO EXE STATE suspensionPolicy=%d \
                             m_ipc_to_event_th=%p",suspensionPolicy ,m_ipc_to_event_th);
                if((suspensionPolicy == OMX_SuspensionEnabled) && !m_ipc_to_event_th)
                {
                  m_ipc_to_event_th = omx_amr_event_thread_create(
                                            process_event_cb, this,
                                            (char*)"EVENT_THREAD");
                  if(!m_ipc_to_event_th)
                  {
                      DEBUG_PRINT_ERROR("ERROR!!! EVENT THREAD failed to get created\n");
                      sem_post (&sem_States);
                      return OMX_ErrorHardware;
                  }
              }
                struct msm_audio_config drv_config;
                struct msm_audio_pcm_config  pcm_config;
                ioctl(m_drv_fd, AUDIO_GET_CONFIG, &drv_config);

                if(!amrwb)
                {
                   drv_config.sample_rate = m_amrwbPlus_param.nSampleRate;
                   drv_config.channel_count = m_amrwbPlus_param.nChannels;
                }
                else
                {
                   drv_config.sample_rate = m_amrwbPlus_param.nSampleRate;
                   drv_config.channel_count = m_amr_param.nChannels;
                }
                drv_config.type = 2; // amr decoding
                if(pcm_feedback)
                {
                    drv_config.meta_field = 1; //Non-Tunnelled Mode
                }
                else
                {
                    drv_config.meta_field = 0; //Tunnelled mode
                }
                ioctl(m_drv_fd, AUDIO_SET_CONFIG, &drv_config);
                DEBUG_PRINT(" configure driver mode as %d \n",pcm_feedback);
                ioctl(m_drv_fd, AUDIO_GET_PCM_CONFIG, &pcm_config);
                pcm_config.pcm_feedback = pcm_feedback;
                pcm_config.buffer_size  =
                    OMX_AMR_OUTPUT_BUFFER_SIZE + sizeof(META_OUT);
                ioctl(m_drv_fd, AUDIO_SET_PCM_CONFIG, &pcm_config);

                pthread_mutex_lock(&m_suspendresume_lock);
                resetSuspendFlg();
                resetResumeFlg();
                pthread_mutex_unlock(&m_suspendresume_lock);
                DEBUG_PRINT("SCP-->Idle to Executing\n");
                nState = eState;
            }
            else if ( eState == OMX_StateIdle )
            {
                DEBUG_PRINT("OMXCORE-SM: Idle-->Idle\n");
                m_cb.EventHandler(&this->m_cmp,
                                  this->m_app_data,
                                  OMX_EventError,
                                  OMX_ErrorSameState,
                                  0, NULL );
                eRet = OMX_ErrorSameState;
            }
            else if ( eState == OMX_StateWaitForResources )
            {
                DEBUG_PRINT("OMXCORE-SM: Idle-->WaitForResources\n");
                this->m_cb.EventHandler(&this->m_cmp,
                                        this->m_app_data,
                                        OMX_EventError,
                                        OMX_ErrorIncorrectStateTransition,
                                        0, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            }

            else if ( eState == OMX_StatePause )
            {
                DEBUG_PRINT("OMXCORE-SM: Idle-->Pause\n");
            }

            else if ( eState == OMX_StateInvalid )
            {
                DEBUG_PRINT("OMXCORE-SM: Idle-->Invalid\n");
                m_state = OMX_StateInvalid;
                this->m_cb.EventHandler(&this->m_cmp,
                                        this->m_app_data,
                                        OMX_EventError,
                                        OMX_ErrorInvalidState,
                                        0, NULL );
                eRet = OMX_ErrorInvalidState;
            }
            else
            {
                DEBUG_PRINT_ERROR("SCP--> Idle to %d Not Handled\n",eState);
                eRet = OMX_ErrorBadParameter;
            }
        }

        /******************************/
        /* Current State is Executing */
        /******************************/
        else if ( OMX_StateExecuting == m_state )
        {
            if ( OMX_StateIdle == eState )
            {
                DEBUG_PRINT("SCP-->Executing to Idle \n");
                if ( 0 == pcm_feedback )
                {
                    execute_omx_flush(0,false); // Flush all ports
                }
                else
                {
                    execute_omx_flush(-1,false); // Flush all ports
                }
            }
            else if ( OMX_StatePause == eState )
            {
                DEBUG_DETAIL("*************************\n");
                DEBUG_PRINT("SCP-->RXED PAUSE STATE\n");
                DEBUG_DETAIL("*************************\n");
                //ioctl(m_drv_fd, AUDIO_PAUSE, 0);
                DEBUG_PRINT("SCP:E-->P, start timer\n");
                getTimerInst()->startTimer();
            } else if ( eState == OMX_StateLoaded )
            {
                DEBUG_PRINT("\n OMXCORE-SM: Executing --> Loaded \n");
                this->m_cb.EventHandler(&this->m_cmp,
                                        this->m_app_data,
                                        OMX_EventError,
                                        OMX_ErrorIncorrectStateTransition,
                                        0, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            else if ( eState == OMX_StateWaitForResources )
            {
                DEBUG_PRINT("\n OMXCORE-SM: Executing --> WaitForResources \n");
                this->m_cb.EventHandler(&this->m_cmp,
                                        this->m_app_data,
                                        OMX_EventError,
                                        OMX_ErrorIncorrectStateTransition,
                                        0, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            else if ( eState == OMX_StateExecuting )
            {
                DEBUG_PRINT("\n OMXCORE-SM: Executing --> Executing \n");
                this->m_cb.EventHandler(&this->m_cmp, this->m_app_data,
                                        OMX_EventError, OMX_ErrorSameState,
                                        0, NULL );
                eRet = OMX_ErrorSameState;
            }
            else if ( eState == OMX_StateInvalid )
            {
                DEBUG_PRINT("\n OMXCORE-SM: Executing --> Invalid \n");
                m_state = OMX_StateInvalid;
                this->m_cb.EventHandler(&this->m_cmp, this->m_app_data,
                                        OMX_EventError, OMX_ErrorInvalidState,
                                        0, NULL );
                eRet = OMX_ErrorInvalidState;
            }
            else
            {
                DEBUG_PRINT_ERROR("SCP--> Executing to %d Not Handled\n",eState);
                eRet = OMX_ErrorBadParameter;
            }
        }
        /***************************/
        /* Current State is Pause  */
        /***************************/
        else if ( OMX_StatePause == m_state )
        {
            DEBUG_PRINT("SCP: Paused --> Executing %d %d in=%d out=%d\n",\
                             getSuspendFlg(),getResumeFlg(),
                             is_in_th_sleep,is_out_th_sleep);
            if(!getTimerInst()->getTimerExpiry())
            {
                getTimerInst()->stopTimer();
            }

            getTimerInst()->resetTimerExpiry();

            if (eState == OMX_StateExecuting)
            {
                DEBUG_PRINT("SCP: changing flag stat m_pause_to_exe=%d\n",m_pause_to_exe);
                m_pause_to_exe = true;
            }

            if( (eState == OMX_StateExecuting || eState == OMX_StateIdle) )
            {
                pthread_mutex_lock(&m_out_th_lock_1);
                if (is_out_th_sleep)
                {
                    DEBUG_DETAIL("PE: WAKING UP OUT THREAD\n");
                    out_th_wakeup();
                    is_out_th_sleep = false;
                }
                pthread_mutex_unlock(&m_out_th_lock_1);
            }

            if ( OMX_StateExecuting == eState )
            {
                nState = eState;
                pthread_mutex_lock(&m_suspendresume_lock);
                if(getSuspendFlg())
                    setResumeFlg();
                DEBUG_PRINT("SCP: Paused --> Executing %d %d\n",\
                            getSuspendFlg(),getResumeFlg());
                if(getSuspendFlg() && getResumeFlg())
                {
                    post_output(0,0,OMX_COMPONENT_RESUME);
                    rc = ioctl(m_drv_fd, AUDIO_START, 0);
                    if (rc < 0)
                    {
                      DEBUG_PRINT_ERROR("AUDIO_START FAILED\n");
                      post_command((unsigned)OMX_CommandStateSet,
                             (unsigned)OMX_StateInvalid,
                              OMX_COMPONENT_GENERATE_COMMAND);
                      pthread_mutex_unlock(&m_suspendresume_lock);
                      execute_omx_flush(-1,false);
                      return OMX_ErrorInvalidState;
                    }
                    resetSuspendFlg();
                    resetResumeFlg();
                }
                pthread_mutex_unlock(&m_suspendresume_lock);
            } else if ( OMX_StateIdle == eState )
            {
                DEBUG_PRINT("SCP-->Paused to Idle \n");
                DEBUG_PRINT ("\n Internal flush issued");
                pthread_mutex_lock(&m_flush_lock);
                m_flush_cnt = 2;
                pthread_mutex_unlock(&m_flush_lock);
                if ( 0 == pcm_feedback )
                {
                    execute_omx_flush(0,false); // Flush all ports
                }
                else
                {
                    execute_omx_flush(-1,false); // Flush all ports
                }
            }
            else if ( eState == OMX_StateLoaded )
            {
                DEBUG_PRINT("\n Pause --> loaded \n");
                this->m_cb.EventHandler(&this->m_cmp,
                                        this->m_app_data,
                                        OMX_EventError,
                                        OMX_ErrorIncorrectStateTransition,
                                        0, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            else if ( eState == OMX_StateWaitForResources )
            {
                DEBUG_PRINT("\n Pause --> WaitForResources \n");
                this->m_cb.EventHandler(&this->m_cmp,
                                        this->m_app_data,
                                        OMX_EventError,
                                        OMX_ErrorIncorrectStateTransition,
                                        0, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            else if ( eState == OMX_StatePause )
            {
                DEBUG_PRINT("\n Pause --> Pause \n");
                this->m_cb.EventHandler(&this->m_cmp, this->m_app_data,
                                        OMX_EventError, OMX_ErrorSameState,
                                        0, NULL );
                eRet = OMX_ErrorSameState;
            }
            else if ( eState == OMX_StateInvalid )
            {
                DEBUG_PRINT("\n Pause --> Invalid \n");
                m_state = OMX_StateInvalid;
                this->m_cb.EventHandler(&this->m_cmp, this->m_app_data,
                                        OMX_EventError, OMX_ErrorInvalidState,
                                        0, NULL );
                eRet = OMX_ErrorInvalidState;
            }
            else
            {
                DEBUG_PRINT("SCP-->Paused to %d Not Handled\n",eState);
                eRet = OMX_ErrorBadParameter;
            }
        }
        /**************************************/
        /* Current State is WaitForResources  */
        /**************************************/
        else if ( m_state == OMX_StateWaitForResources )
        {
            if ( eState == OMX_StateLoaded )
            {
                DEBUG_PRINT("OMXCORE-SM: WaitForResources-->Loaded\n");
            }
            else if ( eState == OMX_StateWaitForResources )
            {
                DEBUG_PRINT("OMXCORE-SM: WaitForResources-->WaitForResources\n");
                this->m_cb.EventHandler(&this->m_cmp, this->m_app_data,
                                        OMX_EventError, OMX_ErrorSameState,
                                        0, NULL );
                eRet = OMX_ErrorSameState;
            }
            else if ( eState == OMX_StateExecuting )
            {
                DEBUG_PRINT("OMXCORE-SM: WaitForResources-->Executing\n");
                this->m_cb.EventHandler(&this->m_cmp,
                                        this->m_app_data,
                                        OMX_EventError,
                                        OMX_ErrorIncorrectStateTransition,
                                        0, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            else if ( eState == OMX_StatePause )
            {
                DEBUG_PRINT("OMXCORE-SM: WaitForResources-->Pause\n");
                this->m_cb.EventHandler(&this->m_cmp,
                                        this->m_app_data,
                                        OMX_EventError,
                                        OMX_ErrorIncorrectStateTransition,
                                        0, NULL );
                eRet = OMX_ErrorIncorrectStateTransition;
            }
            else if ( eState == OMX_StateInvalid )
            {
                DEBUG_PRINT("OMXCORE-SM: WaitForResources-->Invalid\n");
                m_state = OMX_StateInvalid;
                this->m_cb.EventHandler(&this->m_cmp, this->m_app_data,
                                        OMX_EventError, OMX_ErrorInvalidState,
                                        0, NULL );
                eRet = OMX_ErrorInvalidState;
            }
            else
            {
                DEBUG_PRINT_ERROR("SCP--> %d to %d(Not Handled)\n",
                                  m_state,eState);
                eRet = OMX_ErrorBadParameter;
            }
        }
        /****************************/
        /* Current State is Invalid */
        /****************************/
        else if ( m_state == OMX_StateInvalid )
        {
            if ( OMX_StateLoaded == eState || OMX_StateWaitForResources == eState
                 || OMX_StateIdle == eState || OMX_StateExecuting == eState
                 || OMX_StatePause == eState || OMX_StateInvalid == eState )
            {
                DEBUG_PRINT("OMXCORE-SM: Invalid-->Loaded/Idle/Executing"
                            "/Pause/Invalid/WaitForResources\n");
                m_state = OMX_StateInvalid;
                this->m_cb.EventHandler(&this->m_cmp, this->m_app_data,
                                        OMX_EventError, OMX_ErrorInvalidState,
                                        0, NULL );
                eRet = OMX_ErrorInvalidState;
            }
        }
        else
        {
            DEBUG_PRINT_ERROR("OMXCORE-SM: %d --> %d(Not Handled)\n",
                              m_state,eState);
            eRet = OMX_ErrorBadParameter;
        }
    }
    else if ( OMX_CommandFlush == cmd )
    {
        DEBUG_DETAIL("*************************\n");
        DEBUG_PRINT("SCP-->RXED FLUSH COMMAND port=%lu\n",param1);
        DEBUG_DETAIL("*************************\n");
        bFlag = 0;
        if ( param1 == OMX_CORE_INPUT_PORT_INDEX ||
             param1 == OMX_CORE_OUTPUT_PORT_INDEX ||
             param1 == OMX_ALL )
        {
            execute_omx_flush(param1);
        }
        else
        {
            eRet = OMX_ErrorBadPortIndex;
            m_cb.EventHandler(&m_cmp, m_app_data, OMX_EventError,
                              OMX_CommandFlush, OMX_ErrorBadPortIndex, NULL );
        }
    }
    else if ( cmd == OMX_CommandPortDisable )
    {
	// Skip the event notification
	bFlag = 0;
        if ( param1 == OMX_CORE_INPUT_PORT_INDEX || param1 == OMX_ALL )
        {
            DEBUG_PRINT("SCP: Disabling Input port Indx\n");
            m_inp_bEnabled = OMX_FALSE;
            if ( (m_state == OMX_StateLoaded || m_state == OMX_StateIdle)
                 && release_done(0) )
            {
                DEBUG_PRINT("send_command_proxy:OMX_CommandPortDisable:\
                            OMX_CORE_INPUT_PORT_INDEX:release_done \n");
                DEBUG_PRINT("************* OMX_CommandPortDisable:\
                            m_inp_bEnabled =%d********\n",m_inp_bEnabled);

                post_command(OMX_CommandPortDisable,
                             OMX_CORE_INPUT_PORT_INDEX,
                             OMX_COMPONENT_GENERATE_EVENT);
            }

            else
            {
                if ( m_state == OMX_StatePause ||m_state == OMX_StateExecuting )
                {
                    DEBUG_PRINT("SCP: execute_omx_flush in Disable in  "
                                "param1=%lu m_state=%d \n",param1, m_state);
                    execute_omx_flush(param1);
                }
                DEBUG_PRINT("send_command_proxy:OMX_CommandPortDisable:\
                            OMX_CORE_INPUT_PORT_INDEX \n");
                BITMASK_SET(&m_flags, OMX_COMPONENT_INPUT_DISABLE_PENDING);
            }

        }
        if ( param1 == OMX_CORE_OUTPUT_PORT_INDEX || param1 == OMX_ALL )
        {

            pcm_feedback = 0;
            DEBUG_PRINT("SCP: Disabling Output port Indx\n");
            m_out_bEnabled = OMX_FALSE;
            if ( (m_state == OMX_StateLoaded || m_state == OMX_StateIdle)
                 && release_done(1) )
            {
                DEBUG_PRINT("send_command_proxy:OMX_CommandPortDisable:\
                            OMX_CORE_OUTPUT_PORT_INDEX:release_done \n");
                DEBUG_PRINT("************* OMX_CommandPortDisable:\
                            m_out_bEnabled=%d********\n",m_inp_bEnabled);

                post_command(OMX_CommandPortDisable,
                             OMX_CORE_OUTPUT_PORT_INDEX,
                             OMX_COMPONENT_GENERATE_EVENT);
            }

            else
            {
                if ( m_state == OMX_StatePause ||m_state == OMX_StateExecuting )
                {
                    DEBUG_PRINT("SCP: execute_omx_flush in Disable out "
                                "param1=%lu m_state=%d \n",param1, m_state);
                    execute_omx_flush(param1);
                }
                BITMASK_SET(&m_flags, OMX_COMPONENT_OUTPUT_DISABLE_PENDING);
            }
        }
        else
        {
            DEBUG_PRINT_ERROR("OMX_CommandPortDisable: disable wrong port ID");
        }

    }
    else if ( cmd == OMX_CommandPortEnable )
    {
	// Skip the event notification
	bFlag = 0;
        if ( param1 == OMX_CORE_INPUT_PORT_INDEX  || param1 == OMX_ALL )
        {
            m_inp_bEnabled = OMX_TRUE;
            m_residual_data_len = 0;
            DEBUG_PRINT("SCP: Enabling Input port Indx\n");
            if ( (m_state == OMX_StateLoaded
                  && !BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING))
                 || (m_state == OMX_StateWaitForResources)
                 || (m_inp_bPopulated == OMX_TRUE) )
            {
                post_command(OMX_CommandPortEnable,
                             OMX_CORE_INPUT_PORT_INDEX,
                             OMX_COMPONENT_GENERATE_EVENT);
            }
            else
            {
                BITMASK_SET(&m_flags, OMX_COMPONENT_INPUT_ENABLE_PENDING);
            }
        }
        if ( param1 == OMX_CORE_OUTPUT_PORT_INDEX || param1 == OMX_ALL )
        {
            pcm_feedback = 1;
            DEBUG_PRINT("SCP: Enabling Output port Indx\n");
            m_out_bEnabled = OMX_TRUE;
            if ( (m_state == OMX_StateLoaded
                  && !BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING))
                 || (m_state == OMX_StateWaitForResources)
                 || (m_out_bPopulated == OMX_TRUE) )
            {
                post_command(OMX_CommandPortEnable,
                             OMX_CORE_OUTPUT_PORT_INDEX,
                             OMX_COMPONENT_GENERATE_EVENT);
            }
            else
            {
                DEBUG_PRINT("send_command_proxy:OMX_CommandPortEnable:\
                            OMX_CORE_OUTPUT_PORT_INDEX:release_done \n");
                BITMASK_SET(&m_flags, OMX_COMPONENT_OUTPUT_ENABLE_PENDING);
            }
            pthread_mutex_lock(&m_in_th_lock_1);
            if(is_in_th_sleep)
            {
                    is_in_th_sleep = false;
                    DEBUG_DETAIL("SCP:WAKING UP IN THREADS\n");
                    in_th_wakeup();
            }
            pthread_mutex_unlock(&m_in_th_lock_1);
            pthread_mutex_lock(&m_out_th_lock_1);
            if ( is_out_th_sleep )
            {
                is_out_th_sleep = false;
                DEBUG_PRINT("SCP:WAKING OUT THR, OMX_CommandPortEnable\n");
                out_th_wakeup();
            }
            pthread_mutex_unlock(&m_out_th_lock_1);
        }
        else
        {
            DEBUG_PRINT_ERROR("OMX_CommandPortEnable: disable wrong port ID");
        }
    }
    else
    {
        DEBUG_PRINT_ERROR("SCP-->ERROR: Invali Command [%d]\n",cmd);
        eRet = OMX_ErrorNotImplemented;
    }
    DEBUG_PRINT("posting sem_States\n");
    sem_post (&sem_States);
    if ( eRet == OMX_ErrorNone && bFlag )
    {
        post_command(cmd,eState,OMX_COMPONENT_GENERATE_EVENT);
    }
    return eRet;
}

/*=============================================================================
FUNCTION:
  execute_omx_flush

DESCRIPTION:
  Function that flushes buffers that are pending to be written to driver

INPUT/OUTPUT PARAMETERS:
  [IN] param1
  [IN] cmd_cmpl

RETURN VALUE:
  true
  false

Dependency:
  None

SIDE EFFECTS:
  None
=============================================================================*/
bool omx_amr_adec::execute_omx_flush(OMX_IN OMX_U32 param1, bool cmd_cmpl)
{
    bool bRet = true;
    DEBUG_PRINT("Execute_omx_flush Port[%lu]", param1);
    struct timespec abs_timeout;
    abs_timeout.tv_sec = 1;
    abs_timeout.tv_nsec = 0; //333333;

    if ( param1 == OMX_ALL )
    {
        bFlushinprogress = true;
        DEBUG_PRINT("Execute flush for both I/p O/p port\n");
        pthread_mutex_lock(&m_flush_lock);
        m_flush_cnt = 2;
        pthread_mutex_unlock(&m_flush_lock);

        // Send Flush commands to input and output threads
        post_input(OMX_CommandFlush,
                   OMX_CORE_INPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_COMMAND);
        post_output(OMX_CommandFlush,
                    OMX_CORE_OUTPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_COMMAND);
        // Send Flush to the kernel so that the in and out buffers are released
        if ( ioctl( m_drv_fd, AUDIO_FLUSH, 0) == -1 )
            DEBUG_PRINT("FLush:ioctl flush failed errno=%d\n",errno);
        DEBUG_DETAIL("****************************************");
        DEBUG_DETAIL("is_in_th_sleep=%d is_out_th_sleep=%d\n",\
                     is_in_th_sleep,is_out_th_sleep);
        DEBUG_DETAIL("****************************************");

        pthread_mutex_lock(&m_in_th_lock_1);
        if ( is_in_th_sleep )
        {
            is_in_th_sleep = false;
            DEBUG_DETAIL("For FLUSH-->WAKING UP IN THREADS\n");
            in_th_wakeup();
        }
        pthread_mutex_unlock(&m_in_th_lock_1);

        pthread_mutex_lock(&m_out_th_lock_1);
        if ( is_out_th_sleep )
        {
            is_out_th_sleep = false;
            DEBUG_DETAIL("For FLUSH-->WAKING UP OUT THREADS\n");
            out_th_wakeup();
        }
        pthread_mutex_unlock(&m_out_th_lock_1);

        while ( 1 )
        {
            pthread_mutex_lock(&out_buf_count_lock);
            pthread_mutex_lock(&in_buf_count_lock);
            DEBUG_PRINT("Flush:nNumOutputBuf = %d nNumInputBuf=%d\n",\
                        nNumOutputBuf,nNumInputBuf);

            if ( nNumOutputBuf > 0 || nNumInputBuf > 0 )
            {
                pthread_mutex_unlock(&in_buf_count_lock);
                pthread_mutex_unlock(&out_buf_count_lock);
                pthread_mutex_lock(&m_in_th_lock_1);
                if(is_in_th_sleep)
                {
                    is_in_th_sleep = false;
                    DEBUG_DETAIL("FLUSH-->WAKING UP IN THREAD\n");
                    in_th_wakeup();
                }
                pthread_mutex_unlock(&m_in_th_lock_1);

                pthread_mutex_lock(&m_out_th_lock_1);
                if ( is_out_th_sleep )
                {
                    is_out_th_sleep = false;
                    DEBUG_DETAIL("FLUSH-->WAKING UP OUT THREAD\n");
                    out_th_wakeup();
                }
                pthread_mutex_unlock(&m_out_th_lock_1);
                DEBUG_PRINT(" READ FLUSH PENDING HENCE WAIT\n");
                DEBUG_PRINT("BEFORE READ ioctl_flush\n");
                usleep (10000);
                if ( ioctl( m_drv_fd, AUDIO_FLUSH, 0) == -1 )
                    DEBUG_PRINT("Flush: ioctl flush failed %d\n",\
                                errno);
                DEBUG_PRINT("AFTER READ ioctl_flush\n");
                sem_timedwait(&sem_read_msg,&abs_timeout);
                DEBUG_PRINT("AFTER READ SEM_TIMEWAIT\n");
            }
            else
            {
                pthread_mutex_unlock(&in_buf_count_lock);
                pthread_mutex_unlock(&out_buf_count_lock);
                break;
            }
        }
        // sleep till the FLUSH ACK are done by both the input and
        // output threads
        DEBUG_DETAIL("WAITING FOR FLUSH ACK's param1=%lu",param1);
        wait_for_event();

        DEBUG_PRINT("RECIEVED BOTH FLUSH ACK's param1=%lu cmd_cmpl=%d",\
                    param1,cmd_cmpl);

        // If not going to idle state, Send FLUSH complete message to the Client,
        // now that FLUSH ACK's have been recieved.
        if ( cmd_cmpl )
        {
            m_cb.EventHandler(&m_cmp, m_app_data, OMX_EventCmdComplete,
                              OMX_CommandFlush,
                              OMX_CORE_INPUT_PORT_INDEX, NULL );
            m_cb.EventHandler(&m_cmp, m_app_data, OMX_EventCmdComplete,
                              OMX_CommandFlush,
                              OMX_CORE_OUTPUT_PORT_INDEX, NULL );
            DEBUG_PRINT("Inside FLUSH.. sending FLUSH CMPL\n");
        }
        bFlushinprogress = false;
    }
    else if ( OMX_CORE_INPUT_PORT_INDEX == param1 )
    {
        DEBUG_PRINT("Execute FLUSH for I/p port\n");
        pthread_mutex_lock(&m_flush_lock);
        m_flush_cnt = 1;
        pthread_mutex_unlock(&m_flush_lock);
        post_input(OMX_CommandFlush,
                   OMX_CORE_INPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_COMMAND);
        if ( ioctl( m_drv_fd, AUDIO_FLUSH, 0) == -1 )
            DEBUG_PRINT("Flush:Input port, ioctl flush failed %d\n",errno);
        DEBUG_DETAIL("****************************************");
        DEBUG_DETAIL("is_in_th_sleep=%d is_out_th_sleep=%d\n",\
                     is_in_th_sleep,is_out_th_sleep);
        DEBUG_DETAIL("****************************************");

        if ( is_in_th_sleep )
        {
            pthread_mutex_lock(&m_in_th_lock_1);
            is_in_th_sleep = false;
            pthread_mutex_unlock(&m_in_th_lock_1);
            DEBUG_DETAIL("For FLUSH-->WAKING UP IN THREADS\n");
            in_th_wakeup();
        }

        if ( is_out_th_sleep )
        {
            pthread_mutex_lock(&m_out_th_lock_1);
            is_out_th_sleep = false;
            pthread_mutex_unlock(&m_out_th_lock_1);
            DEBUG_DETAIL("For FLUSH-->WAKING UP OUT THREADS\n");
            out_th_wakeup();
        }

        // sleep till the FLUSH ACK are done by both the input and
        // output threads
        DEBUG_DETAIL("Executing FLUSH for I/p port\n");
        DEBUG_DETAIL("WAITING FOR FLUSH ACK's param1=%lu",param1);
        wait_for_event();
        DEBUG_DETAIL(" RECIEVED FLUSH ACK FOR I/P PORT param1=%lu",param1);

        // Send FLUSH complete message to the Client,
        // now that FLUSH ACK's have been recieved.
        // if not going to idle state, send FLUSH complete message to client
        if ( cmd_cmpl )
        {
            m_cb.EventHandler(&m_cmp, m_app_data, OMX_EventCmdComplete,
                              OMX_CommandFlush,
                              OMX_CORE_INPUT_PORT_INDEX, NULL );
        }
    }
    else if ( OMX_CORE_OUTPUT_PORT_INDEX == param1 )
    {
        DEBUG_PRINT("Executing FLUSH for O/p port\n");
        pthread_mutex_lock(&m_flush_lock);
        m_flush_cnt = 1;
        pthread_mutex_unlock(&m_flush_lock);
        DEBUG_DETAIL("Executing FLUSH for O/p port\n");
        DEBUG_DETAIL("WAITING FOR FLUSH ACK's param1=%lu",param1);
        post_output(OMX_CommandFlush,
                    OMX_CORE_OUTPUT_PORT_INDEX,OMX_COMPONENT_GENERATE_COMMAND);
        if ( ioctl( m_drv_fd, AUDIO_FLUSH, 0) ==-1 )
            DEBUG_PRINT("Flush:Output port, ioctl flush failed %d\n",errno);
        DEBUG_DETAIL("****************************************");
        DEBUG_DETAIL("is_in_th_sleep=%d is_out_th_sleep=%d\n",\
                     is_in_th_sleep,is_out_th_sleep);
        DEBUG_DETAIL("****************************************");
        if ( is_in_th_sleep )
        {
            pthread_mutex_lock(&m_in_th_lock_1);
            is_in_th_sleep = false;
            pthread_mutex_unlock(&m_in_th_lock_1);
            DEBUG_DETAIL("For FLUSH-->WAKING UP IN THREADS\n");
            in_th_wakeup();
        }

        //   pthread_mutex_lock(&m_out_th_lock_1);
        if ( is_out_th_sleep )
        {
            pthread_mutex_lock(&m_out_th_lock_1);
            is_out_th_sleep = false;
            pthread_mutex_unlock(&m_out_th_lock_1);
            DEBUG_DETAIL("For FLUSH-->WAKING UP OUT THREADS\n");
            out_th_wakeup();
        }
        //  pthread_mutex_unlock(&m_out_th_lock_1);
        // sleep till the FLUSH ACK are done by both the input and output threads
        wait_for_event();
        // Send FLUSH complete message to the Client,
        // now that FLUSH ACK's have been recieved.
        if ( cmd_cmpl )
        {
            m_cb.EventHandler(&m_cmp, m_app_data, OMX_EventCmdComplete,
                              OMX_CommandFlush,
                              OMX_CORE_OUTPUT_PORT_INDEX, NULL );
        }
        DEBUG_DETAIL("RECIEVED FLUSH ACK FOR O/P PORT param1=%lu",param1);
    }
    else
    {
        DEBUG_PRINT("Invalid Port ID[%lu]",param1);
    }
    return bRet;
}

/*=============================================================================
FUNCTION:
  execute_input_omx_flush

DESCRIPTION:
  Function that flushes buffers that are pending to be written to driver

INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE:
  true
  false

Dependency:
  None

SIDE EFFECTS:
  None
=============================================================================*/
bool omx_amr_adec::execute_input_omx_flush()
{
    OMX_BUFFERHEADERTYPE *omx_buf;
    unsigned      p1; // Parameter - 1
    unsigned      p2; // Parameter - 2
    unsigned      ident;
    unsigned      qsize     = 0; // qsize
    unsigned      tot_qsize = 0; // qsize

    DEBUG_PRINT("Execute_omx_flush on input port");

    pthread_mutex_lock(&m_lock);
    do
    {
        qsize = m_input_q.m_size;
        tot_qsize = qsize;
        tot_qsize += m_input_ctrl_ebd_q.m_size;

        DEBUG_DETAIL("Input FLUSH-->flushq[%d] ebd[%d]dataq[%d]",\
                     m_input_ctrl_cmd_q.m_size,
                     m_input_ctrl_ebd_q.m_size,qsize);
        if ( !tot_qsize )
        {
            DEBUG_DETAIL("Input-->BREAKING FROM execute_input_flush LOOP");
            break;
        }
        if ( qsize )
        {
            m_input_q.pop_entry(&p1, &p2, &ident);
            if ( (ident == OMX_COMPONENT_GENERATE_ETB) ||
                 (ident == OMX_COMPONENT_GENERATE_BUFFER_DONE) )
            {
                omx_buf = (OMX_BUFFERHEADERTYPE *) p2;
                DEBUG_DETAIL("Flush:Input dataq=0x%p \n", omx_buf);
                omx_buf->nFilledLen = 0;
                buffer_done_cb((OMX_BUFFERHEADERTYPE *)omx_buf);
            }
        }
        else if ( m_input_ctrl_ebd_q.m_size )
        {
            m_input_ctrl_ebd_q.pop_entry(&p1, &p2, &ident);
            if ( ident == OMX_COMPONENT_GENERATE_BUFFER_DONE )
            {
                omx_buf = (OMX_BUFFERHEADERTYPE *) p2;
                omx_buf->nFilledLen = 0;
                DEBUG_DETAIL("Flush:ctrl dataq=0x%p \n", omx_buf);
                buffer_done_cb((OMX_BUFFERHEADERTYPE *)omx_buf);
            }
        }
        else
        {
        }
    }while ( tot_qsize>0 );
    DEBUG_DETAIL("*************************\n");
    DEBUG_DETAIL("IN-->FLUSHING DONE\n");
    DEBUG_DETAIL("*************************\n");
    flush_ack();
    pthread_mutex_unlock(&m_lock);
    return true;
}

/*=============================================================================
FUNCTION:
  execute_output_omx_flush

DESCRIPTION:
  Function that flushes buffers that are pending to be written to driver

INPUT/OUTPUT PARAMETERS:
  None

RETURN VALUE:
  true
  false

Dependency:
  None

SIDE EFFECTS:
  None
=============================================================================*/
bool omx_amr_adec::execute_output_omx_flush()
{
    OMX_BUFFERHEADERTYPE *omx_buf;
    unsigned      p1; // Parameter - 1
    unsigned      p2; // Parameter - 2
    unsigned      ident;
    unsigned      qsize=0; // qsize
    unsigned      tot_qsize=0; // qsize

    DEBUG_PRINT("Execute_omx_flush on output port");

    pthread_mutex_lock(&m_outputlock);
    do
    {
        qsize = m_output_q.m_size;
        DEBUG_DETAIL("OUT FLUSH-->flushq[%d] fbd[%d]dataq[%d]",\
                     m_output_ctrl_cmd_q.m_size,
                     m_output_ctrl_fbd_q.m_size,qsize);
        tot_qsize = qsize;
        tot_qsize += m_output_ctrl_fbd_q.m_size;
        if ( !tot_qsize )
        {
            DEBUG_DETAIL("OUT-->BREAKING FROM execute_input_flush LOOP");
            break;
        }
        if ( qsize )
        {
            m_output_q.pop_entry(&p1,&p2,&ident);
            if ( (OMX_COMPONENT_GENERATE_FTB == ident) ||
                 (OMX_COMPONENT_GENERATE_FRAME_DONE == ident) )
            {
                omx_buf = (OMX_BUFFERHEADERTYPE *) p2;
                DEBUG_DETAIL("Ouput Buf_Addr=%p TS[0x%x] \n",\
                             omx_buf,nTimestamp);
                omx_buf->nTimeStamp = nTimestamp;
                omx_buf->nFilledLen = 0;
                frame_done_cb((OMX_BUFFERHEADERTYPE *)omx_buf);
                DEBUG_DETAIL("CALLING FBD FROM FLUSH");
            }
        }
        else if ( (qsize = m_output_ctrl_fbd_q.m_size) )
        {
            m_output_ctrl_fbd_q.pop_entry(&p1, &p2, &ident);
            if ( OMX_COMPONENT_GENERATE_FRAME_DONE == ident )
            {
                omx_buf = (OMX_BUFFERHEADERTYPE *) p2;
                DEBUG_DETAIL("Ouput Buf_Addr=%p TS[0x%x] \n", \
                             omx_buf,nTimestamp);
                omx_buf->nTimeStamp = nTimestamp;
                omx_buf->nFilledLen = 0;
                frame_done_cb((OMX_BUFFERHEADERTYPE *)omx_buf);
                DEBUG_DETAIL("CALLING FROM CTRL-FBDQ FROM FLUSH");
            }
        }
    }while ( qsize>0 );
    DEBUG_DETAIL("*************************\n");
    DEBUG_DETAIL("OUT-->FLUSHING DONE\n");
    DEBUG_DETAIL("*************************\n");
    flush_ack();
    pthread_mutex_unlock(&m_outputlock);

    return true;
}

/*=============================================================================
FUNCTION:
  post_input

DESCRIPTION:
  Function that posts command in the command queue

INPUT/OUTPUT PARAMETERS:
  [IN] p1
  [IN] p2
  [IN] id - command ID
  [IN] lock - self-locking mode

RETURN VALUE:
  true
  false

Dependency:
  None

SIDE EFFECTS:
  None
=============================================================================*/
bool omx_amr_adec::post_input(unsigned int p1,
                              unsigned int p2,
                              unsigned int id)
{
    bool bRet = false;
    pthread_mutex_lock(&m_lock);

    if((OMX_COMPONENT_GENERATE_COMMAND == id) || (id == OMX_COMPONENT_SUSPEND))
    {
        // insert flush message and ebd
        m_input_ctrl_cmd_q.insert_entry(p1,p2,id);
    }
    else if ( (OMX_COMPONENT_GENERATE_BUFFER_DONE == id) )
    {
        // insert ebd
        m_input_ctrl_ebd_q.insert_entry(p1,p2,id);
    }
    else
    {
        // ETBS in this queue
        m_input_q.insert_entry(p1,p2,id);
    }

    if ( m_ipc_to_in_th )
    {
        bRet = true;
        omx_amr_post_msg(m_ipc_to_in_th, id);
    }

    DEBUG_DETAIL("PostInput-->state[%d]id[%d]flushq[%d]ebdq[%d]dataq[%d] \n",\
                 m_state,
                 id,
                 m_input_ctrl_cmd_q.m_size,
                 m_input_ctrl_ebd_q.m_size,
                 m_input_q.m_size);

    pthread_mutex_unlock(&m_lock);
    return bRet;
}

/*=============================================================================
FUNCTION:
  post_command

DESCRIPTION:
  Function that posts command in the command queue

INPUT/OUTPUT PARAMETERS:
  [IN] p1
  [IN] p2
  [IN] id - command ID
  [IN] lock - self-locking mode

RETURN VALUE:
  true
  false

Dependency:
  None

SIDE EFFECTS:
  None
=============================================================================*/
bool omx_amr_adec::post_command(unsigned int p1,
                                unsigned int p2,
                                unsigned int id)
{
    bool bRet  = false;

    pthread_mutex_lock(&m_commandlock);

    m_command_q.insert_entry(p1,p2,id);

    if ( m_ipc_to_cmd_th )
    {
        bRet = true;
        omx_amr_post_msg(m_ipc_to_cmd_th, id);
    }

    DEBUG_DETAIL("PostCmd-->state[%d]id[%d]cmdq[%d]flags[%x]\n",\
                 m_state,
                 id,
                 m_command_q.m_size,
                 m_flags >> 3);

    pthread_mutex_unlock(&m_commandlock);
    return bRet;
}

/*=============================================================================
FUNCTION:
  post_output

DESCRIPTION:
  Function that posts command in the command queue

INPUT/OUTPUT PARAMETERS:
  [IN] p1
  [IN] p2
  [IN] id - command ID
  [IN] lock - self-locking mode

RETURN VALUE:
  true
  false

Dependency:
  None

SIDE EFFECTS:
  None
=============================================================================*/
bool omx_amr_adec::post_output(unsigned int p1,
                               unsigned int p2,
                               unsigned int id)
{
    bool bRet = false;

    pthread_mutex_lock(&m_outputlock);
    if((OMX_COMPONENT_GENERATE_COMMAND == id) || (id == OMX_COMPONENT_SUSPEND)
        || (id == OMX_COMPONENT_RESUME))
    {
        // insert flush message and fbd
        m_output_ctrl_cmd_q.insert_entry(p1,p2,id);
    }
    else if ( (OMX_COMPONENT_GENERATE_FRAME_DONE == id) )
    {
        // insert flush message and fbd
        m_output_ctrl_fbd_q.insert_entry(p1,p2,id);
    }
    else
    {
        m_output_q.insert_entry(p1,p2,id);
    }

    if ( m_ipc_to_out_th )
    {
        bRet = true;
        omx_amr_post_msg(m_ipc_to_out_th, id);
    }
    DEBUG_DETAIL("PostOutput-->state[%d]id[%d]flushq[%d]ebdq[%d]dataq[%d]\n",\
                 m_state,
                 id,
                 m_output_ctrl_cmd_q.m_size,
                 m_output_ctrl_fbd_q.m_size,
                 m_output_q.m_size);

    pthread_mutex_unlock(&m_outputlock);
    return bRet;
}
/*
  @brief member function that return parameters to IL client

  @param hComp handle to component instance
  @param paramIndex Parameter type
  @param paramData pointer to memory space which would hold the
        paramter
  @return error status
*/
OMX_ERRORTYPE  omx_amr_adec::get_parameter(OMX_IN OMX_HANDLETYPE     hComp,
                                           OMX_IN OMX_INDEXTYPE paramIndex,
                                           OMX_INOUT OMX_PTR     paramData)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    if(hComp == NULL)
    {
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if ( m_state == OMX_StateInvalid )
    {
        DEBUG_PRINT_ERROR("Get Param in Invalid State\n");
        return OMX_ErrorInvalidState;
    }
    if ( paramData == NULL )
    {
        DEBUG_PRINT("get_parameter: paramData is NULL\n");
        return OMX_ErrorBadParameter;
    }

    switch ( paramIndex )
    {
        case OMX_IndexParamPortDefinition:
            {
                OMX_PARAM_PORTDEFINITIONTYPE *portDefn;
                portDefn = (OMX_PARAM_PORTDEFINITIONTYPE *) paramData;

                DEBUG_PRINT("OMX_IndexParamPortDefinition nPortIndex = %lu\n",
                            portDefn->nPortIndex);

                portDefn->nVersion.nVersion = OMX_SPEC_VERSION;
                portDefn->nSize = sizeof(portDefn);
                portDefn->eDomain    = OMX_PortDomainAudio;

                if ( 0 == portDefn->nPortIndex )
                {
                    portDefn->eDir =  OMX_DirInput;
                    portDefn->bEnabled           = m_inp_bEnabled;
                    portDefn->bPopulated         = m_inp_bPopulated;
                    portDefn->nBufferCountActual = m_inp_act_buf_count;
                    portDefn->nBufferCountMin    = OMX_CORE_NUM_INPUT_BUFFERS;
                    portDefn->nBufferSize = input_buffer_size;
                    portDefn->format.audio.bFlagErrorConcealment = OMX_TRUE;
                    portDefn->format.audio.eEncoding = OMX_AUDIO_CodingAMR;
                    portDefn->format.audio.pNativeRender = 0;
                }
                else if ( 1 == portDefn->nPortIndex )
                {
                    portDefn->eDir =  OMX_DirOutput;
                    portDefn->bEnabled   = m_out_bEnabled;
                    portDefn->bPopulated = m_out_bPopulated;
                    portDefn->nBufferCountActual = m_out_act_buf_count;
                    portDefn->nBufferCountMin    = OMX_CORE_NUM_OUTPUT_BUFFERS;
                    portDefn->nBufferSize = output_buffer_size;
                    portDefn->format.audio.cMIMEType = mime_type;
                    portDefn->format.audio.bFlagErrorConcealment = OMX_TRUE;
                    portDefn->format.audio.eEncoding = OMX_AUDIO_CodingPCM;
                    portDefn->format.audio.pNativeRender = 0;
                }
                else
                {
                    portDefn->eDir =  OMX_DirMax;
                    DEBUG_PRINT_ERROR("Bad Port idx %d\n",
                                      (int)portDefn->nPortIndex);
                    eRet = OMX_ErrorBadPortIndex;
                }

                break;
            }

        case OMX_IndexParamAudioInit:
            {
                OMX_PORT_PARAM_TYPE *portParamType =
                (OMX_PORT_PARAM_TYPE *) paramData;
                DEBUG_PRINT("OMX_IndexParamAudioInit\n");

                portParamType->nVersion.nVersion = OMX_SPEC_VERSION;
                portParamType->nSize = sizeof(portParamType);
                portParamType->nPorts           = 2;
                portParamType->nStartPortNumber = 0;
                break;
            }

        case OMX_IndexParamAudioPortFormat:
            {
                OMX_AUDIO_PARAM_PORTFORMATTYPE *portFormatType =
                (OMX_AUDIO_PARAM_PORTFORMATTYPE *) paramData;
                DEBUG_PRINT("OMX_IndexParamAudioPortFormat\n");
                portFormatType->nVersion.nVersion = OMX_SPEC_VERSION;
                portFormatType->nSize = sizeof(portFormatType);

                if ( OMX_CORE_INPUT_PORT_INDEX == portFormatType->nPortIndex )
                {
                    portFormatType->eEncoding = OMX_AUDIO_CodingAMR;
                }
                else if ( OMX_CORE_OUTPUT_PORT_INDEX== portFormatType->nPortIndex )
                {
                    DEBUG_PRINT("get_parameter:OMX_IndexParamAudioFormat: %lu\n",
                                portFormatType->nIndex);
                    portFormatType->eEncoding = OMX_AUDIO_CodingPCM;
                }
                else
                {
                    DEBUG_PRINT_ERROR("get_parameter: Bad port index %d\n",
                                      (int)portFormatType->nPortIndex);
                    eRet = OMX_ErrorBadPortIndex;
                }
                break;
            }

        case OMX_IndexParamAudioAmr:
            {
                OMX_AUDIO_PARAM_AMRTYPE *amrParam =
                (OMX_AUDIO_PARAM_AMRTYPE *) paramData;
                DEBUG_PRINT("OMX_IndexParamAudioAmr\n");
                if ( OMX_CORE_INPUT_PORT_INDEX== amrParam->nPortIndex )
                {
                    memcpy(amrParam,&m_amr_param,sizeof(OMX_AUDIO_PARAM_AMRTYPE));
                }
                else
                {
                    DEBUG_PRINT_ERROR("get_parameter:OMX_IndexParamAudioAmr "
                                      "portIndex %d\n",
                                      (int)amrParam->nPortIndex);
                    eRet = OMX_ErrorBadPortIndex; }
                break;
            }

        case QOMX_IndexParamAudioAmrWbPlus:
            {
                QOMX_AUDIO_PARAM_AMRWBPLUSTYPE *amrwbPlusParam =
                (QOMX_AUDIO_PARAM_AMRWBPLUSTYPE *) paramData;
                DEBUG_PRINT("OMX_IndexParamAudioAmrWbPlus\n");
                if ( OMX_CORE_INPUT_PORT_INDEX== amrwbPlusParam->nPortIndex )
                {
                    memcpy(amrwbPlusParam,&m_amrwbPlus_param,sizeof(QOMX_AUDIO_PARAM_AMRWBPLUSTYPE));
                }
                else
                {
                    DEBUG_PRINT_ERROR("get_parameter: QOMX_IndexParamAudioAmrWbPlus "
                                      "portIndex %d\n",
                                      (int)amrwbPlusParam->nPortIndex);
                    eRet = OMX_ErrorBadPortIndex; }
                break;
            }

        case OMX_IndexParamAudioPcm:
            {
                OMX_AUDIO_PARAM_PCMMODETYPE *pcmparam =
                (OMX_AUDIO_PARAM_PCMMODETYPE *) paramData;
                if ( OMX_CORE_OUTPUT_PORT_INDEX== pcmparam->nPortIndex )
                {
                    pcmparam->nChannels = m_pcm_param.nChannels;
                    pcmparam->eNumData  =    m_pcm_param.eNumData;
                    pcmparam->bInterleaved  = m_pcm_param.bInterleaved;
                    pcmparam->nBitPerSample = m_pcm_param.nBitPerSample;
                    pcmparam->nSamplingRate = m_pcm_param.nSamplingRate;
                    pcmparam->ePCMMode = m_pcm_param.ePCMMode;
                    pcmparam->eChannelMapping[0] =
                         m_pcm_param.eChannelMapping[0];
                    pcmparam->eChannelMapping[1] =
                        m_pcm_param.eChannelMapping[1] ;
                    DEBUG_PRINT("get_parameter: Sampling rate %lu",
                                pcmparam->nSamplingRate);
                    DEBUG_PRINT("get_parameter: Number of channels %lu",
                                pcmparam->nChannels);
                }
                else
                {
                    DEBUG_PRINT_ERROR("get_parameter:OMX_IndexParamAudioPcm "
                                      "OMX_ErrorBadPortIndex %lu\n",
                                      pcmparam->nPortIndex);
                    eRet = OMX_ErrorBadPortIndex;
                }
                break;
            }
        case OMX_IndexParamComponentSuspended:
        {
            OMX_PARAM_SUSPENSIONTYPE *suspend= (OMX_PARAM_SUSPENSIONTYPE *) paramData;
            if(bSuspendEventRxed)
            {
              suspend->eType = OMX_Suspended;
            }
            else
            {
              suspend->eType = OMX_NotSuspended;
            }
            DEBUG_PRINT("get_parameter: suspend type %d", suspend->eType);

           break;
        }
        case OMX_IndexParamVideoInit:
            {
                OMX_PORT_PARAM_TYPE *portParamType =
                    (OMX_PORT_PARAM_TYPE *) paramData;
                DEBUG_PRINT("get_parameter: OMX_IndexParamVideoInit\n");
                portParamType->nVersion.nVersion = OMX_SPEC_VERSION;
                portParamType->nSize = sizeof(portParamType);
                portParamType->nPorts           = 0;
                portParamType->nStartPortNumber = 0;
                break;
            }
        case OMX_IndexParamPriorityMgmt:
            {
                OMX_PRIORITYMGMTTYPE *priorityMgmtType =
                    (OMX_PRIORITYMGMTTYPE*)paramData;
                DEBUG_PRINT("get_parameter: OMX_IndexParamPriorityMgmt\n");
                priorityMgmtType->nSize = sizeof(priorityMgmtType);
                priorityMgmtType->nVersion.nVersion = OMX_SPEC_VERSION;
                priorityMgmtType->nGroupID = m_priority_mgm.nGroupID;
                priorityMgmtType->nGroupPriority = m_priority_mgm.nGroupPriority;
                break;
            }
        case OMX_IndexParamImageInit:
            {
                OMX_PORT_PARAM_TYPE *portParamType =
                (OMX_PORT_PARAM_TYPE *) paramData;
                DEBUG_PRINT("get_parameter: OMX_IndexParamImageInit\n");
                portParamType->nVersion.nVersion = OMX_SPEC_VERSION;
                portParamType->nSize = sizeof(portParamType);
                portParamType->nPorts           = 0;
                portParamType->nStartPortNumber = 0;
                break;
            }

        case OMX_IndexParamCompBufferSupplier:
            {
                DEBUG_PRINT("get_parameter: OMX_IndexParamCompBufferSupplier\n");
                OMX_PARAM_BUFFERSUPPLIERTYPE *bufferSupplierType
                = (OMX_PARAM_BUFFERSUPPLIERTYPE*) paramData;
                DEBUG_PRINT("get_parameter: OMX_IndexParamCompBufferSupplier\n");

                bufferSupplierType->nSize = sizeof(bufferSupplierType);
                bufferSupplierType->nVersion.nVersion = OMX_SPEC_VERSION;
                if ( OMX_CORE_INPUT_PORT_INDEX   ==
                     bufferSupplierType->nPortIndex )
                {
                    bufferSupplierType->nPortIndex = OMX_BufferSupplyUnspecified;
                }
                else if ( OMX_CORE_OUTPUT_PORT_INDEX ==
                          bufferSupplierType->nPortIndex )
                {
                    bufferSupplierType->nPortIndex = OMX_BufferSupplyUnspecified;
                }
                else
                {
                    DEBUG_PRINT_ERROR("get_parameter:"
                                      "OMX_IndexParamCompBufferSupplier eRet %08x\n",
                                      eRet);
                    eRet = OMX_ErrorBadPortIndex;
                }
                break;
            }

            /*Component should support this port definition*/
        case OMX_IndexParamOtherInit:
            {
                OMX_PORT_PARAM_TYPE *portParamType =
                    (OMX_PORT_PARAM_TYPE *) paramData;
                DEBUG_PRINT("get_parameter: OMX_IndexParamOtherInit\n");
                portParamType->nVersion.nVersion = OMX_SPEC_VERSION;
                portParamType->nSize = sizeof(portParamType);
                portParamType->nPorts           = 0;
                portParamType->nStartPortNumber = 0;
                break;
            }
         case QOMX_IndexParamAudioSessionId:
        {
           QOMX_AUDIO_STREAM_INFO_DATA *streaminfoparam =
                   (QOMX_AUDIO_STREAM_INFO_DATA *) paramData;
           streaminfoparam->sessionId = m_session_id;
               break;
         }
        default:
            {
                DEBUG_PRINT_ERROR("unknown param %08x\n", paramIndex);
                eRet = OMX_ErrorUnsupportedIndex;
            }
    }
    return eRet;

}

/**
 @brief member function that set paramter from IL client

 @param hComp handle to component instance
 @param paramIndex parameter type
 @param paramData pointer to memory space which holds the paramter
 @return error status
 */
OMX_ERRORTYPE  omx_amr_adec::set_parameter(OMX_IN OMX_HANDLETYPE     hComp,
                                           OMX_IN OMX_INDEXTYPE paramIndex,
                                           OMX_IN OMX_PTR        paramData)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    if(hComp == NULL)
    {
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if ( OMX_StateInvalid == m_state )
    {
        DEBUG_PRINT_ERROR("Set Param in Invalid State\n");
        return OMX_ErrorInvalidState;
    }
    if ( NULL == paramData )
    {
        DEBUG_PRINT("param data is NULL");
        return OMX_ErrorBadParameter;
    }

    switch ( paramIndex )
    {
        case OMX_IndexParamAudioAmr:
            {
                DEBUG_PRINT("OMX_IndexParamAudioAmr");
                OMX_AUDIO_PARAM_AMRTYPE *amr_param =
                                      ((OMX_AUDIO_PARAM_AMRTYPE *) paramData);
                memcpy(&m_amr_param,amr_param,sizeof(OMX_AUDIO_PARAM_AMRTYPE));
                break;
            }

        case QOMX_IndexParamAudioAmrWbPlus:
            {
                DEBUG_PRINT("QOMX_IndexParamAudioAmrWbPlus");
                QOMX_AUDIO_PARAM_AMRWBPLUSTYPE *amrwbPlus_param =
                                ((QOMX_AUDIO_PARAM_AMRWBPLUSTYPE *) paramData);
                memcpy(&m_amrwbPlus_param,amrwbPlus_param,sizeof(QOMX_AUDIO_PARAM_AMRWBPLUSTYPE));
                break;
            }

        case OMX_IndexParamPortDefinition:
            {
                OMX_PARAM_PORTDEFINITIONTYPE *portDefn;
                portDefn = (OMX_PARAM_PORTDEFINITIONTYPE *) paramData;

                if ( ((m_state == OMX_StateLoaded)&&
                      !BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING))
                     || (m_state == OMX_StateWaitForResources &&
                         ((OMX_DirInput == portDefn->eDir && m_inp_bEnabled == true)||
                          (OMX_DirInput == portDefn->eDir && m_out_bEnabled == true)))
                     ||(((OMX_DirInput == portDefn->eDir && m_inp_bEnabled == false)||
                         (OMX_DirInput == portDefn->eDir && m_out_bEnabled == false)) &&
                        (m_state != OMX_StateWaitForResources)) )
                {
                    DEBUG_PRINT("Set Parameter called in valid state\n");
                }
                else
                {
                    DEBUG_PRINT_ERROR("Set Parameter called in Invalid State\n");
                    return OMX_ErrorIncorrectStateOperation;
                }
                DEBUG_PRINT("OMX_IndexParamPortDefinition nPortIndex = %lu\n",
                            portDefn->nPortIndex);
                if ( OMX_CORE_INPUT_PORT_INDEX == portDefn->nPortIndex )
                {
                    if ( portDefn->nBufferCountActual > OMX_CORE_NUM_INPUT_BUFFERS )
                    {
                        m_inp_act_buf_count = portDefn->nBufferCountActual;
                    }
                    else
                    {
                        m_inp_act_buf_count =OMX_CORE_NUM_INPUT_BUFFERS;
                    }
                    input_buffer_size = portDefn->nBufferSize;

                }
                else if ( OMX_CORE_OUTPUT_PORT_INDEX == portDefn->nPortIndex )
                {
                    if ( portDefn->nBufferCountActual >
                         OMX_CORE_NUM_OUTPUT_BUFFERS )
                    {
                        m_out_act_buf_count = portDefn->nBufferCountActual;
                    }
                    else
                    {
                        m_out_act_buf_count =OMX_CORE_NUM_OUTPUT_BUFFERS;
                    }
                    output_buffer_size = portDefn->nBufferSize;
                }
                else
                {
                    DEBUG_PRINT(" set_parameter: Bad Port idx %d",
                                (int)portDefn->nPortIndex);
                    eRet = OMX_ErrorBadPortIndex;
                }
                break;
            }
        case OMX_IndexParamPriorityMgmt:
            {
                DEBUG_PRINT("set_parameter: OMX_IndexParamPriorityMgmt\n");
                if ( m_state != OMX_StateLoaded )
                {
                    DEBUG_PRINT_ERROR("Set Parameter called in Invalid State\n");
                    return OMX_ErrorIncorrectStateOperation;
                }
                OMX_PRIORITYMGMTTYPE *priorityMgmtype
                = (OMX_PRIORITYMGMTTYPE*) paramData;
                DEBUG_PRINT("set_parameter: OMX_IndexParamPriorityMgmt %lu\n",
                            priorityMgmtype->nGroupID);

                DEBUG_PRINT("set_parameter: priorityMgmtype %lu\n",
                            priorityMgmtype->nGroupPriority);

                m_priority_mgm.nGroupID = priorityMgmtype->nGroupID;
                m_priority_mgm.nGroupPriority = priorityMgmtype->nGroupPriority;
                break;
            }
        case  OMX_IndexParamAudioPortFormat:
            {
                OMX_AUDIO_PARAM_PORTFORMATTYPE *portFormatType =
                (OMX_AUDIO_PARAM_PORTFORMATTYPE *) paramData;
                DEBUG_PRINT("set_parameter: OMX_IndexParamAudioPortFormat\n");

                if ( OMX_CORE_INPUT_PORT_INDEX== portFormatType->nPortIndex )
                {
                    portFormatType->eEncoding = OMX_AUDIO_CodingAMR;
                }
                else if ( OMX_CORE_OUTPUT_PORT_INDEX == portFormatType->nPortIndex )
                {
                    DEBUG_PRINT("set_parameter: OMX_IndexParamAudioFormat: %lu\n",
                                portFormatType->nIndex);
                    portFormatType->eEncoding = OMX_AUDIO_CodingPCM;
                }
                else
                {
                    DEBUG_PRINT_ERROR("set_parameter: Bad port index %d\n",
                                      (int)portFormatType->nPortIndex);
                    eRet = OMX_ErrorBadPortIndex;
                }
                break;
            }

        case OMX_IndexParamCompBufferSupplier:
            {
                DEBUG_PRINT("set_parameter: OMX_IndexParamCompBufferSupplier\n");
                OMX_PARAM_BUFFERSUPPLIERTYPE *bufferSupplierType
                = (OMX_PARAM_BUFFERSUPPLIERTYPE*) paramData;
                DEBUG_PRINT("set_parameter: OMX_IndexParamCompBufferSupplier %d\n",
                            bufferSupplierType->eBufferSupplier);

                if ( (bufferSupplierType->nPortIndex == OMX_CORE_INPUT_PORT_INDEX)
                    || (bufferSupplierType->nPortIndex ==
                        OMX_CORE_OUTPUT_PORT_INDEX) ) {
                    DEBUG_PRINT("set_parameter: OMX_IndexParamCompBufferSupplier "
                                "In/Out put \n");
                    m_buffer_supplier.eBufferSupplier =
                        bufferSupplierType->eBufferSupplier;
                }
                else
                {
                    DEBUG_PRINT_ERROR("set_parameter:OMX_IndexParamCompBufferSupplier: "
                                      "eRet  %08x\n", eRet);
                    eRet = OMX_ErrorBadPortIndex;
                }

                break;
            }

        case OMX_IndexParamAudioPcm:
            {
                DEBUG_PRINT("set_parameter: OMX_IndexParamAudioPcm\n");
                OMX_AUDIO_PARAM_PCMMODETYPE *pcmparam
                = (OMX_AUDIO_PARAM_PCMMODETYPE *) paramData;

                if ( OMX_CORE_OUTPUT_PORT_INDEX== pcmparam->nPortIndex )
                {
                    m_pcm_param.nChannels =  pcmparam->nChannels;
                    m_pcm_param.eNumData = pcmparam->eNumData;
                    m_pcm_param.bInterleaved = pcmparam->bInterleaved;
                    m_pcm_param.nBitPerSample =   pcmparam->nBitPerSample;
                    m_pcm_param.nSamplingRate =   pcmparam->nSamplingRate;
                    m_pcm_param.ePCMMode      =  pcmparam->ePCMMode;
                    m_pcm_param.eChannelMapping[0] =
                        pcmparam->eChannelMapping[0];
                    m_pcm_param.eChannelMapping[1] =
                        pcmparam->eChannelMapping[1];
                    DEBUG_PRINT("set_parameter: Sampling rate %lu",
                                pcmparam->nSamplingRate);
                    DEBUG_PRINT("set_parameter: Number of channels %lu",
                                pcmparam->nChannels);
                }
                else
                {
                    DEBUG_PRINT_ERROR("get_parameter:OMX_IndexParamAudioPcm "
                                      "OMX_ErrorBadPortIndex %d\n",
                                      (int)pcmparam->nPortIndex);
                    eRet = OMX_ErrorBadPortIndex;
                }
                break;
            }
        case OMX_IndexParamSuspensionPolicy:
            {
                OMX_PARAM_SUSPENSIONPOLICYTYPE *suspend_policy;
                suspend_policy = (OMX_PARAM_SUSPENSIONPOLICYTYPE*)paramData;
                suspensionPolicy= suspend_policy->ePolicy;
                DEBUG_PRINT("SET_PARAMETER: Set SUSPENSION POLICY %d  m_ipc_to_event_th=%p\n",
                                    suspensionPolicy,m_ipc_to_event_th);
                break;
            }
        case OMX_IndexParamStandardComponentRole:
            {
                OMX_PARAM_COMPONENTROLETYPE *componentRole;
                componentRole = (OMX_PARAM_COMPONENTROLETYPE*)paramData;
                component_Role.nSize = componentRole->nSize;
                component_Role.nVersion = componentRole->nVersion;
                strcpy((char *)component_Role.cRole,
                       (const char*)componentRole->cRole);
                break;
            }

        default:
            {
                DEBUG_PRINT_ERROR("unknown param %d\n", paramIndex);
                eRet = OMX_ErrorUnsupportedIndex;
            }
    }
    return eRet;
}

/* ======================================================================
FUNCTION
  omx_amr_adec::GetConfig

DESCRIPTION
  OMX Get Config Method implementation.

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if successful.

========================================================================== */
OMX_ERRORTYPE  omx_amr_adec::get_config(OMX_IN OMX_HANDLETYPE      hComp,
                                        OMX_IN OMX_INDEXTYPE configIndex,
                                        OMX_INOUT OMX_PTR     configData)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    if(hComp == NULL)
    {
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if ( OMX_StateInvalid == m_state )
    {
        DEBUG_PRINT_ERROR("Get Config in Invalid State\n");
        return OMX_ErrorInvalidState;
    }

    switch ( configIndex )
    {
        case OMX_IndexConfigAudioVolume:
            {
                OMX_AUDIO_CONFIG_VOLUMETYPE *volume =
                    (OMX_AUDIO_CONFIG_VOLUMETYPE*) configData;

                if ( volume->nPortIndex == OMX_CORE_INPUT_PORT_INDEX )
                {
                    volume->nSize = sizeof(volume);
                    volume->nVersion.nVersion = OMX_SPEC_VERSION;
                    volume->bLinear = OMX_TRUE;
                    volume->sVolume.nValue = m_volume;
                    volume->sVolume.nMax   = OMX_ADEC_MAX;
                    volume->sVolume.nMin   = OMX_ADEC_MIN;
                }
                else
                {
                    eRet = OMX_ErrorBadPortIndex;
                }
            }
            break;

        case OMX_IndexConfigAudioMute:
            {
                OMX_AUDIO_CONFIG_MUTETYPE *mute =
                    (OMX_AUDIO_CONFIG_MUTETYPE*) configData;

                if ( mute->nPortIndex == OMX_CORE_INPUT_PORT_INDEX )
                {
                    mute->nSize = sizeof(mute);
                    mute->nVersion.nVersion = OMX_SPEC_VERSION;
                    mute->bMute = (BITMASK_PRESENT(&m_flags, OMX_COMPONENT_MUTED)
                                   ? OMX_TRUE : OMX_FALSE);
                }
                else
                {
                    eRet = OMX_ErrorBadPortIndex;
                }
            }
            break;

        default:
            eRet = OMX_ErrorUnsupportedIndex;
            break;
    }
    return eRet;
}

/* ======================================================================
FUNCTION
  omx_amr_adec::SetConfig

DESCRIPTION
  OMX Set Config method implementation

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if successful.
========================================================================== */
OMX_ERRORTYPE  omx_amr_adec::set_config(OMX_IN OMX_HANDLETYPE      hComp,
                                        OMX_IN OMX_INDEXTYPE configIndex,
                                        OMX_IN OMX_PTR        configData)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    if(hComp == NULL)
    {
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if ( OMX_StateInvalid == m_state )
    {
        DEBUG_PRINT_ERROR("Set Config in Invalid State\n");
        return OMX_ErrorInvalidState;
    }
    if ( OMX_StateExecuting == m_state )
    {
        DEBUG_PRINT_ERROR("set_config:Ignore in Exe state\n");
        return OMX_ErrorInvalidState;
    }

    switch ( configIndex )
    {
        case OMX_IndexConfigAudioVolume:
            {
                OMX_AUDIO_CONFIG_VOLUMETYPE *vol = (OMX_AUDIO_CONFIG_VOLUMETYPE*)
                                                   configData;
                if ( vol->nPortIndex == OMX_CORE_INPUT_PORT_INDEX )
                {
                    if ( (vol->sVolume.nValue <= OMX_ADEC_MAX) &&
                         (vol->sVolume.nValue >= OMX_ADEC_MIN) )
                    {
                        m_volume = vol->sVolume.nValue;
                        if ( BITMASK_ABSENT(&m_flags, OMX_COMPONENT_MUTED) )
                        {
                            /* ioctl(m_drv_fd, AUDIO_VOLUME,
                                     m_volume * OMX_ADEC_VOLUME_STEP); */
                        }

                    }
                    else
                    {
                        eRet = OMX_ErrorBadParameter;
                    }
                }
                else
                {
                    eRet = OMX_ErrorBadPortIndex;
                }
            }
            break;

        case OMX_IndexConfigAudioMute:
            {
                OMX_AUDIO_CONFIG_MUTETYPE *mute = (OMX_AUDIO_CONFIG_MUTETYPE*)
                                                  configData;
                if ( mute->nPortIndex == OMX_CORE_INPUT_PORT_INDEX )
                {
                    if ( mute->bMute == OMX_TRUE )
                    {
                        BITMASK_SET(&m_flags, OMX_COMPONENT_MUTED);
                        /* ioctl(m_drv_fd, AUDIO_VOLUME, 0); */
                    }
                    else
                    {
                        BITMASK_CLEAR(&m_flags, OMX_COMPONENT_MUTED);
                        /* ioctl(m_drv_fd, AUDIO_VOLUME,
                                 m_volume * OMX_ADEC_VOLUME_STEP); */
                    }
                }
                else
                {
                    eRet = OMX_ErrorBadPortIndex;
                }
            }
            break;

        default:
            eRet = OMX_ErrorUnsupportedIndex;
            break;
    }
    return eRet;
}

/* ======================================================================
FUNCTION
  omx_amr_adec::GetExtensionIndex

DESCRIPTION
  OMX GetExtensionIndex method implementaion.  <TBD>

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  omx_amr_adec::get_extension_index
(
    OMX_IN OMX_HANDLETYPE      hComp,
    OMX_IN OMX_STRING      paramName,
    OMX_OUT OMX_INDEXTYPE* indexType
)
{
    if((hComp == NULL) || (paramName == NULL) || (indexType == NULL))
    {
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }

    if ( OMX_StateInvalid == m_state )
    {
        DEBUG_PRINT_ERROR("Get Extension Index in Invalid State\n");
        return OMX_ErrorInvalidState;
    }

    if(strncmp(paramName,"OMX.Qualcomm.index.audio.amrwbplus",strlen("OMX.Qualcomm.index.audio.amrwbplus")) == 0)
    {
        *indexType =(OMX_INDEXTYPE)QOMX_IndexParamAudioAmrWbPlus;
        printf("indexType = %d\n",*indexType);
    }
    else
    {
         return OMX_ErrorBadParameter;
    }
    return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
  omx_amr_adec::GetState

DESCRIPTION
  Returns the state information back to the caller.<TBD>

PARAMETERS
  <TBD>.

RETURN VALUE
  Error None if everything is successful.
========================================================================== */
OMX_ERRORTYPE  omx_amr_adec::get_state(OMX_IN OMX_HANDLETYPE  hComp,
                                       OMX_OUT OMX_STATETYPE* state)
{
    if(hComp == NULL)
    {
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    *state = m_state;
    DEBUG_PRINT("Returning the state %d\n",*state);
    return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
  omx_amr_adec::ComponentTunnelRequest

DESCRIPTION
  OMX Component Tunnel Request method implementation. <TBD>

PARAMETERS
  None.

RETURN VALUE
  OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  omx_amr_adec::component_tunnel_request
(
    OMX_IN OMX_HANDLETYPE           hComp,
    OMX_IN OMX_U32                  port,
    OMX_IN OMX_HANDLETYPE           peerComponent,
    OMX_IN OMX_U32                  peerPort,
    OMX_INOUT OMX_TUNNELSETUPTYPE*  tunnelSetup
)
{
    DEBUG_PRINT_ERROR("Error: component_tunnel_request Not Implemented\n");
    if((hComp == NULL) || (peerComponent == NULL) || (tunnelSetup == NULL))
    {
        port = 0;
        peerPort = 0;
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    return OMX_ErrorNotImplemented;
}


/* ======================================================================
FUNCTION
  omx_amr_adec::AllocateInputBuffer

DESCRIPTION
  Helper function for allocate buffer in the input pin

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_amr_adec::allocate_input_buffer
(
    OMX_IN OMX_HANDLETYPE            hComp,
    OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
    OMX_IN OMX_U32                   port,
    OMX_IN OMX_PTR                   appData,
    OMX_IN OMX_U32                   bytes
)
{
    OMX_ERRORTYPE             eRet = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE      *bufHdr; // buffer header
    unsigned                  nBufSize = MAX(bytes, input_buffer_size);
    char                      *buf_ptr;

    if ( m_inp_current_buf_count < m_inp_act_buf_count )
    {
        buf_ptr = (char *) calloc( (nBufSize + sizeof(OMX_BUFFERHEADERTYPE)+
                                    sizeof(META_IN) ) , 1);
        if(hComp == NULL)
        {
            port = 0;
            DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
            free(buf_ptr);
            return OMX_ErrorBadParameter;
        }
        if ( buf_ptr != NULL )
        {
            bufHdr = (OMX_BUFFERHEADERTYPE *) buf_ptr;
            *bufferHdr = bufHdr;
            memset(bufHdr,0,sizeof(OMX_BUFFERHEADERTYPE));

            bufHdr->pBuffer = (OMX_U8 *)((buf_ptr) + sizeof(META_IN) + \
                                         sizeof(OMX_BUFFERHEADERTYPE));
            DEBUG_PRINT("bufHdr %p bufHdr->pBuffer %p", bufHdr, bufHdr->pBuffer);
            bufHdr->nSize             = sizeof(OMX_BUFFERHEADERTYPE);
            bufHdr->nVersion.nVersion = OMX_SPEC_VERSION;
            bufHdr->nAllocLen         = nBufSize;
            bufHdr->pAppPrivate       = appData;
            bufHdr->nInputPortIndex   = OMX_CORE_INPUT_PORT_INDEX;
            m_input_buf_hdrs.insert(bufHdr, NULL);
            m_inp_current_buf_count++;
            DEBUG_PRINT("AIB:bufHdr %p bufHdr->pBuffer %p"
                        "m_inp_buf_cnt=%d bytes=%lu", \
                        bufHdr, bufHdr->pBuffer,m_inp_current_buf_count, bytes);
        }

    }
    else
    {
        DEBUG_PRINT("Input buffer memory allocation failed 1 \n");
        eRet =  OMX_ErrorInsufficientResources;
    }
    return eRet;
}

OMX_ERRORTYPE  omx_amr_adec::allocate_output_buffer
(
    OMX_IN OMX_HANDLETYPE               hComp,
    OMX_INOUT OMX_BUFFERHEADERTYPE**    bufferHdr,
    OMX_IN OMX_U32                      port,
    OMX_IN OMX_PTR                      appData,
    OMX_IN OMX_U32                      bytes
)
{
    OMX_ERRORTYPE           eRet = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE    *bufHdr; // buffer header
    unsigned                nBufSize = MAX(bytes,output_buffer_size);
    char                    *buf_ptr;

    if(hComp == NULL)
    {
        port = 0;
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if ( m_out_current_buf_count < m_out_act_buf_count )
    {
        buf_ptr = (char *) calloc( (nBufSize + sizeof(OMX_BUFFERHEADERTYPE) +
                                    sizeof(META_OUT) ) , 1);

        if ( buf_ptr != NULL )
        {
            bufHdr = (OMX_BUFFERHEADERTYPE *) buf_ptr;
            *bufferHdr = bufHdr;
            memset(bufHdr,0,sizeof(OMX_BUFFERHEADERTYPE));

            bufHdr->pBuffer = (OMX_U8 *)((buf_ptr) + sizeof(META_OUT) +
                                         sizeof(OMX_BUFFERHEADERTYPE));
            DEBUG_PRINT("bufHdr %p pBuffer %p", bufHdr, bufHdr->pBuffer);
            bufHdr->nSize             = sizeof(OMX_BUFFERHEADERTYPE);
            bufHdr->nVersion.nVersion = OMX_SPEC_VERSION;
            bufHdr->nAllocLen         = nBufSize;
            bufHdr->pAppPrivate       = appData;
            bufHdr->nOutputPortIndex  = OMX_CORE_OUTPUT_PORT_INDEX;
            m_output_buf_hdrs.insert(bufHdr, NULL);
            m_out_current_buf_count++;
            DEBUG_PRINT("AOB::bufHdr %p bufHdr->pBuffer %p "
                        "m_out_buf_cnt=%d bytes=%lu", \
                        bufHdr, bufHdr->pBuffer,m_out_current_buf_count, bytes);
        }
        else
        {
            DEBUG_PRINT("Output buffer memory allocation failed 1 \n");
            eRet =  OMX_ErrorInsufficientResources;
        }
    }
    else
    {
        DEBUG_PRINT("Output buffer memory allocation failed 2 \n");
        eRet =  OMX_ErrorInsufficientResources;
    }
    return eRet;
}

/* ======================================================================
FUNCTION
  omx_amr_adec::AllocateBuffer

DESCRIPTION
  Returns zero if all the buffers released..

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_amr_adec::allocate_buffer
(
    OMX_IN OMX_HANDLETYPE               hComp,
    OMX_INOUT OMX_BUFFERHEADERTYPE**    bufferHdr,
    OMX_IN OMX_U32                      port,
    OMX_IN OMX_PTR                      appData,
    OMX_IN OMX_U32                      bytes
)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone; // OMX return type

    if ( m_state == OMX_StateInvalid )
    {
        DEBUG_PRINT_ERROR("Allocate Buf in Invalid State\n");
        return OMX_ErrorInvalidState;
    }
    // What if the client calls again.
    if ( port == OMX_CORE_INPUT_PORT_INDEX )
    {
        eRet = allocate_input_buffer(hComp,bufferHdr,port,appData,bytes);
    }
    else if ( port == OMX_CORE_OUTPUT_PORT_INDEX )
    {
        eRet = allocate_output_buffer(hComp,bufferHdr,port,appData,bytes);
    }
    else
    {
        DEBUG_PRINT_ERROR("Error: Invalid Port Index received %d\n",
                          (int)port);
        eRet = OMX_ErrorBadPortIndex;
    }

    if ( (eRet == OMX_ErrorNone) )
    {
        DEBUG_PRINT("allocate_buffer:  before allocate_done \n");
        if ( allocate_done() )
        {
            DEBUG_PRINT("allocate_buffer:  after allocate_done \n");
            m_is_alloc_buf++;
            if ( BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING) )
            {
                m_residual_data_len=0;
                BITMASK_CLEAR(&m_flags, OMX_COMPONENT_IDLE_PENDING);
                post_command(OMX_CommandStateSet,OMX_StateIdle,
                             OMX_COMPONENT_GENERATE_EVENT);
                DEBUG_PRINT("allocate_buffer:  post idle transition event \n");
            }
            DEBUG_PRINT("allocate_buffer:  complete \n");
        }

        if ( port == OMX_CORE_INPUT_PORT_INDEX && m_inp_bPopulated )
        {
            if ( BITMASK_PRESENT(&m_flags,OMX_COMPONENT_INPUT_ENABLE_PENDING) )
            {
                BITMASK_CLEAR((&m_flags),OMX_COMPONENT_INPUT_ENABLE_PENDING);
                post_command(OMX_CommandPortEnable, OMX_CORE_INPUT_PORT_INDEX,
                             OMX_COMPONENT_GENERATE_EVENT);
            }
        }
        if ( port == OMX_CORE_OUTPUT_PORT_INDEX && m_out_bPopulated )
        {
            if ( BITMASK_PRESENT(&m_flags,OMX_COMPONENT_OUTPUT_ENABLE_PENDING) )
            {
                BITMASK_CLEAR((&m_flags),OMX_COMPONENT_OUTPUT_ENABLE_PENDING);
                m_out_bEnabled = OMX_TRUE;

                DEBUG_PRINT("AllocBuf-->is_out_th_sleep=%d\n",is_out_th_sleep);
                pthread_mutex_lock(&m_out_th_lock_1);
                if ( is_out_th_sleep )
                {
                    is_out_th_sleep = false;
                    DEBUG_DETAIL("AllocBuf:WAKING UP OUT THREADS\n");
                    out_th_wakeup();
                }
                pthread_mutex_unlock(&m_out_th_lock_1);
                pthread_mutex_lock(&m_in_th_lock_1);
                if(is_in_th_sleep)
                {
                   is_in_th_sleep = false;
                   DEBUG_DETAIL("AB:WAKING UP IN THREADS\n");
                   in_th_wakeup();
                }
                pthread_mutex_unlock(&m_in_th_lock_1);
                post_command(OMX_CommandPortEnable, OMX_CORE_OUTPUT_PORT_INDEX,
                             OMX_COMPONENT_GENERATE_EVENT);
            }
        }
    }
    DEBUG_PRINT("Allocate Buffer exit with ret Code %d\n", eRet);
    return eRet;
}

/*=============================================================================
FUNCTION:
  use_buffer

DESCRIPTION:
  OMX Use Buffer method implementation.

INPUT/OUTPUT PARAMETERS:
  [INOUT] bufferHdr
  [IN] hComp
  [IN] port
  [IN] appData
  [IN] bytes
  [IN] buffer

RETURN VALUE:
  OMX_ERRORTYPE

Dependency:
  None

SIDE EFFECTS:
  None
=============================================================================*/
OMX_ERRORTYPE  omx_amr_adec::use_buffer(
                                       OMX_IN OMX_HANDLETYPE            hComp,
                                       OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                                       OMX_IN OMX_U32                   port,
                                       OMX_IN OMX_PTR                   appData,
                                       OMX_IN OMX_U32                   bytes,
                                       OMX_IN OMX_U8*                   buffer)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;

    if ( OMX_CORE_INPUT_PORT_INDEX == port )
    {
        eRet = use_input_buffer(hComp,bufferHdr,port,appData,bytes,buffer);
    }
    else if ( OMX_CORE_OUTPUT_PORT_INDEX == port )
    {
        eRet = use_output_buffer(hComp,bufferHdr,port,appData,bytes,buffer);
    }
    else
    {
        DEBUG_PRINT_ERROR("Error: Invalid Port Index received %d\n",(int)port);
        eRet = OMX_ErrorBadPortIndex;
    }


    if ( (eRet == OMX_ErrorNone) )
    {
        DEBUG_PRINT("Checking for Output Allocate buffer Done");
        if ( allocate_done() )
        {
            if ( BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING) )
            {
                m_residual_data_len=0;
                BITMASK_CLEAR(&m_flags, OMX_COMPONENT_IDLE_PENDING);
                post_command(OMX_CommandStateSet,OMX_StateIdle,
                             OMX_COMPONENT_GENERATE_EVENT);
            }
        }

        if ( port == OMX_CORE_INPUT_PORT_INDEX && m_inp_bPopulated )
        {
            if ( BITMASK_PRESENT(&m_flags,OMX_COMPONENT_INPUT_ENABLE_PENDING) )
            {
                BITMASK_CLEAR((&m_flags),OMX_COMPONENT_INPUT_ENABLE_PENDING);
                post_command(OMX_CommandPortEnable, OMX_CORE_INPUT_PORT_INDEX,
                             OMX_COMPONENT_GENERATE_EVENT);

            }
        }
        if ( port == OMX_CORE_OUTPUT_PORT_INDEX && m_out_bPopulated )
        {
            if ( BITMASK_PRESENT(&m_flags,OMX_COMPONENT_OUTPUT_ENABLE_PENDING) )
            {
                BITMASK_CLEAR((&m_flags),OMX_COMPONENT_OUTPUT_ENABLE_PENDING);
                post_command(OMX_CommandPortEnable, OMX_CORE_OUTPUT_PORT_INDEX,
                             OMX_COMPONENT_GENERATE_EVENT);
                pthread_mutex_lock(&m_out_th_lock_1);
                if ( is_out_th_sleep )
                {
                    is_out_th_sleep = false;
                    DEBUG_DETAIL("UseBuf:WAKING UP OUT THREADS\n");
                    out_th_wakeup();
                }
                pthread_mutex_unlock(&m_out_th_lock_1);
                pthread_mutex_lock(&m_in_th_lock_1);
                if(is_in_th_sleep)
                {
                   is_in_th_sleep = false;
                   DEBUG_DETAIL("UB:WAKING UP IN THREADS\n");
                   in_th_wakeup();
                }
                pthread_mutex_unlock(&m_in_th_lock_1);
            }
        }
    }
    DEBUG_PRINT("Use Buffer for port%lu\n", port);
    return eRet;
}

/*=============================================================================
FUNCTION:
  use_input_buffer

DESCRIPTION:
  Helper function for Use buffer in the input pin

INPUT/OUTPUT PARAMETERS:
  [INOUT] bufferHdr
  [IN] hComp
  [IN] port
  [IN] appData
  [IN] bytes
  [IN] buffer

RETURN VALUE:
  OMX_ERRORTYPE

Dependency:
  None

SIDE EFFECTS:
  None
=============================================================================*/
OMX_ERRORTYPE  omx_amr_adec::use_input_buffer
(
    OMX_IN OMX_HANDLETYPE            hComp,
    OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
    OMX_IN OMX_U32                   port,
    OMX_IN OMX_PTR                   appData,
    OMX_IN OMX_U32                   bytes,
    OMX_IN OMX_U8*                   buffer
)
{
    OMX_ERRORTYPE         eRet = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE            *bufHdr;
    unsigned                   nBufSize = MAX(bytes, input_buffer_size);
    char                       *buf_ptr;

    if(hComp == NULL)
    {
        port = 0;
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if(bytes < input_buffer_size)
    {
      /* return if i\p buffer size provided by client
       is less than min i\p buffer size supported by omx component*/
      return OMX_ErrorInsufficientResources;
    }
    DEBUG_PRINT("Inside omx_amr_adec::use_input_buffer");
    DEBUG_PRINT("use_input_buffer: input_buffer_size %d bytes %lu\n",
                input_buffer_size,bytes);
    if ( m_inp_current_buf_count < m_inp_act_buf_count )
    {

        buf_ptr = (char *) calloc(sizeof(OMX_BUFFERHEADERTYPE), 1);

        if ( buf_ptr != NULL )
        {
            bufHdr = (OMX_BUFFERHEADERTYPE *) buf_ptr;
            *bufferHdr = bufHdr;
            memset(bufHdr,0,sizeof(OMX_BUFFERHEADERTYPE));

            bufHdr->pBuffer           = (OMX_U8 *)(buffer);
            DEBUG_PRINT("use_input_buffer:bufHdr %p bufHdr->pBuffer %p bytes=%lu",
                        bufHdr, bufHdr->pBuffer,bytes);
            bufHdr->nSize             = sizeof(OMX_BUFFERHEADERTYPE);
            bufHdr->nVersion.nVersion = OMX_SPEC_VERSION;
            bufHdr->nAllocLen         = nBufSize;
            input_buffer_size         = nBufSize;
            bufHdr->pAppPrivate       = appData;
            bufHdr->nInputPortIndex   = OMX_CORE_INPUT_PORT_INDEX;
            bufHdr->nOffset           = 0;
            m_input_buf_hdrs.insert(bufHdr, NULL);
            m_inp_current_buf_count++;
        }
        else
        {
            DEBUG_PRINT("Input buffer memory allocation failed 1 \n");
            eRet =  OMX_ErrorInsufficientResources;
        }
    }
    else
    {
        DEBUG_PRINT("Input buffer memory allocation failed\n");
        eRet =  OMX_ErrorInsufficientResources;
    }
    return eRet;
}

/*=============================================================================
FUNCTION:
  use_output_buffer

DESCRIPTION:
  Helper function for Use buffer in the output pin

INPUT/OUTPUT PARAMETERS:
  [INOUT] bufferHdr
  [IN] hComp
  [IN] port
  [IN] appData
  [IN] bytes
  [IN] buffer

RETURN VALUE:
  OMX_ERRORTYPE

Dependency:
  None

SIDE EFFECTS:
  None
=============================================================================*/
OMX_ERRORTYPE  omx_amr_adec::use_output_buffer
(
    OMX_IN OMX_HANDLETYPE            hComp,
    OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
    OMX_IN OMX_U32                   port,
    OMX_IN OMX_PTR                   appData,
    OMX_IN OMX_U32                   bytes,
    OMX_IN OMX_U8*                   buffer
)
{
    OMX_ERRORTYPE         eRet = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE  *bufHdr;
    unsigned              nBufSize = MAX(bytes,output_buffer_size);
    char                  *buf_ptr;

    if(hComp == NULL)
    {
        port = 0;
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if ( bytes < output_buffer_size )
    {
        /* return if o\p buffer size provided by client
        is less than min o\p buffer size supported by omx component*/
        return OMX_ErrorInsufficientResources;
    }

    DEBUG_PRINT("Inside omx_amr_adec::use_output_buffer");
    if ( m_out_current_buf_count < m_out_act_buf_count )
    {
        buf_ptr = (char *) calloc(sizeof(OMX_BUFFERHEADERTYPE), 1);

        if ( buf_ptr != NULL )
        {
            bufHdr = (OMX_BUFFERHEADERTYPE *) buf_ptr;
            DEBUG_PRINT("BufHdr=%p buffer=%p\n",bufHdr,buffer);
            *bufferHdr = bufHdr;
            memset(bufHdr,0,sizeof(OMX_BUFFERHEADERTYPE));

            bufHdr->pBuffer           = (OMX_U8 *)(buffer);
            DEBUG_PRINT("use_output_buffer:bufHdr %p bufHdr->pBuffer %p len=%lu",
                        bufHdr, bufHdr->pBuffer,bytes);
            bufHdr->nSize             = sizeof(OMX_BUFFERHEADERTYPE);
            bufHdr->nVersion.nVersion = OMX_SPEC_VERSION;
            bufHdr->nAllocLen         = nBufSize;
            output_buffer_size        = nBufSize;
            bufHdr->pAppPrivate       = appData;
            bufHdr->nOutputPortIndex  = OMX_CORE_OUTPUT_PORT_INDEX;
            bufHdr->nOffset           = 0;
            m_output_buf_hdrs.insert(bufHdr, NULL);
            m_out_current_buf_count++;

        }
        else
        {
            DEBUG_PRINT("Output buffer memory allocation failed\n");
            eRet =  OMX_ErrorInsufficientResources;
        }
    }
    else
    {
        DEBUG_PRINT("Output buffer memory allocation failed 2\n");
        eRet =  OMX_ErrorInsufficientResources;
    }
    return eRet;
}

/**
 @brief member function that searches for caller buffer

 @param buffer pointer to buffer header
 @return bool value indicating whether buffer is found
 */
bool omx_amr_adec::search_input_bufhdr(OMX_BUFFERHEADERTYPE *buffer)
{
    bool eRet = false;
    OMX_BUFFERHEADERTYPE *temp = NULL;

    //access only in IL client context
    temp = m_input_buf_hdrs.find_ele(buffer);
    if ( buffer && temp )
    {
        DEBUG_DETAIL("search_input_bufhdr %p \n", buffer);
        eRet = true;
    }
    return eRet;
}

/**
 @brief member function that searches for caller buffer

 @param buffer pointer to buffer header
 @return bool value indicating whether buffer is found
 */
bool omx_amr_adec::search_output_bufhdr(OMX_BUFFERHEADERTYPE *buffer)
{
    bool eRet = false;
    OMX_BUFFERHEADERTYPE *temp = NULL;

    //access only in IL client context
    temp = m_output_buf_hdrs.find_ele(buffer);
    if ( buffer && temp )
    {
        DEBUG_DETAIL("search_output_bufhdr %p\n", buffer);
        eRet = true;
    }
    return eRet;
}

// Free Buffer - API call
/**
  @brief member function that handles free buffer command from IL client

  This function is a block-call function that handles IL client request to
  freeing the buffer

  @param hComp handle to component instance
  @param port id of port which holds the buffer
  @param buffer buffer header
  @return Error status
*/
OMX_ERRORTYPE  omx_amr_adec::free_buffer(OMX_IN OMX_HANDLETYPE         hComp,
                                         OMX_IN OMX_U32                 port,
                                         OMX_IN OMX_BUFFERHEADERTYPE* buffer)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;

    DEBUG_PRINT("Free_Buffer buf %p\n", buffer);

    if(hComp == NULL)
    {
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if ( m_state == OMX_StateIdle &&
         (BITMASK_PRESENT(&m_flags ,OMX_COMPONENT_LOADING_PENDING)) )
    {
        DEBUG_PRINT(" free buffer while Component in Loading pending\n");
    }
    else if ( (m_inp_bEnabled == OMX_FALSE && port == OMX_CORE_INPUT_PORT_INDEX)||
              (m_out_bEnabled == OMX_FALSE && port == OMX_CORE_OUTPUT_PORT_INDEX) )
    {
        DEBUG_PRINT("Free Buffer while port %lu disabled\n", port);
    }
    else if ( m_state == OMX_StateExecuting || m_state == OMX_StatePause )
    {
        DEBUG_PRINT("Invalid state to free buffer,ports need to be disabled:\
                    OMX_ErrorPortUnpopulated\n");
        post_command(OMX_EventError,
                     OMX_ErrorPortUnpopulated,
                     OMX_COMPONENT_GENERATE_EVENT);

        return eRet;
    }
    else
    {
        DEBUG_PRINT("free_buffer: Invalid state to free buffer,ports need to be\
                    disabled:OMX_ErrorPortUnpopulated\n");
        post_command(OMX_EventError,
                     OMX_ErrorPortUnpopulated,
                     OMX_COMPONENT_GENERATE_EVENT);
    }


    if ( port == OMX_CORE_INPUT_PORT_INDEX )
    {
        if ( m_inp_current_buf_count != 0 )
        {
            m_inp_bPopulated = OMX_FALSE;
            if ( search_input_bufhdr(buffer) == true )
            {
                /* Buffer exist */
                //access only in IL client context
                DEBUG_PRINT("Free_Buf:in_buffer[%p]\n",buffer);
                m_input_buf_hdrs.erase(buffer);
                if( buffer )
                {
                    free(buffer);
                    buffer = NULL;
                }
                m_inp_current_buf_count--;
            }
            else
            {
                DEBUG_PRINT_ERROR("Free_Buf:Error-->free_buffer, \
                    Invalid Input buffer header\n");
                eRet = OMX_ErrorBadParameter;
            }
        }
        else
        {
            DEBUG_PRINT_ERROR("Error: free_buffer,Port Index calculation \
                              came out Invalid\n");
            eRet = OMX_ErrorBadPortIndex;
        }

        if ( BITMASK_PRESENT((&m_flags),OMX_COMPONENT_INPUT_DISABLE_PENDING)
             && release_done(0) )
        {
            DEBUG_PRINT("INPUT PORT MOVING TO DISABLED STATE \n");
            BITMASK_CLEAR((&m_flags),OMX_COMPONENT_INPUT_DISABLE_PENDING);
            post_command(OMX_CommandPortDisable,
                         OMX_CORE_INPUT_PORT_INDEX,
                         OMX_COMPONENT_GENERATE_EVENT);
        }

    }
    else if ( port == OMX_CORE_OUTPUT_PORT_INDEX )
    {
        if ( m_out_current_buf_count != 0 )
        {
            m_out_bPopulated = OMX_FALSE;
            if ( search_output_bufhdr(buffer) == true )
            {
                /* Buffer exist */
                //access only in IL client context
                DEBUG_PRINT("Free_Buf:out_buffer[%p]\n",buffer);
                m_output_buf_hdrs.erase(buffer);
                if( buffer )
                {
                    free(buffer);
                    buffer = NULL;
                }
                m_out_current_buf_count--;
            }
            else
            {
                DEBUG_PRINT("Free_Buf:Error-->free_buffer , \
                    Invalid Output buffer header\n");
                eRet = OMX_ErrorBadParameter;
            }
        }
        else
        {
            eRet = OMX_ErrorBadPortIndex;
        }

        if ( BITMASK_PRESENT((&m_flags),OMX_COMPONENT_OUTPUT_DISABLE_PENDING)
             && release_done(1) )
        {
            DEBUG_PRINT("OUTPUT PORT MOVING TO DISABLED STATE \n");
            BITMASK_CLEAR((&m_flags),OMX_COMPONENT_OUTPUT_DISABLE_PENDING);
            post_command(OMX_CommandPortDisable,
                         OMX_CORE_OUTPUT_PORT_INDEX,
                         OMX_COMPONENT_GENERATE_EVENT);
        }
    }
    else
    {
        eRet = OMX_ErrorBadPortIndex;
    }

    if ( (OMX_ErrorNone == eRet) &&
         (BITMASK_PRESENT(&m_flags ,OMX_COMPONENT_LOADING_PENDING)) )
    {
        if ( release_done(-1) )
        {
            m_residual_data_len  = 0;
            if(suspensionPolicy == OMX_SuspensionEnabled)
            {
               ioctl(m_drv_fd,AUDIO_ABORT_GET_EVENT,NULL);
            }
            if(ioctl(m_drv_fd, AUDIO_STOP, 0) < 0)
            {
               DEBUG_PRINT("AUDIO STOP in free buffer failed\n");
            }
            else
            {
               DEBUG_PRINT("AUDIO STOP in free buffer passed\n");
            }

            m_first = 0;

            m_is_alloc_buf = 0;

            DEBUG_PRINT("Free_Buf: Free buffer\n");
            // Send the callback now
            BITMASK_CLEAR((&m_flags),OMX_COMPONENT_LOADING_PENDING);
            post_command(OMX_CommandStateSet,
                         OMX_StateLoaded,OMX_COMPONENT_GENERATE_EVENT);
        }
    }
    return eRet;
}


/**
 @brief member function that that handles empty this buffer command

 This function meremly queue up the command and data would be consumed
 in command server thread context

 @param hComp handle to component instance
 @param buffer pointer to buffer header
 @return error status
 */
OMX_ERRORTYPE  omx_amr_adec::empty_this_buffer
(
    OMX_IN OMX_HANDLETYPE         hComp,
    OMX_IN OMX_BUFFERHEADERTYPE* buffer
)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    DEBUG_PRINT("ETB:Buf:%p Len %lu TS %lld numInBuf=%d\n", buffer,
                buffer->nFilledLen, buffer->nTimeStamp, (nNumInputBuf+1));
    if ( m_state == OMX_StateInvalid )
    {
        DEBUG_PRINT("Empty this buffer in Invalid State\n");
        return OMX_ErrorInvalidState;
    }
    if ( !m_inp_bEnabled )
    {
        DEBUG_PRINT("empty_this_buffer OMX_ErrorIncorrectStateOperation "
                    "Port Status %d \n", m_inp_bEnabled);
        return OMX_ErrorIncorrectStateOperation;
    }
    if ( buffer->nSize != sizeof(OMX_BUFFERHEADERTYPE) )
    {
        DEBUG_PRINT("omx_amr_adec::etb--> Buffer Size Invalid\n");
        return OMX_ErrorBadParameter;
    }
    if ( buffer->nVersion.nVersion != OMX_SPEC_VERSION )
    {
        DEBUG_PRINT("omx_amr_adec::etb--> OMX Version Invalid\n");
        return OMX_ErrorVersionMismatch;
    }

    if ( buffer->nInputPortIndex != OMX_CORE_INPUT_PORT_INDEX )
    {
        return OMX_ErrorBadPortIndex;
    }
    //pthread_mutex_lock(&m_lock);
    if ( (m_state != OMX_StateExecuting) &&
         (m_state != OMX_StatePause) )
    {
        DEBUG_PRINT_ERROR("Invalid state\n");
        eRet = OMX_ErrorInvalidState;
    }
    //pthread_mutex_unlock(&m_lock);

    if ( OMX_ErrorNone == eRet )
    {
        if ( true == search_input_bufhdr(buffer) )
        {
            post_input((unsigned)hComp,
                       (unsigned) buffer,OMX_COMPONENT_GENERATE_ETB);
        }
        else
        {
            DEBUG_PRINT_ERROR("Bad header %p \n", buffer);
            eRet = OMX_ErrorBadParameter;
        }
    }
    pthread_mutex_lock(&in_buf_count_lock);
    nNumInputBuf++;
    m_amr_pb_stats.etb_cnt++;
    pthread_mutex_unlock(&in_buf_count_lock);
    return eRet;
}
/**
  @brief member function that writes data to kernel driver

  @param hComp handle to component instance
  @param buffer pointer to buffer header
  @return error status
 */

OMX_ERRORTYPE  omx_amr_adec::empty_this_buffer_proxy
(
    OMX_IN OMX_HANDLETYPE   hComp,
    OMX_BUFFERHEADERTYPE*   buffer
)
{
    OMX_STATETYPE state;
    META_IN       meta_in;
    //Pointer to the starting location of the data to be transcoded
    OMX_U8 *srcStart;
    //The frame to be transcoded
    OMX_U8  srcFrame[AMRWBP_MAX_INPUT_FRMSIZE] = {0};
    //The data consumed from the buffer that holds the data to be transcoded
    OMX_U32 srcDataConsumed = 0;
    //The total length of the data to be transcoded
    OMX_U32 len = 0;
    //The number of bytes that have been transcoded
    OMX_U32 bytesTranscoded = 0;
    OMX_U8 *data=NULL;

    m_is_full_frame = TRUE;
    PrintFrameHdr(OMX_COMPONENT_GENERATE_ETB,buffer);
    if(m_first == 0)
    {
        m_first++;
        if (ioctl(m_drv_fd, AUDIO_START, 0) < 0)
        {
            DEBUG_PRINT_ERROR("AUDIO_START FAILED\n");
            post_command((unsigned)OMX_CommandStateSet,
                             (unsigned)OMX_StateInvalid,
                              OMX_COMPONENT_GENERATE_COMMAND);
            post_command(OMX_CommandFlush,-1,
                                        OMX_COMPONENT_GENERATE_COMMAND);
            buffer_done_cb(buffer);
            return OMX_ErrorInvalidState;
        }
    }

    if(m_eos_bm)
    {
       m_eos_bm=0x00;
    }
    if ( search_input_bufhdr(buffer) == false )
    {
        DEBUG_PRINT("ETBP: INVALID BUF HDR\n");
        buffer_done_cb((OMX_BUFFERHEADERTYPE *)buffer);
        return OMX_ErrorBadParameter;
    }

    srcStart = buffer->pBuffer;
    len      = buffer->nFilledLen;
    m_is_full_frame = TRUE;

    if (pcm_feedback &&  m_tmp_meta_buf )//Non-Tunnelled mode
    {
        data = m_tmp_meta_buf;

        // copy the metadata info from the BufHdr and insert to payload
        meta_in.offsetVal  = sizeof(META_IN);
        meta_in.nTimeStamp = (((OMX_BUFFERHEADERTYPE*)buffer)->nTimeStamp)*1000;
        meta_in.nFlags     = buffer->nFlags;
        memcpy(data,&meta_in, meta_in.offsetVal);
    }

    while ( bytesTranscoded < (OMX_AMR_TRANSCODE_BUFFER_SIZE - AMRWBP_MAX_TRANSCODE_FRMSIZE))
    {
        OMX_U32 length = 0;
        if ( srcDataConsumed < len )
        {
            memset(srcFrame, 0, sizeof(OMX_U8) * AMRWBP_MAX_INPUT_FRMSIZE);

            if ( FALSE == FillSrcFrame(srcFrame,
                                       &srcStart,
                                       len,
                                       &srcDataConsumed) )
            {
                DEBUG_PRINT_ERROR("ERROR: FillSrcFrame\n");
                buffer_done_cb((OMX_BUFFERHEADERTYPE *)buffer);
                return OMX_ErrorInsufficientResources;
            }

            if ( (srcDataConsumed == 0) && (m_is_full_frame == FALSE) )
            {
                break;
            }

            if ( !(length = AMRTranscode(m_trans_buffer, srcFrame)) )
            {
                buffer_done_cb((OMX_BUFFERHEADERTYPE *)buffer);
                return OMX_ErrorMax;
            }
            bytesTranscoded += length;
            m_trans_buffer += length;
        }
        else
        {
            srcDataConsumed = 0;
            break;
        }
    }
   if(pcm_feedback)
   {
       if(data != NULL)
       {
           memcpy(&data[sizeof(META_IN)], m_trans_buffer_start, bytesTranscoded);
           DEBUG_PRINT("ETBP: bytesTranscoded = %lu \n",bytesTranscoded);
           write(m_drv_fd, data, bytesTranscoded + sizeof(META_IN));
       } 
   }
   else
   {
       DEBUG_PRINT("ETBP: bytesTranscoded = %lu \n",bytesTranscoded);
       write(m_drv_fd, m_trans_buffer_start, bytesTranscoded);
   }

    bytesTranscoded = 0;
    m_trans_buffer = m_trans_buffer_start;
    toc_cnt = 0;
    if ( (srcDataConsumed < len) && (srcDataConsumed > 0) )
    {
        memcpy(m_residual_buffer, srcStart, (len - srcDataConsumed));
        m_residual_data_len = len - srcDataConsumed;
        DEBUG_DETAIL("ETBP: m_residual_data_len = %lu\n",m_residual_data_len);
    }
    /* Assume empty this buffer function has already checked
    validity of buffer */
    DEBUG_PRINT("Empty buffer %p to kernel driver\n", buffer);

    if ( buffer->nFlags & OMX_BUFFERFLAG_EOS)
    {
        DEBUG_PRINT("\n INPUT EOS OCCURED \n");
        if ( pcm_feedback == 0 )
        {
            fsync(m_drv_fd);
            m_cb.EventHandler(&m_cmp,
                              m_app_data,
                              OMX_EventBufferFlag,
                              1, 0, NULL );
        }
    }
    pthread_mutex_lock(&m_state_lock);
    get_state(&m_cmp, &state);
    pthread_mutex_unlock(&m_state_lock);

   if(buffer->nFlags & OMX_BUFFERFLAG_EOS)
   {
       m_eos_bm |= IP_PORT_BITMASK;
       DEBUG_PRINT("ETBP:EOSFLAG=%d\n",m_eos_bm);
   }
    if (OMX_StatePause != state)
    {
        DEBUG_DETAIL("In Exe state, EBD CB");
        buffer_done_cb((OMX_BUFFERHEADERTYPE *)buffer);
    }
    else
    {
        // post the event to input thread.
        DEBUG_DETAIL("Not in exe state, post EBD to input thread thread");
        post_input((unsigned) & hComp,(unsigned) buffer,
                   OMX_COMPONENT_GENERATE_BUFFER_DONE);
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE  omx_amr_adec::fill_this_buffer_proxy
(
    OMX_IN OMX_HANDLETYPE   hComp,
    OMX_BUFFERHEADERTYPE*   buffer
)
{
    int byte_count = 0;
    int nDatalen = 0;
    OMX_STATETYPE state;
    META_OUT       meta_out;
    int nReadbytes = 0;
    get_state(&m_cmp, &state);
    if(fake_eos_recieved)
    {
        DEBUG_PRINT("Residual still left[%lu]state[%d]\n",
                                               m_bufMgr->getBufFilledSpace(),state);
        if(state == OMX_StateExecuting)
        {
            byte_count = m_bufMgr->emptyToBuf(buffer->pBuffer,buffer->nAllocLen);
            DEBUG_PRINT("Num of bytes copied[%d]Residual[%lu]\n",byte_count,
                                                          m_bufMgr->getBufFilledSpace());
            if(byte_count == 0)
            {
                fake_eos_recieved = false;

                // input EOS sent to DSP
                DEBUG_PRINT("FTBP: EOSFLAG=%d\n",m_eos_bm);
                if((m_eos_bm & IP_OP_PORT_BITMASK) == IP_OP_PORT_BITMASK)
                {
                   DEBUG_PRINT("FTBP:ENABLING SUSPEND FLAG\n");
                   buffer->nFlags= 0x01;
                   m_eos_bm = 0;
                }
                pthread_mutex_lock(&m_in_th_lock_1);
                if(is_in_th_sleep)
                {
                    DEBUG_DETAIL("FTBP::WAKING UP IN THREADS\n");
                    in_th_wakeup();
                    is_in_th_sleep = false;
                }
                pthread_mutex_unlock(&m_in_th_lock_1);

            }
            buffer->nTimeStamp = nTimestamp;
            buffer->nFilledLen = byte_count;
            frame_done_cb((OMX_BUFFERHEADERTYPE *)buffer);
            return OMX_ErrorNone;
        }
    }
    /* Assume fill this buffer function has already checked
    validity of buffer */

    if ( true == search_output_bufhdr(buffer) )
    {
        int ncount = buffer->nAllocLen/OMX_AMR_OUTPUT_BUFFER_SIZE;
        int nRead = 0;
        OMX_IN OMX_U8  *pBuf = NULL;
        pBuf = buffer->pBuffer;
        for ( nRead = 0; nRead < ncount; nRead++ )
        {
            if ( m_output_ctrl_cmd_q.m_size )
            {
                DEBUG_DETAIL("FTBP-->: FLUSH CMD IN Q, Exit from Loop");
                break;
            }

            DEBUG_PRINT("\nBefore Read..m_drv_fd = %d, pBuf = %p\n",
                        m_drv_fd,pBuf);
            nReadbytes = read(m_drv_fd,m_tmp_out_meta_buf,
                              (OMX_AMR_OUTPUT_BUFFER_SIZE+sizeof(meta_out)));
            DEBUG_DETAIL("FTBP->Al_len[%lu]buf[%p]Loop[%d]size[%d]numOutBuf[%d]\n",\
                         buffer->nAllocLen, buffer->pBuffer,
                         nRead, nReadbytes,nNumOutputBuf);
            if ( nReadbytes <= 0 )
            {
                DEBUG_PRINT("FTBP: breaking read since nReadbytes = 0  ");
                buffer->nFilledLen = 0;
                buffer->nTimeStamp= nTimestamp;
                post_output((unsigned) & hComp,(unsigned) buffer,
                            OMX_COMPONENT_GENERATE_FRAME_DONE);
                return OMX_ErrorNone;

            }
            // extract the metadata contents
            memcpy(&meta_out, m_tmp_out_meta_buf,sizeof(META_OUT));
            DEBUG_PRINT("FTBP->Meta:loop[%d]bytesread[%d]meta-len[0x%x]TS[0x%lld]"\
                        "nFlags[0x%lu]\n",\
                        nRead,nReadbytes, meta_out.offsetVal,
                        meta_out.nTimeStamp, meta_out.nFlags);
            if ( nRead == 0 )
            {
                buffer->nTimeStamp = (meta_out.nTimeStamp)/1000;
                DEBUG_PRINT("FTBP:Meta loop=%d TS[0x%lld] TS[0x%lld]\n",\
                            nRead,buffer->nTimeStamp,meta_out.nTimeStamp);
                nTimestamp = buffer->nTimeStamp;
            }
            nTimestamp = (meta_out.nTimeStamp)/1000;
            // copy the pcm frame to the client buffer
            if(nReadbytes > 0x18)
            {
                buffer->nFlags = (meta_out.nFlags & 0x00FE);
            }
            else
            {
                buffer->nFlags |= meta_out.nFlags;
            }

            memcpy(pBuf, &m_tmp_out_meta_buf[sizeof(META_OUT)],
                   (nReadbytes - sizeof(META_OUT)));

            if ( ((buffer->nFlags) & 0x0001) == OMX_BUFFERFLAG_EOS )
            {
                DEBUG_PRINT("FTBP: EOS reached Output port");
                if(m_eos_bm){
                  DEBUG_PRINT("FTBP: END OF STREAM \n");
                  m_eos_bm |= OP_PORT_BITMASK; //output EOS recieved;
                }
                DEBUG_PRINT("FTBP: END OF STREAM m_eos_bm=%d\n",m_eos_bm);
                break;
            }

            // just reading msize, should it be guarded by mutex ?
            else if ( m_output_ctrl_cmd_q.m_size )
            {
                DEBUG_DETAIL("FTBP-->: Flush Cmd found in Q, STOP READING");
                nDatalen = nDatalen + (nReadbytes - sizeof(META_OUT));
                break;
            }
            pBuf += (nReadbytes - sizeof(META_OUT));
            nDatalen = nDatalen + (nReadbytes - sizeof(META_OUT));
        }
        buffer->nFilledLen = nDatalen;

        if ( (nDatalen <= 0 ) && !(getSuspendFlg()))
        {
            buffer->nFilledLen = 0;
            DEBUG_PRINT("FTBP-->: Invalid data length read \n");
            if ( (buffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS )
            {
                DEBUG_PRINT("FTBP: Now, Send EOS flag to Client \n");
                m_cb.EventHandler(&m_cmp,
                                  m_app_data,
                                  OMX_EventBufferFlag,
                                  1, 1, NULL );
                DEBUG_PRINT("FTBP: END OF STREAM m_eos_bm=%d\n",m_eos_bm);
            }
            frame_done_cb((OMX_BUFFERHEADERTYPE *)buffer);
        }
        else
        {
           if ( (buffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS )
           {
              if(getSuspendFlg())
              {
                unsigned      p1; // Parameter - 1
                unsigned      p2; // Parameter - 2
                unsigned      id;
                bool          ret = false;
                fake_eos_recieved = true;

                // Faked EOS recieved, no processing of this mesg is reqd.
                ret = m_output_ctrl_cmd_q.get_msg_id(&id);
                if(ret && (id == OMX_COMPONENT_SUSPEND))
                {
                    m_output_ctrl_cmd_q.pop_entry(&p1,&p2,&id);
                }
                if(!m_eos_bm)
                {
                    // reset the flag as input eos was not set when this scenario happened
                    // o/p thread blocked on read() before faked_eos sent to dsp,
                    // now i/p thread fakes eos, o/p thread recieves the faked eos,
                    // since suspend flag is true, reset the nflags
                    buffer->nFlags = 0;
                }

                ioctl(m_drv_fd, AUDIO_STOP, 0);
                // now inform the client that the comp is SUSPENDED
                DEBUG_PRINT("\nFTBP: EOS reached Sending EH ctrlq=%d fake_eos=%d",\
                                               m_output_ctrl_cmd_q.m_size,fake_eos_recieved);
            }
            else
            {
                DEBUG_PRINT("FTBP: Now, Send EOS flag to Client \n");
                post_output((unsigned) & hComp,(unsigned) buffer,
                            OMX_COMPONENT_GENERATE_EOS);
                post_output((unsigned) & hComp,(unsigned) buffer,
                            OMX_COMPONENT_GENERATE_FRAME_DONE);
                return OMX_ErrorNone;
            }
           }
         DEBUG_PRINT("nState %d \n",nState );
            pthread_mutex_lock(&m_state_lock);
            get_state(&m_cmp, &state);
            pthread_mutex_unlock(&m_state_lock);

            if ( state == OMX_StatePause )
            {
                DEBUG_PRINT("FTBP:Post the FBD to event thread currstate=%d\n",\
                            state);
                post_output((unsigned) & hComp,(unsigned) buffer,
                            OMX_COMPONENT_GENERATE_FRAME_DONE);
            }
            else
            {
                frame_done_cb((OMX_BUFFERHEADERTYPE *)buffer);
            }
        }

    }
    else
    {
        DEBUG_PRINT("\n FTBP-->Invalid buffer in FTB \n");
    }

    return OMX_ErrorNone;
}


/* ======================================================================
FUNCTION
  omx_amr_adec::FillThisBuffer

DESCRIPTION
  IL client uses this method to release the frame buffer
  after displaying them.

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
OMX_ERRORTYPE  omx_amr_adec::fill_this_buffer
(
    OMX_IN OMX_HANDLETYPE         hComp,
    OMX_IN OMX_BUFFERHEADERTYPE* buffer
)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;

    if ( buffer->nSize != sizeof(OMX_BUFFERHEADERTYPE) )
    {
        DEBUG_PRINT("omx_amr_adec::ftb--> Buffer Size Invalid\n");
        return OMX_ErrorBadParameter;
    }
    if ( m_out_bEnabled == OMX_FALSE )
    {
        return OMX_ErrorIncorrectStateOperation;
    }

    if ( buffer->nVersion.nVersion != OMX_SPEC_VERSION )
    {
        DEBUG_PRINT("omx_amr_adec::ftb--> OMX Version Invalid\n");
        return OMX_ErrorVersionMismatch;
    }
    if ( buffer->nOutputPortIndex != OMX_CORE_OUTPUT_PORT_INDEX )
    {
        return OMX_ErrorBadPortIndex;
    }
    pthread_mutex_lock(&out_buf_count_lock);
    nNumOutputBuf++;
    m_amr_pb_stats.ftb_cnt++;
    DEBUG_DETAIL("FTB:nNumOutputBuf is %d", nNumOutputBuf);
    pthread_mutex_unlock(&out_buf_count_lock);
    post_output((unsigned)hComp,
                (unsigned) buffer,OMX_COMPONENT_GENERATE_FTB);
    return eRet; //OMX_ErrorNotImplemented;
}

/* ======================================================================
FUNCTION
  omx_amr_adec::SetCallbacks

DESCRIPTION
  Set the callbacks.

PARAMETERS
  None.

RETURN VALUE
  OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  omx_amr_adec::set_callbacks(OMX_IN OMX_HANDLETYPE        hComp,
                                           OMX_IN OMX_CALLBACKTYPE* callbacks,
                                           OMX_IN OMX_PTR             appData)
{
    if(hComp == NULL)
    {
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    m_cb       = *callbacks;
    m_app_data =    appData;

    return OMX_ErrorNone;
}

/* ======================================================================
FUNCTION
  omx_amr_adec::ComponentDeInit

DESCRIPTION
  Destroys the component and release memory allocated to the heap.

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if everything successful.

========================================================================== */
OMX_ERRORTYPE  omx_amr_adec::component_deinit(OMX_IN OMX_HANDLETYPE hComp)
{
    if(hComp == NULL)
    {
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if ( OMX_StateLoaded != m_state && OMX_StateInvalid != m_state )
    {
        DEBUG_PRINT_ERROR("Warning: Received DeInit when not in LOADED state, "
                          "cur_state %d\n", m_state);
        return OMX_ErrorInvalidState;
    }
    DEBUG_PRINT("Component-deinit being processed\n");
    DEBUG_PRINT("********************************\n");
    DEBUG_PRINT("STATS: in-buf-len[%lu]out-buf-len[%lu] tot-pb-time[%ld]",\
                m_amr_pb_stats.tot_in_buf_len,
                m_amr_pb_stats.tot_out_buf_len,
                m_amr_pb_stats.tot_pb_time);
    DEBUG_PRINT("STATS: fbd-cnt[%lu]ftb-cnt[%lu]etb-cnt[%lu]ebd-cnt[%lu]",\
                m_amr_pb_stats.fbd_cnt,m_amr_pb_stats.ftb_cnt,
                m_amr_pb_stats.etb_cnt,
                m_amr_pb_stats.ebd_cnt);
    DEBUG_PRINT("********************************\n");
    memset(&m_amr_pb_stats,0,sizeof(AMR_PB_STATS));
    if (mime_type)
    {
        free(mime_type);
        mime_type = NULL;
    }
    deinit_decoder();
    DEBUG_PRINT("************************************\n");
    DEBUG_PRINT_ERROR(" DEINIT COMPLETED");
    DEBUG_PRINT("************************************\n");
    return OMX_ErrorNone;
}
/* ======================================================================
FUNCTION
  omx_amr_adec::deinit_decoder

DESCRIPTION
  Closes all the threads and release memory allocated to the heap.

PARAMETERS
  None.

RETURN VALUE
  None.

========================================================================== */

void omx_amr_adec::deinit_decoder()
{
    DEBUG_PRINT("Component-deinit STARTED\n");

    frameLength = 0;
    frameSize = 0;
    amrwb = 0;
    frameType = 0;
    if((OMX_StateLoaded != m_state ) && (m_state != OMX_StateInvalid))
    {
        DEBUG_PRINT_ERROR("%s,Deinit called in state[%d]\n",__FUNCTION__,\
                                                              m_state);
        // Get back any buffers from driver
        execute_omx_flush(-1,false);
        // force state change to loaded so that all threads can be exited
        pthread_mutex_lock(&m_state_lock);
        m_state = OMX_StateLoaded;
        pthread_mutex_unlock(&m_state_lock);
        DEBUG_PRINT_ERROR("Freeing Buf:inp_current_buf_count[%d][%d]\n",\
        m_inp_current_buf_count,
        m_input_buf_hdrs.size());
        m_input_buf_hdrs.eraseall();
        DEBUG_PRINT_ERROR("Freeing Buf:out_current_buf_count[%d][%d]\n",\
        m_out_current_buf_count,
        m_output_buf_hdrs.size());
        m_output_buf_hdrs.eraseall();
       if(suspensionPolicy == OMX_SuspensionEnabled)
       {
           ioctl(m_drv_fd,AUDIO_ABORT_GET_EVENT,NULL);
       }

    }

    pthread_mutex_lock(&m_in_th_lock_1);
    if ( is_in_th_sleep )
    {
        is_in_th_sleep = false;
        DEBUG_DETAIL("Deinit:WAKING UP IN THREADS\n");
        in_th_wakeup();
    }
    pthread_mutex_unlock(&m_in_th_lock_1);
    pthread_mutex_lock(&m_out_th_lock_1);
    if ( is_out_th_sleep )
    {
        is_out_th_sleep = false;
        DEBUG_DETAIL("SCP:WAKING UP OUT THREADS\n");
        out_th_wakeup();
    }
    pthread_mutex_unlock(&m_out_th_lock_1);
    if ( m_ipc_to_in_th != NULL )
    {
        omx_amr_thread_stop(m_ipc_to_in_th);
        m_ipc_to_in_th = NULL;
    }
    if ( m_ipc_to_cmd_th != NULL )
    {
        omx_amr_thread_stop(m_ipc_to_cmd_th);
        m_ipc_to_cmd_th = NULL;
    }
    if ( pcm_feedback ==1 )
    {
        if ( m_ipc_to_out_th != NULL )
        {
            omx_amr_thread_stop(m_ipc_to_out_th);
            m_ipc_to_out_th = NULL;
        }
    }

     if(suspensionPolicy == OMX_SuspensionEnabled)
    {
          ioctl(m_drv_fd,AUDIO_ABORT_GET_EVENT,NULL);
    }
    if(m_ipc_to_event_th != NULL)
    {

        omx_amr_thread_stop(m_ipc_to_event_th);
          m_ipc_to_event_th = NULL;
    }
    if(ioctl(m_drv_fd, AUDIO_STOP, 0) <0)
    {
        DEBUG_PRINT("De-init: AUDIO_STOP FAILED\n");
    }

    if ( m_tmp_meta_buf )
    {
        free(m_tmp_meta_buf);
        m_tmp_meta_buf = NULL;
    }
    if ( m_tmp_out_meta_buf )
    {
        free(m_tmp_out_meta_buf);
        m_tmp_out_meta_buf = NULL;
    }
    if ( m_trans_buffer_start!= NULL )
    {
        free(m_trans_buffer_start);
        m_trans_buffer_start= NULL;
    }
    if ( m_residual_buffer_start != NULL )
    {
        free(m_residual_buffer_start);
        m_residual_buffer_start = NULL;
    }

    nNumInputBuf = 0;
    nNumOutputBuf = 0;
    m_first = 0;

    m_is_alloc_buf = 0;
    resetSuspendFlg();
    resetResumeFlg();
    fake_eos_recieved = false;
    m_eos_bm=false;
    m_pause_to_exe=false;
    bFlushinprogress = 0;
    m_inp_current_buf_count=0;
    m_out_current_buf_count=0;
    m_out_act_buf_count = 0;
    m_inp_act_buf_count = 0;
    m_inp_bEnabled = OMX_FALSE;
    m_out_bEnabled = OMX_FALSE;
    m_inp_bPopulated = OMX_FALSE;
    m_out_bPopulated = OMX_FALSE;
    if ( m_drv_fd >= 0 )
    {
        if(close(m_drv_fd) < 0)
        DEBUG_PRINT("De-init: Driver Close Failed \n");
        m_drv_fd = -1;
    }
    else
    {
        DEBUG_PRINT_ERROR(" AMR device already closed\n");
    }
    m_comp_deinit = 1;
    m_is_out_th_sleep = 1;
    m_is_in_th_sleep = 1;
    if(m_timer)
        delete m_timer;
    if(m_bufMgr)
        delete m_bufMgr;
    DEBUG_PRINT("************************************\n");
    DEBUG_PRINT(" DEINIT COMPLETED");
    DEBUG_PRINT("************************************\n");
}

/* ======================================================================
FUNCTION
  omx_amr_adec::UseEGLImage

DESCRIPTION
  OMX Use EGL Image method implementation <TBD>.

PARAMETERS
  <TBD>.

RETURN VALUE
  Not Implemented error.

========================================================================== */
OMX_ERRORTYPE  omx_amr_adec::use_EGL_image
(
    OMX_IN OMX_HANDLETYPE            hComp,
    OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
    OMX_IN OMX_U32                   port,
    OMX_IN OMX_PTR                   appData,
    OMX_IN void*                     eglImage
)
{
    DEBUG_PRINT_ERROR("Error : use_EGL_image:  Not Implemented \n");
    if((hComp == NULL) || (appData == NULL) || (eglImage == NULL))
    {
        bufferHdr = NULL;
        port = 0;
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    return OMX_ErrorNotImplemented;
}

/* ======================================================================
FUNCTION
  omx_amr_adec::ComponentRoleEnum

DESCRIPTION
  OMX Component Role Enum method implementation.

PARAMETERS
  <TBD>.

RETURN VALUE
  OMX Error None if everything is successful.
========================================================================== */
OMX_ERRORTYPE  omx_amr_adec::component_role_enum(OMX_IN OMX_HANDLETYPE hComp,
                                                 OMX_OUT OMX_U8*        role,
                                                 OMX_IN OMX_U32        index)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    const char *cmp_role = "audio_decoder.amrwb";

    if(hComp == NULL)
    {
        DEBUG_PRINT_ERROR("Returning OMX_ErrorBadParameter\n");
        return OMX_ErrorBadParameter;
    }
    if ( index == 0 && role )
    {
        memcpy(role, cmp_role, sizeof(cmp_role));
        *(((char *) role) + sizeof(cmp_role)) = '\0';
    }
    else
    {
        eRet = OMX_ErrorNoMore;
    }
    return eRet;
}




/* ======================================================================
FUNCTION
  omx_amr_adec::AllocateDone

DESCRIPTION
  Checks if entire buffer pool is allocated by IL Client or not.
  Need this to move to IDLE state.

PARAMETERS
  None.

RETURN VALUE
  true/false.

========================================================================== */
bool omx_amr_adec::allocate_done(void)
{
    OMX_BOOL bRet = OMX_FALSE;
    if ( pcm_feedback==1 )
    {
        if ( (m_inp_act_buf_count == m_inp_current_buf_count)
             &&(m_out_act_buf_count == m_out_current_buf_count) )
        {
            bRet=OMX_TRUE;
        }
        if ( (m_inp_act_buf_count == m_inp_current_buf_count) && m_inp_bEnabled )
        {
            m_inp_bPopulated = OMX_TRUE;
        }
        if ( (m_out_act_buf_count == m_out_current_buf_count) && m_out_bEnabled )
        {
            m_out_bPopulated = OMX_TRUE;
        }
    }
    else if ( pcm_feedback==0 )
    {
        if ( m_inp_act_buf_count == m_inp_current_buf_count )
        {
            bRet=OMX_TRUE;
        }
        if ( (m_inp_act_buf_count == m_inp_current_buf_count) && m_inp_bEnabled )
        {
            m_inp_bPopulated = OMX_TRUE;
        }
    }

    return bRet;
}
/* ======================================================================
FUNCTION
  omx_amr_adec::ReleaseDone

DESCRIPTION
  Checks if IL client has released all the buffers.

PARAMETERS
  None.

RETURN VALUE
  true/false

========================================================================== */
bool omx_amr_adec::release_done(OMX_U32 param1)
{
    DEBUG_PRINT("Inside omx_amr_adec::release_done");

    OMX_BOOL bRet = OMX_FALSE;
    if ( param1 == OMX_ALL )
    {
        if ( (0 == m_inp_current_buf_count)&&(0 == m_out_current_buf_count) )
        {
            bRet=OMX_TRUE;
        }
    }
    else if ( param1 == OMX_CORE_INPUT_PORT_INDEX )
    {
        if ( (0 == m_inp_current_buf_count) )
        {
            bRet=OMX_TRUE;
        }
    }
    else if ( param1 == OMX_CORE_OUTPUT_PORT_INDEX )
    {
        if ( (0 == m_out_current_buf_count) )
        {
            bRet=OMX_TRUE;
        }
    }
    return bRet;
}
bool omx_amr_adec::omx_amr_fake_eos()
{
    int ret;
    META_IN meta_in;

    // FAKE an EOS
    // copy the metadata info from the BufHdr and insert to payload
    meta_in.offsetVal  = sizeof(META_IN);
    meta_in.nTimeStamp = nTimestamp;
    meta_in.nFlags     = OMX_BUFFERFLAG_EOS;
    ret = write(m_drv_fd, &meta_in, sizeof(META_IN));
    DEBUG_PRINT("omx_amr_fake_eos :ret=%d", ret);
    return true;
}
void omx_amr_adec::append_data_to_temp_buf()
{
    int bytes_read = 0;
    unsigned int tot_bytes = 0;
    unsigned int cur_bytes = 0;
    META_OUT meta_out;

    DEBUG_PRINT("APPEND:SUSPEND: output_buffer_size = %d\n", output_buffer_size);

    while( 1)
    {
        bytes_read = read(m_drv_fd, m_tmp_out_meta_buf,
                                      (OMX_AMR_OUTPUT_BUFFER_SIZE + sizeof(META_OUT)));
        DEBUG_PRINT("Append-->current bytes_read=%d total=%d\n",bytes_read,tot_bytes);
        if(bytes_read > 0)
        {
            memcpy(&meta_out, m_tmp_out_meta_buf, sizeof(META_OUT));

            nTimestamp = (meta_out.nTimeStamp)/1000;
            if((meta_out.nFlags & 0x001) == OMX_BUFFERFLAG_EOS)
            {
                DEBUG_PRINT("**********READ=0**********************");
                DEBUG_PRINT("Append:EOS found Residual pcm[%d][%lu]",tot_bytes,
                                               m_bufMgr->getBufFilledSpace());
                DEBUG_PRINT("**********READ=0**********************");
                break;
            }
            else
            {
                cur_bytes = m_bufMgr->appendToBuf(
                                      &m_tmp_out_meta_buf[sizeof(META_OUT)],
                                      (bytes_read - sizeof(META_OUT)));
                if(cur_bytes == 0)
                {
                    DEBUG_PRINT_ERROR("Append: Reject residual pcm,"
                                      " no more space \n");
                    break;
                }
                tot_bytes += cur_bytes;
            }
        }
        else /*if(bytes_read ==-1)*/
        {
            DEBUG_PRINT("**********READ=-1**********************");
            DEBUG_PRINT("Append:Read=-1 totalbytes=%u Residual pcm[%lu]",tot_bytes,
                                      m_bufMgr->getBufFilledSpace());
            DEBUG_PRINT("**********READ=-1**********************");
            break;

        }
    }
    // EOS recieved.
    pthread_mutex_lock(&m_suspendresume_lock);
    setResumeFlg();
    pthread_mutex_unlock(&m_suspendresume_lock);
    fake_eos_recieved = true;
     if(m_eos_bm & IP_PORT_BITMASK)
     {
         m_eos_bm |= OP_PORT_BITMASK;
     }
     /*m_cb.EventHandler(&m_cmp, m_app_data,
               OMX_EventError,OMX_ErrorComponentSuspended,
                                   0, NULL );*/


     DEBUG_PRINT_ERROR("Rel DSP res, eos_bm[%d]sus[%d]\n",m_eos_bm,
                                     getWaitForSuspendCmplFlg());

    ioctl(m_drv_fd, AUDIO_STOP, 0);

    if (getWaitForSuspendCmplFlg())
    {
        DEBUG_PRINT_ERROR("Release P-->Executing context to IL client.\n");
        release_wait_for_suspend();
    }

    // now inform the client that the comp is SUSPENDED
    //m_cb.EventHandler(&m_cmp, m_app_data,
    //                   OMX_EventError,OMX_ErrorComponentSuspended,
    //                   0, NULL );
    DEBUG_PRINT("**********OMX_IndexParamComponentSUSPENDED****");
    if(m_timer->getTimerExpiry())
    {
        post_command(0,0,OMX_COMPONENT_RESUME);
    }
DEBUG_PRINT_ERROR("Enter into TCXO shutdown mode, residualdata[%lu]\n",m_bufMgr->getBufFilledSpace());
    return ;
}

timer::timer(omx_amr_adec* base):
    m_timerExpiryFlg(false),
    m_timeout(30),
    m_deleteTimer(OMX_FALSE),
    m_timer_cnt(0),
    m_base(base),
    m_timerinfo(NULL)
{
    sem_init(&m_sem_state,0, 0);

    pthread_cond_init (&m_timer_cond, NULL);
    pthread_mutexattr_init(&m_timer_mutex_attr);
    pthread_mutex_init(&m_timer_mutex, &m_timer_mutex_attr);

    pthread_cond_init (&m_tcond, NULL);
    pthread_mutexattr_init(&m_tmutex_attr);
    pthread_mutex_init(&m_tmutex, &m_tmutex_attr);

    m_timerinfo = (TIMERINFO*)malloc(sizeof(TIMERINFO));
    m_timerinfo->pTimer = this;
    m_timerinfo->base  = m_base;
    int rc = pthread_create(&m_timerinfo->thr ,0, omx_amr_comp_timer_handler,
                             (void*)m_timerinfo );
    if(rc < 0)
{
        DEBUG_PRINT_ERROR("Fail to create timer thread rc=%d errno=%d\n",rc,
                                                                     errno);
        free(m_timerinfo);
        m_timerinfo = NULL;
        }
        else
        DEBUG_PRINT("Created thread for timer object...\n");
}

timer::~timer()
{

    releaseTimer();
    stopTimer();
    if(m_timerinfo)
    {
        int rc = pthread_join(m_timerinfo->thr,NULL);
        DEBUG_PRINT("******************************\n");
        DEBUG_PRINT("CLOSING TIMER THREAD...%u %lu\n",rc,m_timerinfo->thr);
        DEBUG_PRINT("******************************\n");
        m_timerinfo->pTimer = NULL;
        m_timerinfo->base = NULL;
        free(m_timerinfo);
        m_timerinfo = NULL;
        }

    sem_destroy(&m_sem_state);
    pthread_mutexattr_destroy(&m_timer_mutex_attr);
    pthread_mutex_destroy(&m_timer_mutex);
    pthread_cond_destroy(&m_timer_cond);

    pthread_mutexattr_destroy(&m_tmutex_attr);
    pthread_mutex_destroy(&m_tmutex);
    pthread_cond_destroy(&m_tcond);

    m_timerExpiryFlg=false;
    m_deleteTimer = OMX_FALSE;
    m_timer_cnt = 1;
    m_base = NULL;
    }

void timer::wait_for_timer_event()
{
    sem_wait(&m_sem_state);
}

void timer::startTimer()
{
    sem_post(&m_sem_state);
}

int timer::timer_run()
{
    int rc =0;

    struct timespec   ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    DEBUG_PRINT("%s: Starting timer at %ld %ld %d\n",
                            __FUNCTION__,
                            ts.tv_sec,ts.tv_nsec,m_timer_cnt);
    clock_gettime(CLOCK_REALTIME, &ts);
            /* Convert from timeval to timespec */
    ts.tv_sec += m_timeout;
    pthread_mutex_lock(&m_timer_mutex);
    while (m_timer_cnt == 0)
    {
        if(getReleaseTimerStat()== OMX_TRUE)
        {
            DEBUG_PRINT_ERROR("Killing timer thread...\n");
            pthread_mutex_unlock(&m_timer_mutex);
            return 0;
        }
        rc = pthread_cond_timedwait(&m_timer_cond,
                                    &m_timer_mutex,
                                    &ts);
        DEBUG_PRINT("Timed wait rc=%d\n",rc);
        break;
    }
    m_timer_cnt = 0;
    pthread_mutex_unlock(&m_timer_mutex);
    clock_gettime(CLOCK_REALTIME, &ts);
    DEBUG_PRINT("%s: Elapsed Timer: %ld %ld\n",
                            __FUNCTION__,
                            ts.tv_sec,ts.tv_nsec);
    return rc;
    }

void timer::stopTimer()
{
    pthread_mutex_lock(&m_timer_mutex);
    if(m_timer_cnt == 0)
    {
        m_timer_cnt = 1;
        pthread_cond_signal(&m_timer_cond);
    }
    m_timer_cnt=0;
    pthread_mutex_unlock(&m_timer_mutex);
    DEBUG_PRINT("STOP TIMER...\n");
    return;
}

void* omx_amr_comp_timer_handler(void *pT)
{
    TIMERINFO *pTime = (TIMERINFO*)pT;
    timer *pt = pTime->pTimer;
    omx_amr_adec *pb = pTime->base;
    int               rc = 0;

    while(1)
    {
        pt->wait_for_timer_event();
        if(pt->getReleaseTimerStat()== OMX_TRUE)
        {
            DEBUG_PRINT_ERROR("Killing timer thread...\n");
            goto exit_th;
        }
        rc = pt->timer_run();
        if(rc != ETIMEDOUT)
        {
            if(pt->getReleaseTimerStat()== OMX_TRUE)
            {
                DEBUG_PRINT_ERROR("Now, Kill timer thread...\n");
                goto exit_th;
            }
            else
            {
                DEBUG_PRINT("Timer, go and wait again...\n");
                continue;
            }
}
        // now post event to command thread;
        OMX_STATETYPE state;
        DEBUG_DETAIL("SH:state=%d suspendstat=%d\n",pb->get_state((OMX_HANDLETYPE)pb,&state),\
                                                    pb->getSuspendFlg());
        pb->get_state((OMX_HANDLETYPE)pb,&state);
        if( ((state== OMX_StatePause)) && !(pb->getSuspendFlg()))
        {
            // post suspend message to command thread;
            pb->post_command(0,0,omx_amr_adec::OMX_COMPONENT_SUSPEND);
            pt->setTimerExpiry();
        }
        else
        {
            DEBUG_PRINT("SH: Ignore Timer expiry state=%d",state);
        }
    }

exit_th:
    DEBUG_PRINT_ERROR("Timer thread exited\n");
    return NULL;
}
OMX_U32 omxBufMgr::appendToBuf(OMX_U8 *srcbuf, OMX_U32 len)
{
    OMX_U32 tempLen=len;
    DEBUG_DETAIL("Remaining space in buffer=%lu\n",m_remaining_space);
    DEBUG_DETAIL("Append: write=%p read=%p\n",m_write,m_read);

    if(!m_remaining_space || (m_remaining_space < len))
    {
        m_rejected_bytes += len;
        m_tot_rejected_bytes += len;
        DEBUG_DETAIL("Reject pcm data %lu\n",m_rejected_bytes);
        return NULL;
    }

    if(m_write > m_read)
    {
        if((m_write + len) >= m_end )
        {
           OMX_U32 temp=0;
           memcpy(m_write,srcbuf,(m_end - m_write));
           temp = (m_end - m_write);
           m_write=m_start;
           memcpy(m_write,&srcbuf[temp],(len - temp));
           m_write += (len - temp);
        }
        else
        {
            memcpy(m_write,srcbuf,len);
            m_write += len;
        }
    }
    else if(m_write < m_read)
    {
        if((m_write + len) >= m_read )
        {
           OMX_U32 temp=0;
           memcpy(m_write,srcbuf,(m_read - m_write));
           temp = (m_read - m_write);
           m_write += temp;
           tempLen = temp;
        }
        else
        {
            memcpy(m_write,srcbuf,len);
            m_write += len;
        }
    }
    else
    {
        if((m_write + len) >= m_end )
        {
           OMX_U32 temp=0;
           memcpy(m_write,srcbuf,(m_end - m_write));
           temp = (m_end - m_write);
           m_write=m_start;
           memcpy(m_write,&srcbuf[temp],(len - temp));
           m_write += (len - temp);

        }
        else
        {
            memcpy(m_write,srcbuf,len);
            m_write += len;
        }

    }
    m_remaining_space -= tempLen;

    if(m_write >= m_end)
        m_write = m_start;

    DEBUG_DETAIL("==>Remaining space in buffer=%lu\n",m_remaining_space);
    return tempLen;
}

OMX_U32 omxBufMgr::emptyToBuf(OMX_U8 *destBuf, OMX_U32 destLen)
{
    if(destBuf == NULL)
    {
        DEBUG_DETAIL("emptyToBuf: NULL destBuf...\n");
        return 0;
    }
    OMX_U32 filledSpace = (OMX_TCXO_BUFFER -1 - m_remaining_space);
    OMX_U32 len=0;

    DEBUG_DETAIL("***********************\n");
    DEBUG_DETAIL("FilledSpace=%lu destLen=%lu\n",filledSpace,destLen);
    DEBUG_DETAIL("***********************\n");
    if(!filledSpace)
    {
        DEBUG_DETAIL("Buf empty ...\n");
        return 0;
    }
    if(filledSpace > destLen)
    {
        len = destLen;
    }
    else
    {
        len = filledSpace;
    }
    if(m_read > m_write)
    {
        if((m_read + len) >= m_end)
        {
            memcpy(destBuf,m_read,(m_end - m_read));
            memset(m_read,0,(m_end - m_read));

            OMX_U32 temp = (m_end - m_read);
            m_read=m_start;
            memcpy(&destBuf[temp],m_read,(len- temp));
            memset(m_read,0,(len - temp));
            m_read += (len - temp);
        }
        else
        {
            memcpy(destBuf,m_read,len);
            memset(m_read,0,len);
            m_read += len;
        }
    }
    else
    {
        memcpy(destBuf,m_read,len);
        memset(m_read,0,len);
        m_read += len;
    }
    m_remaining_space += len;

    if(m_read >= m_end)
        m_read = m_start;
DEBUG_PRINT("--> Copied %lu bytes\n",len);
    return len;
}




