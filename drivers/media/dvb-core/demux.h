/*
 * demux.h
 *
 * Copyright (c) 2002 Convergence GmbH
 *
 * based on code:
 * Copyright (c) 2000 Nokia Research Center
 *                    Tampere, FINLAND
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#ifndef __DEMUX_H
#define __DEMUX_H

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/list.h>
#include <linux/time.h>
#include <linux/dvb/dmx.h>

/*--------------------------------------------------------------------------*/
/* Common definitions */
/*--------------------------------------------------------------------------*/

/*
 * DMX_MAX_FILTER_SIZE: Maximum length (in bytes) of a section/PES filter.
 */

#ifndef DMX_MAX_FILTER_SIZE
#define DMX_MAX_FILTER_SIZE 18
#endif

/*
 * DMX_MAX_SECFEED_SIZE: Maximum length (in bytes) of a private section feed filter.
 */

#ifndef DMX_MAX_SECTION_SIZE
#define DMX_MAX_SECTION_SIZE 4096
#endif
#ifndef DMX_MAX_SECFEED_SIZE
#define DMX_MAX_SECFEED_SIZE (DMX_MAX_SECTION_SIZE + 188)
#endif

/*--------------------------------------------------------------------------*/
/* TS packet reception */
/*--------------------------------------------------------------------------*/

/* TS filter type for set() */

#define TS_PACKET       1   /* send TS packets (188 bytes) to callback (default) */
#define	TS_PAYLOAD_ONLY 2   /* in case TS_PACKET is set, only send the TS
			       payload (<=184 bytes per packet) to callback */
#define TS_DECODER      4   /* send stream to built-in decoder (if present) */
#define TS_DEMUX        8   /* in case TS_PACKET is set, send the TS to
			       the demux device, not to the dvr device */

/**
 * struct dmx_ts_feed - Structure that contains a TS feed filter
 *
 * @is_filtering:	Set to non-zero when filtering in progress
 * @parent:		pointer to struct dmx_demux
 * @priv:		pointer to private data of the API client
 * @set:		sets the TS filter
 * @start_filtering:	starts TS filtering
 * @stop_filtering:	stops TS filtering
 *
 * A TS feed is typically mapped to a hardware PID filter on the demux chip.
 * Using this API, the client can set the filtering properties to start/stop
 * filtering TS packets on a particular TS feed.
 */
struct dmx_ts_feed {
	int is_filtering;
	struct dmx_demux *parent;
	void *priv;
	int (*set) (struct dmx_ts_feed *feed,
		    u16 pid,
		    int type,
		    enum dmx_ts_pes pes_type,
		    size_t circular_buffer_size,
		    struct timespec timeout);
	int (*start_filtering) (struct dmx_ts_feed* feed);
	int (*stop_filtering) (struct dmx_ts_feed* feed);
};

/*--------------------------------------------------------------------------*/
/* Section reception */
/*--------------------------------------------------------------------------*/

/**
 * struct dmx_section_filter - Structure that describes a section filter
 *
 * @filter_value: Contains up to 16 bytes (128 bits) of the TS section header
 *		  that will be matched by the section filter
 * @filter_mask:  Contains a 16 bytes (128 bits) filter mask with the bits
 *		  specified by @filter_value that will be used on the filter
 *		  match logic.
 * @filter_mode:  Contains a 16 bytes (128 bits) filter mode.
 * @parent:	  Pointer to struct dmx_section_feed.
 * @priv:	  Pointer to private data of the API client.
 *
 *
 * The @filter_mask controls which bits of @filter_value are compared with
 * the section headers/payload. On a binary value of 1 in filter_mask, the
 * corresponding bits are compared. The filter only accepts sections that are
 * equal to filter_value in all the tested bit positions.
 */
struct dmx_section_filter {
	u8 filter_value [DMX_MAX_FILTER_SIZE];
	u8 filter_mask [DMX_MAX_FILTER_SIZE];
	u8 filter_mode [DMX_MAX_FILTER_SIZE];
	struct dmx_section_feed* parent; /* Back-pointer */
	void* priv; /* Pointer to private data of the API client */
};

/**
 * struct dmx_section_feed - Structure that contains a section feed filter
 *
 * @is_filtering:	Set to non-zero when filtering in progress
 * @parent:		pointer to struct dmx_demux
 * @priv:		pointer to private data of the API client
 * @check_crc:		If non-zero, check the CRC values of filtered sections.
 * @set:		sets the section filter
 * @allocate_filter:	This function is used to allocate a section filter on
 *			the demux. It should only be called when no filtering
 *			is in progress on this section feed. If a filter cannot
 *			be allocated, the function fails with -ENOSPC.
 * @release_filter:	This function releases all the resources of a
 * 			previously allocated section filter. The function
 *			should not be called while filtering is in progress
 *			on this section feed. After calling this function,
 *			the caller should not try to dereference the filter
 *			pointer.
 * @start_filtering:	starts section filtering
 * @stop_filtering:	stops section filtering
 *
 * A TS feed is typically mapped to a hardware PID filter on the demux chip.
 * Using this API, the client can set the filtering properties to start/stop
 * filtering TS packets on a particular TS feed.
 */
struct dmx_section_feed {
	int is_filtering;
	struct dmx_demux* parent;
	void* priv;

	int check_crc;

	/* private: Used internally at dvb_demux.c */
	u32 crc_val;

	u8 *secbuf;
	u8 secbuf_base[DMX_MAX_SECFEED_SIZE];
	u16 secbufp, seclen, tsfeedp;

	/* public: */
	int (*set) (struct dmx_section_feed* feed,
		    u16 pid,
		    size_t circular_buffer_size,
		    int check_crc);
	int (*allocate_filter) (struct dmx_section_feed* feed,
				struct dmx_section_filter** filter);
	int (*release_filter) (struct dmx_section_feed* feed,
			       struct dmx_section_filter* filter);
	int (*start_filtering) (struct dmx_section_feed* feed);
	int (*stop_filtering) (struct dmx_section_feed* feed);
};

/*--------------------------------------------------------------------------*/
/* Callback functions */
/*--------------------------------------------------------------------------*/

typedef int (*dmx_ts_cb) ( const u8 * buffer1,
			   size_t buffer1_length,
			   const u8 * buffer2,
			   size_t buffer2_length,
			   struct dmx_ts_feed* source);

typedef int (*dmx_section_cb) (	const u8 * buffer1,
				size_t buffer1_len,
				const u8 * buffer2,
				size_t buffer2_len,
				struct dmx_section_filter * source);

/*--------------------------------------------------------------------------*/
/* DVB Front-End */
/*--------------------------------------------------------------------------*/

/**
 * enum dmx_frontend_source - Used to identify the type of frontend
 *
 * @DMX_MEMORY_FE:	The source of the demux is memory. It means that
 *			the MPEG-TS to be filtered comes from userspace,
 *			via write() syscall.
 *
 * @DMX_FRONTEND_0:	The source of the demux is a frontend connected
 *			to the demux.
 */
enum dmx_frontend_source {
	DMX_MEMORY_FE,
	DMX_FRONTEND_0,
};

/**
 * struct dmx_frontend - Structure that lists the frontends associated with
 *			 a demux
 *
 * @connectivity_list:	List of front-ends that can be connected to a
 *			particular demux;
 * @source:		Type of the frontend.
 *
 * FIXME: this structure should likely be replaced soon by some
 *	media-controller based logic.
 */
struct dmx_frontend {
	struct list_head connectivity_list;
	enum dmx_frontend_source source;
};

/*--------------------------------------------------------------------------*/
/* MPEG-2 TS Demux */
/*--------------------------------------------------------------------------*/

/*
 * Flags OR'ed in the capabilities field of struct dmx_demux.
 */

#define DMX_TS_FILTERING                        1
#define DMX_PES_FILTERING                       2
#define DMX_SECTION_FILTERING                   4
#define DMX_MEMORY_BASED_FILTERING              8    /* write() available */
#define DMX_CRC_CHECKING                        16
#define DMX_TS_DESCRAMBLING                     32

/*
 * Demux resource type identifier.
*/

/*
 * DMX_FE_ENTRY(): Casts elements in the list of registered
 * front-ends from the generic type struct list_head
 * to the type * struct dmx_frontend
 *.
*/

#define DMX_FE_ENTRY(list) list_entry(list, struct dmx_frontend, connectivity_list)

struct dmx_demux {
	u32 capabilities;            /* Bitfield of capability flags */
	struct dmx_frontend* frontend;    /* Front-end connected to the demux */
	void* priv;                  /* Pointer to private data of the API client */
	int (*open) (struct dmx_demux* demux);
	int (*close) (struct dmx_demux* demux);
	int (*write) (struct dmx_demux* demux, const char __user *buf, size_t count);
	int (*allocate_ts_feed) (struct dmx_demux* demux,
				 struct dmx_ts_feed** feed,
				 dmx_ts_cb callback);
	int (*release_ts_feed) (struct dmx_demux* demux,
				struct dmx_ts_feed* feed);
	int (*allocate_section_feed) (struct dmx_demux* demux,
				      struct dmx_section_feed** feed,
				      dmx_section_cb callback);
	int (*release_section_feed) (struct dmx_demux* demux,
				     struct dmx_section_feed* feed);
	int (*add_frontend) (struct dmx_demux* demux,
			     struct dmx_frontend* frontend);
	int (*remove_frontend) (struct dmx_demux* demux,
				struct dmx_frontend* frontend);
	struct list_head* (*get_frontends) (struct dmx_demux* demux);
	int (*connect_frontend) (struct dmx_demux* demux,
				 struct dmx_frontend* frontend);
	int (*disconnect_frontend) (struct dmx_demux* demux);

	int (*get_pes_pids) (struct dmx_demux* demux, u16 *pids);

	/* private: Not used upstream and never documented */
#if 0
	int (*get_caps) (struct dmx_demux* demux, struct dmx_caps *caps);
	int (*set_source) (struct dmx_demux* demux, const dmx_source_t *src);
#endif
	/* public: */
	int (*get_stc) (struct dmx_demux* demux, unsigned int num,
			u64 *stc, unsigned int *base);
};

#endif /* #ifndef __DEMUX_H */
