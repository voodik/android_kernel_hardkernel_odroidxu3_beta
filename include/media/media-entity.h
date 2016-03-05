/*
 * Media entity
 *
 * Copyright (C) 2010 Nokia Corporation
 *
 * Contacts: Laurent Pinchart <laurent.pinchart@ideasonboard.com>
 *	     Sakari Ailus <sakari.ailus@iki.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _MEDIA_ENTITY_H
#define _MEDIA_ENTITY_H

#include <linux/bitmap.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/media.h>

/* Enums used internally at the media controller to represent graphs */

/**
 * enum media_gobj_type - type of a graph object
 *
 * @MEDIA_GRAPH_ENTITY:		Identify a media entity
 * @MEDIA_GRAPH_PAD:		Identify a media pad
 * @MEDIA_GRAPH_LINK:		Identify a media link
 * @MEDIA_GRAPH_INTF_DEVNODE:	Identify a media Kernel API interface via
 *				a device node
 */
enum media_gobj_type {
	MEDIA_GRAPH_ENTITY,
	MEDIA_GRAPH_PAD,
	MEDIA_GRAPH_LINK,
	MEDIA_GRAPH_INTF_DEVNODE,
};

#define MEDIA_BITS_PER_TYPE		8
#define MEDIA_BITS_PER_ID		(32 - MEDIA_BITS_PER_TYPE)
#define MEDIA_ID_MASK			 GENMASK_ULL(MEDIA_BITS_PER_ID - 1, 0)

/* Structs to represent the objects that belong to a media graph */

/**
 * struct media_gobj - Define a graph object.
 *
 * @id:		Non-zero object ID identifier. The ID should be unique
 *		inside a media_device, as it is composed by
 *		%MEDIA_BITS_PER_TYPE to store the type plus
 *		%MEDIA_BITS_PER_ID to store the ID
 * @list:	List entry stored in one of the per-type mdev object lists
 *
 * All objects on the media graph should have this struct embedded
 */
struct media_gobj {
	struct media_device	*mdev;
	u32			id;
	struct list_head	list;
};

#define MEDIA_ENTITY_ENUM_MAX_DEPTH	16

/**
 * struct media_entity_enum - An enumeration of media entities.
 *
 * @bmap:	Bit map in which each bit represents one entity at struct
 *		media_entity->internal_idx.
 * @idx_max:	Number of bits in bmap
 */
struct media_entity_enum {
	unsigned long *bmap;
	int idx_max;
};

/**
 * struct media_entity_graph - Media graph traversal state
 *
 * @stack:		Graph traversal stack; the stack contains information
 *			on the path the media entities to be walked and the
 *			links through which they were reached.
 * @ent_enum:		Visited entities
 * @top:		The top of the stack
 */
struct media_entity_graph {
	struct {
		struct media_entity *entity;
		struct list_head *link;
	} stack[MEDIA_ENTITY_ENUM_MAX_DEPTH];

	struct media_entity_enum ent_enum;
	int top;
};

/*
 * struct media_pipeline - Media pipeline related information
 *
 * @streaming_count:	Streaming start count - streaming stop count
 * @graph:		Media graph walk during pipeline start / stop
 */
struct media_pipeline {
	int streaming_count;
	struct media_entity_graph graph;
};

/**
 * struct media_link - A link object part of a media graph.
 *
 * @graph_obj:	Embedded structure containing the media object common data
 * @list:	Linked list associated with an entity or an interface that
 *		owns the link.
 * @gobj0:	Part of a union. Used to get the pointer for the first
 *		graph_object of the link.
 * @source:	Part of a union. Used only if the first object (gobj0) is
 *		a pad. In that case, it represents the source pad.
 * @intf:	Part of a union. Used only if the first object (gobj0) is
 *		an interface.
 * @gobj1:	Part of a union. Used to get the pointer for the second
 *		graph_object of the link.
 * @source:	Part of a union. Used only if the second object (gobj1) is
 *		a pad. In that case, it represents the sink pad.
 * @entity:	Part of a union. Used only if the second object (gobj1) is
 *		an entity.
 * @reverse:	Pointer to the link for the reverse direction of a pad to pad
 *		link.
 * @flags:	Link flags, as defined in uapi/media.h (MEDIA_LNK_FL_*)
 * @is_backlink: Indicate if the link is a backlink.
 */
struct media_link {
	struct media_gobj graph_obj;
	struct list_head list;
	union {
		struct media_gobj *gobj0;
		struct media_pad *source;
		struct media_interface *intf;
	};
	union {
		struct media_gobj *gobj1;
		struct media_pad *sink;
		struct media_entity *entity;
	};
	struct media_link *reverse;
	unsigned long flags;
	bool is_backlink;
};

/**
 * struct media_pad - A media pad graph object.
 *
 * @graph_obj:	Embedded structure containing the media object common data
 * @entity:	Entity this pad belongs to
 * @index:	Pad index in the entity pads array, numbered from 0 to n
 * @flags:	Pad flags, as defined in uapi/media.h (MEDIA_PAD_FL_*)
 */
struct media_pad {
	struct media_gobj graph_obj;	/* must be first field in struct */
	struct media_entity *entity;
	u16 index;			/* Pad index in the entity pads array */
	unsigned long flags;
};

/**
 * struct media_entity_operations - Media entity operations
 * @link_setup:		Notify the entity of link changes. The operation can
 *			return an error, in which case link setup will be
 *			cancelled. Optional.
 * @link_validate:	Return whether a link is valid from the entity point of
 *			view. The media_entity_pipeline_start() function
 *			validates all links by calling this operation. Optional.
 */
struct media_entity_operations {
	int (*link_setup)(struct media_entity *entity,
			  const struct media_pad *local,
			  const struct media_pad *remote, u32 flags);
	int (*link_validate)(struct media_link *link);
};

/**
 * struct media_entity - A media entity graph object.
 *
 * @graph_obj:	Embedded structure containing the media object common data.
 * @name:	Entity name.
 * @function:	Entity main function, as defined in uapi/media.h
 *		(MEDIA_ENT_F_*)
 * @flags:	Entity flags, as defined in uapi/media.h (MEDIA_ENT_FL_*)
 * @num_pads:	Number of sink and source pads.
 * @num_links:	Total number of links, forward and back, enabled and disabled.
 * @num_backlinks: Number of backlinks
 * @internal_idx: An unique internal entity specific number. The numbers are
 *		re-used if entities are unregistered or registered again.
 * @pads:	Pads array with the size defined by @num_pads.
 * @links:	List of data links.
 * @ops:	Entity operations.
 * @stream_count: Stream count for the entity.
 * @use_count:	Use count for the entity.
 * @pipe:	Pipeline this entity belongs to.
 * @info:	Union with devnode information.  Kept just for backward
 *		compatibility.
 * @major:	Devnode major number (zero if not applicable). Kept just
 *		for backward compatibility.
 * @minor:	Devnode minor number (zero if not applicable). Kept just
 *		for backward compatibility.
 *
 * NOTE: @stream_count and @use_count reference counts must never be
 * negative, but are signed integers on purpose: a simple WARN_ON(<0) check
 * can be used to detect reference count bugs that would make them negative.
 */
struct media_entity {
	struct media_gobj graph_obj;	/* must be first field in struct */
	const char *name;
	u32 function;
	unsigned long flags;

	u16 num_pads;
	u16 num_links;
	u16 num_backlinks;
	int internal_idx;

	struct media_pad *pads;
	struct list_head links;

	const struct media_entity_operations *ops;	/* Entity operations */

	/* Reference counts must never be negative, but are signed integers on
	 * purpose: a simple WARN_ON(<0) check can be used to detect reference
	 * count bugs that would make them negative.
	 */
	int stream_count;		/* Stream count for the entity. */
	int use_count;			/* Use count for the entity. */

	struct media_pipeline *pipe;	/* Pipeline this entity belongs to. */

	union {
		/* Node specifications */
		struct {
			u32 major;
			u32 minor;
		} dev;

		/* Sub-device specifications */
		/* Nothing needed yet */
	} info;
};

/**
 * struct media_interface - A media interface graph object.
 *
 * @graph_obj:		embedded graph object
 * @links:		List of links pointing to graph entities
 * @type:		Type of the interface as defined in the
 *			uapi/media/media.h header, e. g.
 *			MEDIA_INTF_T_*
 * @flags:		Interface flags as defined in uapi/media/media.h
 */
struct media_interface {
	struct media_gobj		graph_obj;
	struct list_head		links;
	u32				type;
	u32				flags;
};

/**
 * struct media_intf_devnode - A media interface via a device node.
 *
 * @intf:	embedded interface object
 * @major:	Major number of a device node
 * @minor:	Minor number of a device node
 */
struct media_intf_devnode {
	struct media_interface		intf;

	/* Should match the fields at media_v2_intf_devnode */
	u32				major;
	u32				minor;
};

static inline u32 media_entity_type(struct media_entity *entity)
{
	return entity->function & MEDIA_ENT_TYPE_MASK;
}

static inline u32 media_entity_subtype(struct media_entity *entity)
{
	return entity->function & MEDIA_ENT_SUBTYPE_MASK;
}

static inline u32 media_entity_id(struct media_entity *entity)
{
	return entity->graph_obj.id;
}

static inline enum media_gobj_type media_type(struct media_gobj *gobj)
{
	return gobj->id >> MEDIA_BITS_PER_ID;
}

static inline u32 media_id(struct media_gobj *gobj)
{
	return gobj->id & MEDIA_ID_MASK;
}

static inline u32 media_gobj_gen_id(enum media_gobj_type type, u64 local_id)
{
	u32 id;

	id = type << MEDIA_BITS_PER_ID;
	id |= local_id & MEDIA_ID_MASK;

	return id;
}

static inline bool is_media_entity_v4l2_io(struct media_entity *entity)
{
	if (!entity)
		return false;

	switch (entity->function) {
	case MEDIA_ENT_F_IO_V4L:
	case MEDIA_ENT_F_IO_VBI:
	case MEDIA_ENT_F_IO_SWRADIO:
		return true;
	default:
		return false;
	}
}

static inline bool is_media_entity_v4l2_subdev(struct media_entity *entity)
{
	if (!entity)
		return false;

	switch (entity->function) {
	case MEDIA_ENT_F_V4L2_SUBDEV_UNKNOWN:
	case MEDIA_ENT_F_CAM_SENSOR:
	case MEDIA_ENT_F_FLASH:
	case MEDIA_ENT_F_LENS:
	case MEDIA_ENT_F_ATV_DECODER:
	case MEDIA_ENT_F_TUNER:
		return true;

	default:
		return false;
	}
}

/**
 * __media_entity_enum_init - Initialise an entity enumeration
 *
 * @ent_enum: Entity enumeration to be initialised
 * @idx_max: Maximum number of entities in the enumeration
 *
 * Return: Returns zero on success or a negative error code.
 */
__must_check int __media_entity_enum_init(struct media_entity_enum *ent_enum,
					  int idx_max);

/**
 * media_entity_enum_cleanup - Release resources of an entity enumeration
 *
 * @ent_enum: Entity enumeration to be released
 */
void media_entity_enum_cleanup(struct media_entity_enum *ent_enum);

/**
 * media_entity_enum_zero - Clear the entire enum
 *
 * @e: Entity enumeration to be cleared
 */
static inline void media_entity_enum_zero(struct media_entity_enum *ent_enum)
{
	bitmap_zero(ent_enum->bmap, ent_enum->idx_max);
}

/**
 * media_entity_enum_set - Mark a single entity in the enum
 *
 * @e: Entity enumeration
 * @entity: Entity to be marked
 */
static inline void media_entity_enum_set(struct media_entity_enum *ent_enum,
					 struct media_entity *entity)
{
	if (WARN_ON(entity->internal_idx >= ent_enum->idx_max))
		return;

	__set_bit(entity->internal_idx, ent_enum->bmap);
}

/**
 * media_entity_enum_clear - Unmark a single entity in the enum
 *
 * @e: Entity enumeration
 * @entity: Entity to be unmarked
 */
static inline void media_entity_enum_clear(struct media_entity_enum *ent_enum,
					   struct media_entity *entity)
{
	if (WARN_ON(entity->internal_idx >= ent_enum->idx_max))
		return;

	__clear_bit(entity->internal_idx, ent_enum->bmap);
}

/**
 * media_entity_enum_test - Test whether the entity is marked
 *
 * @e: Entity enumeration
 * @entity: Entity to be tested
 *
 * Returns true if the entity was marked.
 */
static inline bool media_entity_enum_test(struct media_entity_enum *ent_enum,
					  struct media_entity *entity)
{
	if (WARN_ON(entity->internal_idx >= ent_enum->idx_max))
		return true;

	return test_bit(entity->internal_idx, ent_enum->bmap);
}

/**
 * media_entity_enum_test - Test whether the entity is marked, and mark it
 *
 * @e: Entity enumeration
 * @entity: Entity to be tested
 *
 * Returns true if the entity was marked, and mark it before doing so.
 */
static inline bool media_entity_enum_test_and_set(
	struct media_entity_enum *ent_enum, struct media_entity *entity)
{
	if (WARN_ON(entity->internal_idx >= ent_enum->idx_max))
		return true;

	return __test_and_set_bit(entity->internal_idx, ent_enum->bmap);
}

/**
 * media_entity_enum_test - Test whether the entire enum is empty
 *
 * @e: Entity enumeration
 * @entity: Entity to be tested
 *
 * Returns true if the entity was marked.
 */
static inline bool media_entity_enum_empty(struct media_entity_enum *ent_enum)
{
	return bitmap_empty(ent_enum->bmap, ent_enum->idx_max);
}

/**
 * media_entity_enum_intersects - Test whether two enums intersect
 *
 * @e: First entity enumeration
 * @f: Second entity enumeration
 *
 * Returns true if entity enumerations e and f intersect, otherwise false.
 */
static inline bool media_entity_enum_intersects(
	struct media_entity_enum *ent_enum1,
	struct media_entity_enum *ent_enum2)
{
	WARN_ON(ent_enum1->idx_max != ent_enum2->idx_max);

	return bitmap_intersects(ent_enum1->bmap, ent_enum2->bmap,
				 min(ent_enum1->idx_max, ent_enum2->idx_max));
}

#define gobj_to_entity(gobj) \
		container_of(gobj, struct media_entity, graph_obj)

#define gobj_to_pad(gobj) \
		container_of(gobj, struct media_pad, graph_obj)

#define gobj_to_link(gobj) \
		container_of(gobj, struct media_link, graph_obj)

#define gobj_to_link(gobj) \
		container_of(gobj, struct media_link, graph_obj)

#define gobj_to_pad(gobj) \
		container_of(gobj, struct media_pad, graph_obj)

#define gobj_to_intf(gobj) \
		container_of(gobj, struct media_interface, graph_obj)

#define intf_to_devnode(intf) \
		container_of(intf, struct media_intf_devnode, intf)

void media_gobj_create(struct media_device *mdev,
		    enum media_gobj_type type,
		    struct media_gobj *gobj);
void media_gobj_destroy(struct media_gobj *gobj);

int media_entity_pads_init(struct media_entity *entity, u16 num_pads,
		struct media_pad *pads, u16 extra_links);
void media_entity_cleanup(struct media_entity *entity);

__must_check int media_create_pad_link(struct media_entity *source,
			u16 source_pad, struct media_entity *sink,
			u16 sink_pad, u32 flags);

/**
 * media_create_pad_links() - creates a link between two entities.
 *
 * @mdev: Pointer to the media_device that contains the object
 * @source_function: Function of the source entities. Used only if @source is
 *	NULL.
 * @source: pointer to &media_entity of the source pad. If NULL, it will use
 * 	all entities that matches the @sink_function.
 * @source_pad: number of the source pad in the pads array
 * @sink_function: Function of the sink entities. Used only if @sink is NULL.
 * @sink: pointer to &media_entity of the sink pad. If NULL, it will use
 * 	all entities that matches the @sink_function.
 * @sink_pad: number of the sink pad in the pads array.
 * @flags: Link flags, as defined in include/uapi/linux/media.h.
 * @allow_both_undefined: if true, then both @source and @sink can be NULL.
 *	In such case, it will create a crossbar between all entities that
 *	matches @source_function to all entities that matches @sink_function.
 *	If false, it will return 0 and won't create any link if both @source
 *	and @sink are NULL.
 *
 * Valid values for flags:
 * A %MEDIA_LNK_FL_ENABLED flag indicates that the link is enabled and can be
 *	used to transfer media data. If multiple links are created and this
 *	flag is passed as an argument, only the first created link will have
 *	this flag.
 *
 * A %MEDIA_LNK_FL_IMMUTABLE flag indicates that the link enabled state can't
 *	be modified at runtime. If %MEDIA_LNK_FL_IMMUTABLE is set, then
 *	%MEDIA_LNK_FL_ENABLED must also be set since an immutable link is
 *	always enabled.
 *
 * It is common for some devices to have multiple source and/or sink entities
 * of the same type that should be linked. While media_create_pad_link()
 * creates link by link, this function is meant to allow 1:n, n:1 and even
 * cross-bar (n:n) links.
 *
 * NOTE: Before calling this function, media_entity_pads_init() and
 * media_device_register_entity() should be called previously for the entities
 * to be linked.
 */
int media_create_pad_links(const struct media_device *mdev,
			   const u32 source_function,
			   struct media_entity *source,
			   const u16 source_pad,
			   const u32 sink_function,
			   struct media_entity *sink,
			   const u16 sink_pad,
			   u32 flags,
			   const bool allow_both_undefined);

void __media_entity_remove_links(struct media_entity *entity);
void media_entity_remove_links(struct media_entity *entity);

int __media_entity_setup_link(struct media_link *link, u32 flags);
int media_entity_setup_link(struct media_link *link, u32 flags);
struct media_link *media_entity_find_link(struct media_pad *source,
		struct media_pad *sink);
struct media_pad *media_entity_remote_pad(struct media_pad *pad);

struct media_entity *media_entity_get(struct media_entity *entity);

__must_check int media_entity_graph_walk_init(
	struct media_entity_graph *graph, struct media_device *mdev);
void media_entity_graph_walk_cleanup(struct media_entity_graph *graph);

/**
 * media_entity_put - Release the reference to the parent module
 *
 * @entity: The entity
 *
 * Release the reference count acquired by media_entity_get().
 *
 * The function will return immediately if @entity is NULL.
 */
void media_entity_put(struct media_entity *entity);

/**
 * media_entity_graph_walk_start - Start walking the media graph at a given entity
 * @graph: Media graph structure that will be used to walk the graph
 * @entity: Starting entity
 *
 * Before using this function, media_entity_graph_walk_init() must be
 * used to allocate resources used for walking the graph. This
 * function initializes the graph traversal structure to walk the
 * entities graph starting at the given entity. The traversal
 * structure must not be modified by the caller during graph
 * traversal. After the graph walk, the resources must be released
 * using media_entity_graph_walk_cleanup().
 */
void media_entity_graph_walk_start(struct media_entity_graph *graph,
				   struct media_entity *entity);

/**
 * media_entity_graph_walk_next - Get the next entity in the graph
 * @graph: Media graph structure
 *
 * Perform a depth-first traversal of the given media entities graph.
 *
 * The graph structure must have been previously initialized with a call to
 * media_entity_graph_walk_start().
 *
 * Return the next entity in the graph or NULL if the whole graph have been
 * traversed.
 */
struct media_entity *
media_entity_graph_walk_next(struct media_entity_graph *graph);
__must_check int media_entity_pipeline_start(struct media_entity *entity,
					     struct media_pipeline *pipe);
void media_entity_pipeline_stop(struct media_entity *entity);

struct media_intf_devnode *
__must_check media_devnode_create(struct media_device *mdev,
				  u32 type, u32 flags,
				  u32 major, u32 minor);
void media_devnode_remove(struct media_intf_devnode *devnode);
struct media_link *
__must_check media_create_intf_link(struct media_entity *entity,
				    struct media_interface *intf,
				    u32 flags);
void __media_remove_intf_link(struct media_link *link);
void media_remove_intf_link(struct media_link *link);
void __media_remove_intf_links(struct media_interface *intf);

/**
 * media_remove_intf_links() - remove all links associated with an interface
 *
 * @intf:	pointer to &media_interface
 *
 * Notes:
 *
 * this is called automatically when an entity is unregistered via
 * media_device_register_entity() and by media_devnode_remove().
 *
 * Prefer to use this one, instead of __media_remove_intf_links().
 */
void media_remove_intf_links(struct media_interface *intf);

#define media_entity_call(entity, operation, args...)			\
	(((entity)->ops && (entity)->ops->operation) ?			\
	 (entity)->ops->operation((entity) , ##args) : -ENOIOCTLCMD)

#endif
