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

struct fs_parameter {
	const char *key;
	const char *string;
};

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

/* For kernel 4.19: use legacy mount API to implement submount support */
struct fuse_fs_context {
	struct fuse_conn *fc;
	struct fuse_mount *fm;
	struct dentry *reference;
};

static inline struct fs_context *fs_context_for_submount(struct file_system_type *type, struct dentry *reference)
{
	struct fs_context *fc;

	fc = kzalloc(sizeof(*fc), GFP_KERNEL);
	if (!fc)
		return ERR_PTR(-ENOMEM);

	fc->reference = dget(reference);
	fc->need_free = true;
	return fc;
}

static inline void put_fs_context(struct fs_context *fc)
{
	if (fc) {
		if (fc->root)
			dput(fc->root);
		if (fc->reference)
			dput(fc->reference);
		if (fc->need_free)
			kfree(fc);
	}
}

static inline struct super_block *sget_fc(struct fs_context *fc,
					  int (*test)(struct super_block *, void *),
					  int (*set)(struct super_block *, void *))
{
	struct fuse_fs_context *ctx = fc->s_fs_info;
	struct super_block *sb;
	int err;

	sb = sget(fc->reference->d_sb->s_type, test, set, SB_NOSEC, NULL);
	if (IS_ERR(sb))
		return sb;

	if (!sb->s_root) {
		err = 0;
	} else {
		err = -EBUSY;
	}

	if (err) {
		deactivate_locked_super(sb);
		return ERR_PTR(err);
	}

	return sb;
}

static inline struct vfsmount *vfs_create_mount(struct fs_context *fc)
{
	struct vfsmount *mnt;

	if (!fc->root)
		return ERR_PTR(-EINVAL);

	mnt = vfs_kern_mount(fc->reference->d_sb->s_type, 0,
			     fc->reference->d_sb->s_type->name, NULL);
	if (IS_ERR(mnt))
		return mnt;

	/* Replace the root dentry */
	dput(mnt->mnt_root);
	mnt->mnt_root = dget(fc->root);

	return mnt;
}

#endif /* _LINUX_FS_CONTEXT_H */
