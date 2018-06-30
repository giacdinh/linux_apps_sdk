/**
 *  @file tm_reader_async.c
 *  @brief Mercury API - background reading implementation
 *  @author Nathan Williams
 *  @date 11/18/2009
 */

 /*
 * Copyright (c) 2009 ThingMagic, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "tm_config.h"
#include "tm_reader.h"
#include "serial_reader_imp.h"
#include <stdio.h>

bool IsDutyCycleEnabled(TMR_Reader *reader);
TMR_Status restart_reading(struct TMR_Reader *reader);
#ifdef TMR_ENABLE_BACKGROUND_READS

#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

#ifndef WIN32
#include <sys/time.h>
#endif

#ifdef TMR_ENABLE_LLRP_READER
#include "llrp_reader_imp.h"
#endif
#include "osdep.h"
#include "tmr_utils.h"

static void *do_background_reads(void *arg);
static void *parse_tag_reads(void *arg);
static void process_async_response(TMR_Reader *reader);
bool isBufferOverFlow = false;
#endif /* TMR_ENABLE_BACKGROUND_READS */

TMR_Status
TMR_startReading(struct TMR_Reader *reader)
{
#ifdef SINGLE_THREAD_ASYNC_READ
  TMR_Status ret;
  uint32_t ontime;

  TMR_paramGet(reader, TMR_PARAM_READ_ASYNCONTIME, &ontime);
  reader->continuousReading = true;
  if (((TMR_SR_MODEL_M6E == reader->u.serialReader.versionInfo.hardware[0])||
	(TMR_SR_MODEL_M6E_I == reader->u.serialReader.versionInfo.hardware[0]) ||
	(TMR_SR_MODEL_MICRO == reader->u.serialReader.versionInfo.hardware[0]) ||
	(TMR_SR_MODEL_M6E_NANO == reader->u.serialReader.versionInfo.hardware[0])) &&
	(reader->readParams.asyncOffTime == 0 || (reader->readParams.asyncOffTime != 0 && IsDutyCycleEnabled(reader)) ) &&
	((TMR_READ_PLAN_TYPE_SIMPLE == reader->readParams.readPlan->type) ||
	((TMR_READ_PLAN_TYPE_MULTI == reader->readParams.readPlan->type)  &&
	(compareAntennas(&reader->readParams.readPlan->u.multi))))
	)
  {
	if (reader->readParams.asyncOffTime == 0)
	{
		reader->dutyCycle = false;
	}
	else
	{
		reader->dutyCycle = true;
	}
  }
  else
  {
	reader->dutyCycle = false;
  }
  ret = TMR_read(reader, ontime, NULL);
  if(TMR_SUCCESS != ret)
	return ret;

#else
#endif

  return TMR_SUCCESS;
}

#if defined(TMR_ENABLE_BACKGROUND_READS)|| defined(SINGLE_THREAD_ASYNC_READ)
bool IsDutyCycleEnabled(struct TMR_Reader *reader)
{
	uint16_t i;
	uint8_t *readerVersion = reader->u.serialReader.versionInfo.fwVersion ;
	uint8_t checkVersion[4];
	switch (reader->u.serialReader.versionInfo.hardware[0])
	{
		case TMR_SR_MODEL_M6E:
		case TMR_SR_MODEL_M6E_I:
			checkVersion[0] = 0x01; checkVersion[1] = 0x21; checkVersion[2] = 0x01; checkVersion[3] = 0x07;
			break;
		case TMR_SR_MODEL_MICRO:
			checkVersion[0] = 0x01; checkVersion[1] = 0x09; checkVersion[2] = 0x00; checkVersion[3] = 0x02;
			break;
		case TMR_SR_MODEL_M6E_NANO:
			checkVersion[0] = 0x01; checkVersion[1] = 0x07; checkVersion[2] = 0x00; checkVersion[3] = 0x02;
			break;
		default:
			checkVersion[0] = 0xFF; checkVersion[1] = 0xFF; checkVersion[2] = 0xFF; checkVersion[3] = 0xFF;
	}
	for (i = 0; i < 4; i++)
	{
		if (readerVersion[i] < checkVersion[i])
			return false;
		else if (readerVersion[i] > checkVersion[i])
			return true;
	}
	return true;
}

#endif /* TMR_ENABLE_BACKGROUND_READS */

void
reset_continuous_reading(struct TMR_Reader *reader, bool dueToError)
{
  if (true == reader->continuousReading)
  {
#ifndef SINGLE_THREAD_ASYNC_READ
#ifdef TMR_ENABLE_LLRP_READER
    if (TMR_READER_TYPE_LLRP == reader->readerType)
    {
      /**
       * In case of LLRP reader, re-enable the
       * LLRP background receiver as continuous reading is finished
       **/
      TMR_LLRP_setBackgroundReceiverState(reader, true);
    }
#endif

#ifdef TMR_ENABLE_SERIAL_READER
    if ((false == dueToError) && (TMR_READER_TYPE_SERIAL == reader->readerType))
    {
    /**
       * Disable filtering on module
       **/
      TMR_Status ret;
      bool value = reader->u.serialReader.enableReadFiltering;
      reader->hasContinuousReadStarted = false;
      ret = TMR_SR_cmdSetReaderConfiguration(reader, TMR_SR_CONFIGURATION_ENABLE_READ_FILTER, &value);
      if (TMR_SUCCESS != ret)
      {
#ifndef BARE_METAL
        notify_exception_listeners(reader, ret);
#endif
      }
    }
#endif/* TMR_ENABLE_SERIAL_READER */
#endif /*SINGLE_THREAD_ASYNC_READ*/
    /* disable streaming */
    reader->continuousReading = false;
  }
}

TMR_Status
TMR_stopReading(struct TMR_Reader *reader)
{
  reader->hasContinuousReadStarted = false;
#ifdef SINGLE_THREAD_ASYNC_READ
  reader->cmdStopReading(reader);
#else
#ifdef TMR_ENABLE_BACKGROUND_READS

  /* Check if background setup is active */
  pthread_mutex_lock(&reader->backgroundLock);

  if (false == reader->backgroundSetup)
  {
    pthread_mutex_unlock(&reader->backgroundLock);
    return TMR_SUCCESS;
  }

  if (false == reader->searchStatus)
  {
    /**
     * searchStatus is false, i.e., reading is already
     * stopped. Return success.
     **/
    pthread_mutex_unlock(&reader->backgroundLock);
    return TMR_SUCCESS;
  }
  /**
   * Else, read is in progress. Set
   * searchStatus to false;
   **/
  reader->searchStatus = false;
  pthread_mutex_unlock(&reader->backgroundLock);

  /**
   * Wait until the reading has started
   **/
  pthread_mutex_lock(&reader->backgroundLock);
  while (TMR_READ_STATE_STARTING == reader->readState)
  {
    pthread_cond_wait(&reader->readCond, &reader->backgroundLock);
    }
  pthread_mutex_unlock(&reader->backgroundLock);

  if ((true == reader->continuousReading) && (true == reader->trueAsyncflag))
    {
      /**
     * In case of true continuous reading, we need to send
     * stop reading message immediately.
       **/
    if(!isBufferOverFlow)
    {
      reader->cmdStopReading(reader);
    }

    /**
     * Wait logic has been changed in case of continuous reading.
     * Wait while the background reader is still reading.
     **/
    pthread_mutex_lock(&reader->backgroundLock);
    while (TMR_READ_STATE_DONE != reader->readState)
    {
      pthread_cond_wait(&reader->readCond, &reader->backgroundLock);
    }
    pthread_mutex_unlock(&reader->backgroundLock);
    /**
     * By this time, reader->backgroundEnabled is
     * already set to false. i.e., background reader thread
     * is suspended.
     **/
  }

  /**
   * wait until background reader thread finishes.
   * This is needed for pseudo-async reads and also
   * worst case of continuous reading, when read isn't success
   **/
  pthread_mutex_lock(&reader->backgroundLock);
  reader->backgroundEnabled = false;
  while (true == reader->backgroundRunning)
  {
    pthread_cond_wait(&reader->backgroundCond, &reader->backgroundLock);
  }
  pthread_mutex_unlock(&reader->backgroundLock);

  /**
   * Reset continuous reading settings, so that
   * the subsequent startReading() call doesn't have
   * any surprises.
   **/
#else
	reader->cmdStopReading(reader);
#endif
  reset_continuous_reading(reader, false);
#endif
  return TMR_SUCCESS;
}

void
notify_read_listeners(TMR_Reader *reader, TMR_TagReadData *trd)
{
  TMR_ReadListenerBlock *rlb;

  /* notify tag read to listener */
  if (NULL != reader)
  {
#ifndef SINGLE_THREAD_ASYNC_READ
    pthread_mutex_lock(&reader->listenerLock);
#endif
	rlb = reader->readListeners;
    while (rlb)
    {
      rlb->listener(reader, trd, rlb->cookie);
      rlb = rlb->next;
    }
#ifndef SINGLE_THREAD_ASYNC_READ
    pthread_mutex_unlock(&reader->listenerLock);
#endif
  }
}

void
notify_stats_listeners(TMR_Reader *reader, TMR_Reader_StatsValues *stats)
{
  TMR_StatsListenerBlock *slb;

  /* notify stats to the listener */
#ifndef SINGLE_THREAD_ASYNC_READ
  pthread_mutex_lock(&reader->listenerLock);
#endif
  slb = reader->statsListeners;
  while (slb)
  {
    slb->listener(reader, stats, slb->cookie);
    slb = slb->next;
  }
#ifndef SINGLE_THREAD_ASYNC_READ
  pthread_mutex_unlock(&reader->listenerLock);
#endif
}

TMR_Status restart_reading(struct TMR_Reader *reader)
{
	TMR_Status ret = TMR_SUCCESS;

	//Stop continuous reading
	ret = TMR_stopReading(reader);
	if(ret != TMR_SUCCESS)
	{
		return ret;
	}

	#ifdef SINGLE_THREAD_ASYNC_READ
	//Receive all tags from the previous reading
	{
		TMR_TagReadData trd;
		while(true)
		{
			ret = TMR_hasMoreTags(reader);
			if (TMR_SUCCESS == ret)
			{
				TMR_getNextTag(reader, &trd);
				notify_read_listeners(reader, &trd);
			}
			else if(ret == TMR_ERROR_END_OF_READING)
			break;
		}
	}
	#endif
	//Restart reading
	ret = TMR_startReading(reader);

	return ret;
}

#ifdef TMR_ENABLE_BACKGROUND_READS
/* NOTE: There is only one auth object for all the authreq listeners, so whichever listener touches it last wins.
 * For now (2012 Jul 20) we only anticipate having a single authreq listener, but there may be future cases which
 * require multiples.  Revise this design if necessary. */
void
notify_authreq_listeners(TMR_Reader *reader, TMR_TagReadData *trd, TMR_TagAuthentication *auth)
{
  TMR_AuthReqListenerBlock *arlb;

  /* notify tag read to listener */
  pthread_mutex_lock(&reader->listenerLock);
  arlb = reader->authReqListeners;
  while (arlb)
  {
    arlb->listener(reader, trd, arlb->cookie, auth);
    arlb = arlb->next;
  }
  pthread_mutex_unlock(&reader->listenerLock);
}
#endif /* TMR_ENABLE_BACKGROUND_READS */

TMR_Status
TMR_addReadExceptionListener(TMR_Reader *reader,
                             TMR_ReadExceptionListenerBlock *b)
{
#ifndef SINGLE_THREAD_ASYNC_READ
  if (0 != pthread_mutex_lock(&reader->listenerLock))
    return TMR_ERROR_TRYAGAIN;
#endif
  b->next = reader->readExceptionListeners;
  reader->readExceptionListeners = b;

#ifndef SINGLE_THREAD_ASYNC_READ
  pthread_mutex_unlock(&reader->listenerLock);
#endif
  return TMR_SUCCESS;
}

#ifdef TMR_ENABLE_BACKGROUND_READS
TMR_Status
TMR_removeReadExceptionListener(TMR_Reader *reader,
                                TMR_ReadExceptionListenerBlock *b)
{
  TMR_ReadExceptionListenerBlock *block, **prev;

  if (0 != pthread_mutex_lock(&reader->listenerLock))
    return TMR_ERROR_TRYAGAIN;

  prev = &reader->readExceptionListeners;
  block = reader->readExceptionListeners;
  while (NULL != block)
  {
    if (block == b)
    {
      *prev = block->next;
      break;
    }
    prev = &block->next;
    block = block->next;
  }

  pthread_mutex_unlock(&reader->listenerLock);

  if (block == NULL)
  {
    return TMR_ERROR_INVALID;
  }

  return TMR_SUCCESS;
}
#endif

void
notify_exception_listeners(TMR_Reader *reader, TMR_Status status)
{
  TMR_ReadExceptionListenerBlock *relb;

  if (NULL != reader)
  {
#ifndef SINGLE_THREAD_ASYNC_READ
    pthread_mutex_lock(&reader->listenerLock);
#endif
    relb = reader->readExceptionListeners;
    while (relb)
    {
      relb->listener(reader, status, relb->cookie);
      relb = relb->next;
    }
#ifndef SINGLE_THREAD_ASYNC_READ
    pthread_mutex_unlock(&reader->listenerLock);
#endif
  }
}

#ifdef TMR_ENABLE_BACKGROUND_READS
TMR_Queue_tagReads *
dequeue(TMR_Reader *reader)
{
  TMR_Queue_tagReads *tagRead = NULL;
  pthread_mutex_lock(&reader->queue_lock);
  if (NULL != reader->tagQueueHead)
  {
    /* Fetch the head always */
    tagRead = reader->tagQueueHead;
    reader->tagQueueHead = reader->tagQueueHead->next;
  }
  reader->queue_depth --;
  pthread_mutex_unlock(&reader->queue_lock);
  return(tagRead);
}

void enqueue(TMR_Reader *reader, TMR_Queue_tagReads *tagRead)
{
  pthread_mutex_lock(&reader->queue_lock);
  if (NULL == reader->tagQueueHead)
  {
    /* first tag */
    reader->tagQueueHead = tagRead;
    reader->tagQueueHead->next = NULL;
    reader->tagQueueTail = reader->tagQueueHead;
  }
  else
  {
    reader->tagQueueTail->next = tagRead;
    reader->tagQueueTail = tagRead;
    tagRead->next = NULL;
  }
  reader->queue_depth ++;
  pthread_mutex_unlock(&reader->queue_lock);
}

TMR_Status
TMR_addReadListener(TMR_Reader *reader, TMR_ReadListenerBlock *b)
{
#ifndef SINGLE_THREAD_ASYNC_READ
  if (0 != pthread_mutex_lock(&reader->listenerLock))
    return TMR_ERROR_TRYAGAIN;
#endif
  b->next = reader->readListeners;
  reader->readListeners = b;
#ifndef SINGLE_THREAD_ASYNC_READ
  pthread_mutex_unlock(&reader->listenerLock);
#endif
  return TMR_SUCCESS;
}
#ifdef TMR_ENABLE_BACKGROUND_READS

TMR_Status
TMR_removeReadListener(TMR_Reader *reader, TMR_ReadListenerBlock *b)
{
  TMR_ReadListenerBlock *block, **prev;

  if (0 != pthread_mutex_lock(&reader->listenerLock))
    return TMR_ERROR_TRYAGAIN;

  prev = &reader->readListeners;
  block = reader->readListeners;
  while (NULL != block)
  {
    if (block == b)
    {
      *prev = block->next;
      break;
    }
    prev = &block->next;
    block = block->next;
  }

  pthread_mutex_unlock(&reader->listenerLock);

  if (block == NULL)
  {
    return TMR_ERROR_INVALID;
  }

  return TMR_SUCCESS;
}
#endif // TMR_ENABLE_BACKGROUND_READS

TMR_Status
TMR_addAuthReqListener(TMR_Reader *reader, TMR_AuthReqListenerBlock *b)
{

  if (0 != pthread_mutex_lock(&reader->listenerLock))
    return TMR_ERROR_TRYAGAIN;

  b->next = reader->authReqListeners;
  reader->authReqListeners = b;

  pthread_mutex_unlock(&reader->listenerLock);

  return TMR_SUCCESS;
}

TMR_Status
TMR_removeAuthReqListener(TMR_Reader *reader, TMR_AuthReqListenerBlock *b)
{
  TMR_AuthReqListenerBlock *block, **prev;

  if (0 != pthread_mutex_lock(&reader->listenerLock))
    return TMR_ERROR_TRYAGAIN;

  prev = &reader->authReqListeners;
  block = reader->authReqListeners;
  while (NULL != block)
  {
    if (block == b)
    {
      *prev = block->next;
      break;
    }
    prev = &block->next;
    block = block->next;
  }

  pthread_mutex_unlock(&reader->listenerLock);

  if (block == NULL)
  {
    return TMR_ERROR_INVALID;
  }

  return TMR_SUCCESS;
}

TMR_Status
TMR_addStatusListener(TMR_Reader *reader, TMR_StatusListenerBlock *b)
{

  if (0 != pthread_mutex_lock(&reader->listenerLock))
    return TMR_ERROR_TRYAGAIN;

  b->next = reader->statusListeners;
  reader->statusListeners = b;

  /*reader->streamStats |= b->statusFlags & TMR_SR_STATUS_CONTENT_FLAGS_ALL;*/

  pthread_mutex_unlock(&reader->listenerLock);

  return TMR_SUCCESS;
}
#endif /* TMR_ENABLE_BACKGROUND_READS */

TMR_Status
TMR_addStatsListener(TMR_Reader *reader, TMR_StatsListenerBlock *b)
{
#ifndef SINGLE_THREAD_ASYNC_READ
  if (0 != pthread_mutex_lock(&reader->listenerLock))
    return TMR_ERROR_TRYAGAIN;
#endif
  b->next = reader->statsListeners;
  reader->statsListeners = b;

  /*reader->streamStats |= b->statusFlags & TMR_SR_STATUS_CONTENT_FLAGS_ALL; */
#ifndef SINGLE_THREAD_ASYNC_READ
  pthread_mutex_unlock(&reader->listenerLock);
#endif
  return TMR_SUCCESS;
}
#ifdef TMR_ENABLE_BACKGROUND_READS

TMR_Status
TMR_removeStatsListener(TMR_Reader *reader, TMR_StatsListenerBlock *b)
{
  TMR_StatsListenerBlock *block, **prev;

  if (0 != pthread_mutex_lock(&reader->listenerLock))
    return TMR_ERROR_TRYAGAIN;

  prev = &reader->statsListeners;
  block = reader->statsListeners;
  while (NULL != block)
  {
    if (block == b)
    {
      *prev = block->next;
      break;
    }
    prev = &block->next;
    block = block->next;
  }

  /* Remove the status flags requested by this listener and reframe */
  /*reader->streamStats = TMR_SR_STATUS_CONTENT_FLAG_NONE;
  {
    TMR_StatusListenerBlock *current;
    current = reader->statusListeners;
    while (NULL != current)
    {
      reader->streamStats |= current->statusFlags;
      current = current->next;
    }
  }*/

  pthread_mutex_unlock(&reader->listenerLock);

  if (block == NULL)
  {
    return TMR_ERROR_INVALID;
  }

  return TMR_SUCCESS;
}

TMR_Status
TMR_removeStatusListener(TMR_Reader *reader, TMR_StatusListenerBlock *b)
{
  TMR_StatusListenerBlock *block, **prev;

  if (0 != pthread_mutex_lock(&reader->listenerLock))
    return TMR_ERROR_TRYAGAIN;

  prev = &reader->statusListeners;
  block = reader->statusListeners;
  while (NULL != block)
  {
    if (block == b)
    {
      *prev = block->next;
      break;
    }
    prev = &block->next;
    block = block->next;
  }

  /* Remove the status flags requested by this listener and reframe */
  /*reader->streamStats = TMR_SR_STATUS_CONTENT_FLAG_NONE;
    {
    TMR_StatusListenerBlock *current;
    current = reader->statusListeners;
    while (NULL != current)
    {
    reader->streamStats |= current->statusFlags;
    current = current->next;
    }
    }*/

  pthread_mutex_unlock(&reader->listenerLock);

  if (block == NULL)
  {
    return TMR_ERROR_INVALID;
  }

  return TMR_SUCCESS;
}

#ifdef NOTUSE
void cleanup_background_threads(TMR_Reader *reader)
{
  if (NULL != reader)
  {
    pthread_mutex_lock(&reader->backgroundLock);
    pthread_mutex_lock(&reader->listenerLock);
    reader->readExceptionListeners = NULL;
    reader->statsListeners = NULL;
    if (true == reader->backgroundSetup)
    {
      /**
       * Signal for the thread exit by
       * removing all the pthread lock dependency
       **/
      reader->backgroundThreadCancel = true;
      pthread_cond_broadcast(&reader->backgroundCond);
    }
    pthread_mutex_unlock(&reader->listenerLock);
    pthread_mutex_unlock(&reader->backgroundLock);

    if (true == reader->backgroundSetup)
    {
      /**
       * Wait for the back ground thread to exit
       **/
      pthread_join(reader->backgroundReader, NULL);
    }

    pthread_mutex_lock(&reader->parserLock);
    pthread_mutex_lock(&reader->listenerLock);
    reader->readListeners = NULL;
    if (true == reader->parserSetup)
    {
      pthread_cancel(reader->backgroundParser);
    }
    pthread_mutex_unlock(&reader->listenerLock);
    pthread_mutex_unlock(&reader->parserLock);
  }
}
#endif // NOTUSE

void*
do_background_receiveAutonomousReading(void * arg)
{
  TMR_Status ret;
  TMR_TagReadData trd;
  TMR_Reader *reader;
  TMR_Reader_StatsValues stats;

  reader = arg;
  TMR_TRD_init(&trd);

  while (1)
  {
    ret = TMR_SR_receiveAutonomousReading(reader, &trd, &stats);
    if (TMR_SUCCESS == ret)
    {
      if (false == reader->isStatusResponse)
      {
        /* Notify the read listener */
        notify_read_listeners(reader, &trd);
      }
      else
      {
        notify_stats_listeners(reader, &stats);
      }
    }
  }
  return NULL;
}
#endif /* TMR_ENABLE_BACKGROUND_READS */
