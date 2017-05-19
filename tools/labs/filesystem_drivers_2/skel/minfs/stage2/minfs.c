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
static int minfs_create(struct inode *dir, struct dentry *dentry,
		umode_t mode, bool excl);

/* dir and inode operation structures */

static const struct address_space_operations minfs_aops = {
	.readpage       = simple_readpage,
	.write_begin    = simple_write_begin,
	.write_end      = simple_write_end,
};

static const struct file_operations minfs_file_operations = {
	.read_iter	= generic_file_read_iter,
	.write_iter	= generic_file_write_iter,
	.mmap		= generic_file_mmap,
	.llseek		= generic_file_llseek,
};

static const struct inode_operations minfs_file_inode_operations = {
	.getattr	= simple_getattr,
};

static const struct file_operations minfs_dir_operations = {
	.read		= generic_read_dir,
	.iterate	= minfs_readdir,
};

static const struct inode_operations minfs_dir_inode_operations = {
	.lookup		= minfs_lookup,
	.create		= minfs_create,
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
	inode->i_mapping->a_ops = &minfs_aops;

	if (S_ISDIR(inode->i_mode)) {
		inode->i_op = &minfs_dir_inode_operations;
		inode->i_fop = &minfs_dir_operations;

		/* Directory inodes start off with i_nlink == 2. */
		inc_nlink(inode);
	}
	if (S_ISREG(inode->i_mode)) {
		inode->i_op = &minfs_file_inode_operations;
		inode->i_fop = &minfs_file_operations;
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
	/* TODO 5: Copy-paste content from stage 1. */
	return 0;
}

/*
 * Find dentry in parent folder. Return parent folder's data buffer_head.
 */

static struct minfs_dir_entry *minfs_find_entry(struct dentry *dentry,
		struct buffer_head **bhp)
{
	/* TODO 5: Copy-paste content from stage 1. */
	return NULL;
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
 * Create a new VFS inode. Do basic initialization and fill imap.
 */

static struct inode *minfs_new_inode(struct inode *dir)
{
	struct super_block *sb = dir->i_sb;
	struct minfs_sb_info *sbi = sb->s_fs_info;
	struct inode *inode;
	int idx;

	/* TODO 5: Find first available inode and mark it as used. */

	/* TODO 5: Mark superblock buffer head as dirty. */

	/*
	 * TODO 5: Call new_inode(), fill inode fields
	 * and insert inode into inode hash table.
	 */

	return inode;
}

/*
 * Add dentry link on parent inode disk structure.
 */

static int minfs_add_link(struct dentry *dentry, struct inode *inode)
{
	struct buffer_head *bh;
	/* TODO 5: directory inode & containing inode */
	/* struct inode *dir = ... */
	/* const char *name = ... */
	/* struct super_block *sb = dir->i_sb; */
	/* struct minfs_inode_info *mii = ... */
	struct minfs_dir_entry *de;
	int i;
	int err = 0;

	/* TODO 5: Uncomment next line. */
	/* bh = sb_bread(sb, mii->data_block) */

	/* TODO 5: Find first free inode (de->ino == 0). */

	/*
	 * TODO 5: Place new entry in the available slot. Mark buffer_head as dirty.
	 */

out:
	brelse(bh);

	return err;
}

/*
 * Create a VFS file inode. Use minfs_file_... operations.
 */

static int minfs_create(struct inode *dir, struct dentry *dentry, umode_t mode,
		bool excl)
{
	struct inode *inode;
	struct minfs_inode_info *mii;
	int err;

	inode = minfs_new_inode(dir);
	if (inode == NULL) {
		printk(LOG_LEVEL "error allocating new inode\n");
		err = -ENOMEM;
		goto err_new_inode;
	}

	inode->i_mode = mode;
	inode->i_op = &minfs_file_inode_operations;
	inode->i_fop = &minfs_file_operations;
	mii = container_of(inode, struct minfs_inode_info, vfs_inode);
	mii->data_block = MINFS_FIRST_DATA_BLOCK + inode->i_ino;

	err = minfs_add_link(dentry, inode);
	if (err != 0)
		goto err_add_link;

	d_instantiate(dentry, inode);
	mark_inode_dirty(inode);

	dprintk("new file inode created (ino = %lu)\n", inode->i_ino);

	return 0;

err_add_link:
	inode_dec_link_count(inode);
	iput(inode);
err_new_inode:
	return err;
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

/*
 * Write VFS inode contents to disk inode.
 */

static int minfs_write_inode(struct inode *inode,
		struct writeback_control *wbc)
{
	struct super_block *sb = inode->i_sb;
	struct minfs_inode *mi;
	struct minfs_inode_info *mii = container_of(inode,
			struct minfs_inode_info, vfs_inode);
	struct buffer_head *bh;
	int err = 0;

	bh = sb_bread(sb, MINFS_INODE_BLOCK);
	if (bh == NULL) {
		printk(LOG_LEVEL "could not read block\n");
		err = -ENOMEM;
		goto out;
	}

	mi = (struct minfs_inode *) bh->b_data + inode->i_ino;

	/* fill disk inode */
	mi->mode = inode->i_mode;
	i_uid_write(inode, mi->uid);
	i_gid_write(inode, mi->gid);
	mi->size = inode->i_size;
	mi->data_block = mii->data_block;

	dprintk("mode is %05o; data_block is %d\n", mi->mode, mii->data_block);

	mark_buffer_dirty(bh);
	brelse(bh);

	dprintk("wrote inode %lu\n", inode->i_ino);

out:
	return err;
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
	.write_inode	= minfs_write_inode,
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
	return mount_bdev(fs_type, flags, dev_name, data, minfs_fill_super);
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
