/*
 * SO2 Lab - Filesystem drivers (part 2)
 * Type #1 (no-dev filesystem)
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/mm.h>
#include <linux/sched.h>

MODULE_DESCRIPTION("Simple no-dev filesystem");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

#define MYFS_MAGIC	0xdeadbeef
#define LOG_LEVEL	KERN_ALERT

/* declarations of functions that are part of operation structures */

static int myfs_mknod(struct inode *dir,
		struct dentry *dentry, umode_t mode, dev_t dev);
static int myfs_create(struct inode *dir, struct dentry *dentry,
		umode_t mode, bool excl);
static int myfs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode);

/* dir and file operation structures */

static const struct inode_operations myfs_dir_inode_operations = {
	.create         = myfs_create,
	.lookup         = simple_lookup,
	.link           = simple_link,
	.unlink         = simple_unlink,
	.mkdir          = myfs_mkdir,
	.rmdir          = simple_rmdir,
	.mknod          = myfs_mknod,
	.rename         = simple_rename,
};

static const struct file_operations myfs_file_operations = {
	.read_iter      = generic_file_read_iter,
	.write_iter     = generic_file_write_iter,
	.mmap           = generic_file_mmap,
	.llseek         = generic_file_llseek,
};

static const struct inode_operations myfs_file_inode_operations = {
	.getattr        = simple_getattr,
};

static const struct address_space_operations myfs_aops = {
	.readpage       = simple_readpage,
	.write_begin    = simple_write_begin,
	.write_end      = simple_write_end,
};

static struct inode *myfs_get_inode(struct super_block *sb,
		const struct inode *dir, int mode)
{
	struct inode *inode = new_inode(sb);

	if (!inode)
		return NULL;

	inode->i_ino = get_next_ino();
	inode_init_owner(inode, dir, mode);
	inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
	inode->i_mapping->a_ops = &myfs_aops;

	if (S_ISDIR(mode)) {
		inode->i_op = &myfs_dir_inode_operations;
		inode->i_fop = &simple_dir_operations;
		/* directory inodes start off with i_nlink == 2 (for "." entry)
		 * */
		inc_nlink(inode);
	}
	if (S_ISREG(mode)) {
		inode->i_op = &myfs_file_inode_operations;
		inode->i_fop = &myfs_file_operations;
	}

	return inode;
}

static int myfs_mknod(struct inode *dir,
		struct dentry *dentry, umode_t mode, dev_t dev)
{
	struct inode *inode = myfs_get_inode(dir->i_sb, dir, mode);

	if (inode == NULL)
		return -ENOSPC;

	d_instantiate(dentry, inode);
	dget(dentry);
	dir->i_mtime = dir->i_ctime = CURRENT_TIME;

	return 0;
}

static int myfs_create(struct inode *dir, struct dentry *dentry,
		umode_t mode, bool excl)
{
	return myfs_mknod(dir, dentry, mode | S_IFREG, 0);
}

static int myfs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
{
	int ret;

	ret = myfs_mknod(dir, dentry, mode | S_IFDIR, 0);
	if (ret != 0)
		return ret;

	inc_nlink(dir);

	return 0;
}

static const struct super_operations myfs_ops = {
	.statfs		= simple_statfs,
	.drop_inode	= generic_drop_inode,
};

static int myfs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct inode *root_inode;
	struct dentry *root_dentry;

	sb->s_maxbytes = MAX_LFS_FILESIZE;
	sb->s_blocksize = PAGE_SIZE;
	sb->s_blocksize_bits = PAGE_SHIFT;
	sb->s_magic = MYFS_MAGIC;
	sb->s_op = &myfs_ops;

	/* mode = directory & access rights (755) */
	root_inode = myfs_get_inode(sb, NULL,
			S_IFDIR | S_IRWXU | S_IRGRP |
			S_IXGRP | S_IROTH | S_IXOTH);
	if (!root_inode)
		goto out_no_root;

	root_dentry = d_make_root(root_inode);
	if (!root_dentry)
		goto out_no_root;
	sb->s_root = root_dentry;

	return 0;

out_no_root:
	iput(root_inode);
	return -ENOMEM;
}

static struct dentry *myfs_mount(struct file_system_type *fs_type,
		int flags, const char *dev_name, void *data)
{
	return mount_nodev(fs_type, flags, data, myfs_fill_super);
}

static struct file_system_type myfs_fs_type = {
	.owner = THIS_MODULE,
	.name = "myfs",
	.mount = myfs_mount,
	.kill_sb = kill_litter_super,
};

static int __init myfs_init(void)
{
	int err;

	err = register_filesystem(&myfs_fs_type);
	if (err) {
		printk(LOG_LEVEL "register_filesystem failed\n");
		return err;
	}

	return 0;
}

static void __exit myfs_exit(void)
{
	unregister_filesystem(&myfs_fs_type);
}

module_init(myfs_init);
module_exit(myfs_exit);
