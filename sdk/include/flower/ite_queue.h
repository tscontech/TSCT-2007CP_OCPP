/*
 * Copyright (c) 2004 ITE Technology Corp. All Rights Reserved.
 */
/** @file
 * ITE Queue.
 *
 * @author
 * @version 1.0
 */
#ifndef ITE_QUEUE_H
#define ITE_QUEUE_H
#include <stdio.h>
#include <assert.h>
#include "openrtos/FreeRTOS.h"
#include "openrtos/queue.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef bool
typedef unsigned char	bool;
#define  true	1
#define  false	0
#endif

//=============================================================================
//                              Structure Definition
//=============================================================================

typedef struct _msgb
{
	struct _msgb *b_prev;
	struct _msgb *b_next;
	unsigned char *b_datap;
	unsigned char *b_rptr;
	unsigned char *b_wptr;
    int size;
} mblk_ite;

typedef struct _IteAudioStream{
    bool Eof;
    bool Eofmix;
}IteAudioStream;

/**
 * Queue block data definition
 */
struct _IteQueueblk {
    uint8_t data;
    mblk_ite *datap;
    void *private1;
    void *private2;
    // TBD: more complex data struct
};
typedef struct _IteQueueblk IteQueueblk;

//=============================================================================
//                              Global Data Definition
//=============================================================================

//=============================================================================
//                              Public Function Definition
//=============================================================================
/**
 * Flush queue data
 *
 * @param QHandler The handler of queue
 */
void ite_mblk_queue_flush(QueueHandle_t QHandler);

/**
 * Create a queue
 *
 * @param QSize The Size of Queue
 * @param TSize The Size of one Quene entry
 */
QueueHandle_t ite_queue_new(int QSize, int TSize);

/**
 * Free a queue
 *
 * @param QHandler The handler of queue
 */
void ite_queue_free(QueueHandle_t QHandler);

/**
 * Get a data block from queue
 *
 * @param QHandler The handler of queue
 * @param qblk The block of data which get from queue
 */
int ite_queue_get(QueueHandle_t QHandler, IteQueueblk *qblk);

/**
 * Get a data pointer from queue
 *
 * @param QHandler The handler of queue
 */
IteQueueblk *ite_queue_get_byRef(QueueHandle_t QHandler);

/**
 * Get a data block from queue for interrupt-safe version
 *
 * @param QHandler The handler of queue
 * @param qblk The block of data which get from queue
 * @param xHigherPriorityTaskWoken Set pdTRUE if sending to the queue caused a task
 * to unblock, and the unblocked task has a priority higher than the currently
 * running task.
 */
int ite_queue_get_fromISR(QueueHandle_t QHandler, IteQueueblk *qblk, portBASE_TYPE xHigherPriorityTaskWoken);

/**
 * Put a data block to queue
 *
 * @param QHandler The handler of queue
 * @param qblk The block of data which put to queue
 */
int ite_queue_put(QueueHandle_t QHandler, IteQueueblk *qblk);

/**
 * Put a data pointer to queue
 *
 * @param QHandler The handler of queue
 * @param qblk The block of data pointer which put to queue
 */
int ite_queue_put_byRef(QueueHandle_t QHandler, IteQueueblk *qblk);

/**
 * Put a data block to queue for interrupt-safe version
 *
 * @param QHandler The handler of queue
 * @param qblk The block of data which put to queue
 * @param xHigherPriorityTaskWoken Set pdTRUE if sending to the queue caused a task
 * to unblock, and the unblocked task has a priority higher than the currently
 * running task.
 */
int ite_queue_put_fromISR(QueueHandle_t QHandler, IteQueueblk *qblk, portBASE_TYPE xHigherPriorityTaskWoken);

/**
 * Put a data block to queue's head
 *
 * @param QHandler The handler of queue
 * @param qblk The block of data which put to queue
 */
int ite_queue_put_head(QueueHandle_t QHandler, IteQueueblk *qblk);

/**
 * Put a data block to queue's head for interrupt-safe version
 *
 * @param QHandler The handler of queue
 * @param qblk The block of data which put to queue
 * @param xHigherPriorityTaskWoken Set pdTRUE if sending to the queue caused a task
 * to unblock, and the unblocked task has a priority higher than the currently
 * running task.
 */
int ite_queue_put_head_fromISR(QueueHandle_t QHandler, IteQueueblk *qblk, portBASE_TYPE xHigherPriorityTaskWoken);

/**
 * Get the size of queue
 *
 * @param QHandler The handler of queue
 */
int ite_queue_get_size(QueueHandle_t QHandler);

/**
 * Reset the queue
 *
 * @param QHandler The handler of queue
 */
void ite_queue_reset(QueueHandle_t QHandler);

/**
 * Get a data block from queue without the data being removed from the queue
 *
 * @param QHandler The handler of queue
 * @param qblk The block of data which get from the queue
 */
int ite_queue_peek_head(QueueHandle_t QHandler, IteQueueblk *qblk);

/**
 * Get a data block from queue without the data being removed from the queue for interrupt-safe version
 *
 * @param QHandler The handler of queue
 * @param qblk The block of data which get from the queue
 */
int ite_queue_peek_head_fromISR(QueueHandle_t QHandler, IteQueueblk *qblk);

/**
 * Create a memory block 
 *
 * @param memory block size
 * 
 */
mblk_ite *allocb_ite(size_t size);

/**
 * free memory block 
 *
 * @param mp A memory block for free
 * 
 */
void freemsg_ite(mblk_ite *mp);

#ifdef __cplusplus
}
#endif

#endif /* ITE_STREAMER_H */
