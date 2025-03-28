#ifndef __USBHID_H
#define __USBHID_H

/*
 *  Copyright (c) 1999 Andreas Gal
 *  Copyright (c) 2000-2001 Vojtech Pavlik
 *  Copyright (c) 2006 Jiri Kosina
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#if 0
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/input.h>

/*  API provided by hid-core.c for USB HID drivers */
void usbhid_init_reports(struct hid_device *hid);
struct usb_interface *usbhid_find_interface(int minor);
#endif

/* iofl flags */
#define HID_CTRL_RUNNING	1
#define HID_OUT_RUNNING		2
#define HID_IN_RUNNING		3
#define HID_RESET_PENDING	4
#define HID_SUSPENDED		5
#define HID_CLEAR_HALT		6
#define HID_DISCONNECTED	7
#define HID_STARTED		8
#define HID_KEYS_PRESSED	10
#define HID_NO_BANDWIDTH	11
#define HID_RESUME_RUNNING	12
/*
 * The device is opened, meaning there is a client that is interested
 * in data coming from the device.
 */
#define HID_OPENED		13
/*
 * We are polling input endpoint by [re]submitting IN URB, because
 * either HID device is opened or ALWAYS POLL quirk is set for the
 * device.
 */
#define HID_IN_POLLING		14

/*
 * USB-specific HID struct, to be pointed to
 * from struct hid_device->driver_data
 */

struct usbhid_device {
	struct hid_device *hid;						/* pointer to corresponding HID dev */

	struct usb_interface *intf;                                     /* USB interface */
	int ifnum;                                                      /* USB interface number */

	unsigned int bufsize;                                           /* URB buffer size */

	struct urb *urbin;                                              /* Input URB */
	char *inbuf;                                                    /* Input buffer */
	dma_addr_t inbuf_dma;                                           /* Input buffer dma */

	struct urb *urbctrl;                                            /* Control URB */
	struct usb_ctrlrequest *cr;                                     /* Control request struct */
	
	struct hid_control_fifo ctrl[HID_CONTROL_FIFO_SIZE];  		/* Control fifo */
	unsigned char ctrlhead, ctrltail;                               /* Control fifo head & tail */

	char *ctrlbuf;                                                  /* Control buffer */
	dma_addr_t ctrlbuf_dma;                                         /* Control buffer dma */

	unsigned long last_ctrl;						/* record of last output for timeouts */

	struct urb *urbout;                                             /* Output URB */
	struct hid_output_fifo out[HID_CONTROL_FIFO_SIZE];              /* Output pipe fifo */
	unsigned char outhead, outtail;                                 /* Output pipe fifo head & tail */
	char *outbuf;                                                   /* Output buffer */
	dma_addr_t outbuf_dma;                                          /* Output buffer dma */
	unsigned long last_out;							/* record of last output for timeouts */

#if 0	
	spinlock_t lock;						/* fifo spinlock */
#endif
	unsigned long iofl;                                             /* I/O flags (CTRL_RUNNING, OUT_RUNNING) */
#if 0
	struct timer_list io_retry;                                     /* Retry timer */
#endif
	unsigned long stop_retry;                                       /* Time to give up, in jiffies */

	unsigned int retry_delay;                                       /* Delay length in ms */
#if 0
	struct work_struct reset_work;                                  /* Task context for resets */
	wait_queue_head_t wait;						/* For sleeping */
#endif
};

#define	hid_to_usb_dev(hid_dev) \
	(to_usb_interface(hid_dev->dev.parent)->usb_dev)

#endif

