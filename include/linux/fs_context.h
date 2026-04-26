/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Backported fs_context.h for kernel 4.19 compatibility
 * Original fs_context API was introduced in kernel 5.2
 */
#ifndef _LINUX_FS_CONTEXT_H
#define _LINUX_FS_CONTEXT_H

#include <linux/fs.h>
#include <linux/mount.h>
#include <linux/slab.h>

struct fs_context {
	const struct fs_context_operations *ops;
	struct file		*reference;
	struct dentry		*root;
	struct user_namespace	*user_ns;
	struct net		*net_ns;
	const struct cred	*cred;
	char			*source;
	char			*subtype;
	void			*fs_private;
	void			*s_fs_info;
	unsigned int		sb_flags;
	unsigned int		sb_flags_mask;
	unsigned int		s_iflags;
	unsigned int		lsm_flags;
	char			*device;
	char			*data;
	enum fs_context_purpose {
		FS_CONTEXT_FOR_MOUNT		= 0,
		FS_CONTEXT_FOR_SUBMOUNT		= 1,
		FS_CONTEXT_FOR_RECONFIGURE	= 2,
	}			purpose;
	enum fs_phase {
		INITING_CONTEXT		= 0,
		GETTING_TREE		= 1,
		AWAITING_INIT		= 2,
		CREATING_CHILD		= 3,
		RECONFIGURING		= 4,
	}			phase;
	bool			need_free;
};

struct fs_context_operations {
	void (*free)(struct fs_context *fc);
	int (*dup)(struct fs_context *fc, struct fs_context *src_fc);
	int (*parse_param)(struct fs_context *fc, struct fs_parameter *param);
	int (*parse_monolithic)(struct fs_context *fc, void *data);
	int (*get_tree)(struct fs_context *fc);
	int (*reconfigure)(struct fs_context *fc);
};

static inline struct fs_context *fs_context_for_submount(struct file_system_type *type, struct dentry *reference)
{
	return ERR_PTR(-EOPNOTSUPP);
}

static inline void put_fs_context(struct fs_context *fc)
{
	if (fc && fc->need_free)
		kfree(fc);
}

static inline struct vfsmount *vfs_create_mount(struct fs_context *fc)
{
	return ERR_PTR(-EOPNOTSUPP);
}

#endif /* _LINUX_FS_CONTEXT_H */
