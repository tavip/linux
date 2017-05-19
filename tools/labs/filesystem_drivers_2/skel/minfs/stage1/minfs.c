/*
 * SO2 Lab - Filesystem drivers (part 2)
 * Type #2 (dev filesystem)
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/buffer_head.h>
#include <linux/slab.h>

#include "minfs.h"

MODULE_DESCRIPTION("Simple filesystem");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");


#define LOG_LEVEL	KERN_ALERT
#define DEBUG		1

#if DEBUG == 1
#define dprintk(fmt, ...)	\
	printk(LOG_LEVEL fmt, ##__VA_ARGS__)
#else
#define dprintk(fmt, ...)	\
	do {} while (0)
#endif

/* module specific data structures for superblock and inode */

struct minfs_sb_info {
	__u8 version;
	unsigned long imap;
	struct buffer_head *sbh;
};

struct minfs_inode_info {
	__u16 data_block;
	struct inode vfs_inode;
};

/* declarations of functions that are part of operation structures */

static int minfs_readdir(struct file *filp, struct dir_context *ctx);
static struct dentry *minfs_lookup(struct inode *dir,
		struct dentry *dentry, unsigned int flags);

/* dir and inode operation structures */

static const struct file_operations minfs_dir_operations = {
	.read		= generic_read_dir,
	.iterate	= minfs_readdir,
};

static const struct inode_operations minfs_dir_inode_operations = {
	/* TODO 4: Replace with minfs_lookup. */
	.lookup		= simple_lookup,
};

static struct inode *minfs_iget(struct super_block *s, unsigned long ino)
{
	struct minfs_inode *mi;
	struct buffer_head *bh;
	struct inode *inode;
	struct minfs_inode_info *mii;

	/* Allocate VFS inode. */
	inode = iget_locked(s, ino);
	if (inode == NULL) {
		printk(LOG_LEVEL "error aquiring inode\n");
		return ERR_PTR(-ENOMEM);
	}
	if (!(inode->i_state & I_NEW))
		return inode;

	/* Read disk inode block. */
	bh = sb_bread(s, MINFS_INODE_BLOCK);
	if (bh == NULL) {
		printk(LOG_LEVEL "could not read block\n");
		goto out_bad_sb;
	}

	/* Extract disk inode. */
	mi = ((struct minfs_inode *) bh->b_data) + ino;

	/* Fill VFS inode. */
	inode->i_mode = mi->mode;
	i_uid_write(inode, mi->uid);
	i_gid_write(inode, mi->gid);
	inode->i_size = mi->size;
	inode->i_blocks = 0;
	inode->i_mtime = inode->i_atime = inode->i_ctime = CURRENT_TIME;

	if (S_ISDIR(inode->i_mode)) {
		inode->i_op = &minfs_dir_inode_operations;
		inode->i_fop = &minfs_dir_operations;

		/* Directory inodes start off with i_nlink == 2. */
		inc_nlink(inode);
	}

	/* Fill data for mii. */
	mii = container_of(inode, struct minfs_inode_info, vfs_inode);
	mii->data_block = mi->data_block;

	/* Free resources. */
	brelse(bh);
	unlock_new_inode(inode);

	dprintk("got inode %lu\n", ino);

	return inode;

out_bad_sb:
	iget_failed(inode);
	return NULL;
}

static int minfs_readdir(struct file *filp, struct dir_context *ctx)
{
	struct buffer_head *bh;
	struct minfs_dir_entry *de;
	/* TODO 3: Get inode of directory and container inode. */
	/* struct inode *inode = ... */
	/* struct minfs_inode_info *mii = ... */
	/* struct super_block *sb = inode->i_sb; */
	int over;
	int err = 0;

	/* TODO 3: Read data block for directory inode. */

	for (; ctx->pos < MINFS_NUM_ENTRIES; ctx->pos++) {
		/*
		 * TODO 3: Data block contains an array of
		 * "struct minfs_dir_entry". Use `de' for storing.
		 */

		/* TODO 3: Step over empty entries (de->ino == 0). */

		/*
		 * Use `over' to store return value of dir_emit and exit
		 * if required.
		 */
		over = dir_emit(ctx, de->name, MINFS_NAME_LEN, de->ino,
				DT_UNKNOWN);
		if (over) {
			dprintk("Read %s from folder %s, ctx->pos: %lld\n",
					de->name,
					filp->f_path.dentry->d_name.name,
					ctx->pos);
			ctx->pos++;
			goto done;
		}
	}

done:
	brelse(bh);
out_bad_sb:
	return err;
}

/*
 * Find dentry in parent folder. Return parent folder's data buffer_head.
 */

static struct minfs_dir_entry *minfs_find_entry(struct dentry *dentry,
		struct buffer_head **bhp)
{
	struct buffer_head *bh;
	struct inode *dir = dentry->d_parent->d_inode;
	struct minfs_inode_info *mii = container_of(dir,
			struct minfs_inode_info, vfs_inode);
	struct super_block *sb = dir->i_sb;
	const char *name = dentry->d_name.name;
	struct minfs_dir_entry *final_de = NULL;
	struct minfs_dir_entry *de;
	int i;

	/*
	 * TODO 4: Read parent folder data block (contains dentries).
	 * Fill bhp with return value.
	 */

	for (i = 0; i < MINFS_NUM_ENTRIES; i++) {
		/*
		 * TODO 4: Traverse all entries, find entry by name
		 * Use `de' to traverse. Use `final_de' to store dentry
		 * found, if existing.
		 */
	}

	/* bh needs to be released by caller. */
	return final_de;
}

static struct dentry *minfs_lookup(struct inode *dir,
		struct dentry *dentry, unsigned int flags)
{
	struct super_block *sb = dir->i_sb;
	struct minfs_dir_entry *de;
	struct buffer_head *bh = NULL;
	struct inode *inode = NULL;

	dentry->d_op = sb->s_root->d_op;

	de = minfs_find_entry(dentry, &bh);
	if (de != NULL) {
		dprintk("getting entry: name: %s, ino: %d\n",
				de->name, de->ino);
		inode = minfs_iget(sb, de->ino);
		if (IS_ERR(inode))
			return ERR_CAST(inode);
	}

	d_add(dentry, inode);
	brelse(bh);

	dprintk("looked up dentry %s\n", dentry->d_name.name);

	return NULL;
}

/*
 * Allocate and initialize VFS inode. Filling is done in new_inode or
 * minfs_iget.
 */

static struct inode *minfs_alloc_inode(struct super_block *s)
{
	struct minfs_inode_info *mii;

	/* Allocate minfs_inode_info. */
	mii = kzalloc(sizeof(struct minfs_inode_info), GFP_KERNEL);
	if (mii == NULL)
		return NULL;

	inode_init_once(&mii->vfs_inode);

	return &mii->vfs_inode;
}

static void minfs_destroy_inode(struct inode *inode)
{
	kfree(container_of(inode, struct minfs_inode_info, vfs_inode));
}

static void minfs_put_super(struct super_block *sb)
{
	struct minfs_sb_info *sbi = sb->s_fs_info;

	/* Free superblock buffer head. */
	mark_buffer_dirty(sbi->sbh);
	brelse(sbi->sbh);

	dprintk("released superblock resources\n");
}

static const struct super_operations minfs_ops = {
	.statfs		= simple_statfs,
	.alloc_inode	= minfs_alloc_inode,
	.destroy_inode	= minfs_destroy_inode,
	.put_super	= minfs_put_super,
};

static int minfs_fill_super(struct super_block *s, void *data, int silent)
{
	struct minfs_sb_info *sbi;
	struct minfs_super_block *ms;
	struct inode *root_inode;
	struct dentry *root_dentry;
	struct buffer_head *bh;
	int ret = -EINVAL;

	sbi = kzalloc(sizeof(struct minfs_sb_info), GFP_KERNEL);
	if (!sbi)
		return -ENOMEM;
	s->s_fs_info = sbi;

	/* Set block size for superblock. */
	if (!sb_set_blocksize(s, MINFS_BLOCK_SIZE))
		goto out_bad_blocksize;

	/* Read first block from disk (contains disk superblock). */
	bh = sb_bread(s, MINFS_SUPER_BLOCK);
	if (bh == NULL)
		goto out_bad_sb;

	/* Extract disk superblock. */
	ms = (struct minfs_super_block *) bh->b_data;

	/* Fill sbi with information from disk superblock. */
	if (ms->magic == MINFS_MAGIC) {
		sbi->version = ms->version;
		sbi->imap = ms->imap;
	} else
		goto out_bad_magic;

	s->s_magic = MINFS_MAGIC;
	s->s_op = &minfs_ops;

	/* Allocate root inode and root dentry. */
	root_inode = minfs_iget(s, MINFS_ROOT_INODE);
	if (!root_inode)
		goto out_bad_inode;

	root_dentry = d_make_root(root_inode);
	if (!root_dentry)
		goto out_iput;
	s->s_root = root_dentry;

	/* Store superblock buffer_head for further use. */
	sbi->sbh = bh;

	dprintk("superblock filled\n");

	return 0;

out_iput:
	iput(root_inode);
out_bad_inode:
	printk(LOG_LEVEL "bad inode\n");
out_bad_magic:
	printk(LOG_LEVEL "bad magic number\n");
	brelse(bh);
out_bad_sb:
	printk(LOG_LEVEL "error reading buffer_head\n");
out_bad_blocksize:
	printk(LOG_LEVEL "bad block size\n");
	s->s_fs_info = NULL;
	kfree(sbi);
	return ret;
}

static struct dentry *minfs_mount(struct file_system_type *fs_type,
		int flags, const char *dev_name, void *data)
{
	return mount_bdev(fs_type, flags, dev_name, data,
			minfs_fill_super);
}

static struct file_system_type minfs_fs_type = {
	.owner		= THIS_MODULE,
	.name		= "minfs",
	.mount		= minfs_mount,
	.kill_sb	= kill_block_super,
	.fs_flags	= FS_REQUIRES_DEV,
};

static int __init minfs_init(void)
{
	int err;

	err = register_filesystem(&minfs_fs_type);
	if (err) {
		printk(LOG_LEVEL "register_filesystem failed\n");
		return err;
	}

	dprintk("registered filesystem\n");

	return 0;
}

static void __exit minfs_exit(void)
{
	unregister_filesystem(&minfs_fs_type);
	dprintk("unregistered filesystem\n");
}

module_init(minfs_init);
module_exit(minfs_exit);
