/*
 * SO2 Lab - Filesystem drivers (part 1)
 * Exercise #1 (no-dev filesystem)
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/pagemap.h>

MODULE_DESCRIPTION("Simple no-dev filesystem");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

#define MYFS_BLOCKSIZE		4096
#define MYFS_BLOCKSIZE_BITS	12
#define MYFS_MAGIC		0xbeefcafe
#define LOG_LEVEL		KERN_ALERT

/* TODO 2: define super_operations structure */

struct inode *myfs_get_inode(struct super_block *sb, int mode)
{
	struct inode *inode = new_inode(sb);

	if (!inode)
		return NULL;

	/* TODO 3: fill inode structure
	 *     - mode
	 *     - uid
	 *     - gid
	 *     - atime,ctime,mtime
	 */

	/*
	 * TODO 3: inode operations for root inode (use S_ISDIR macro)
	 *     directory link count should be incremented (use inc_nlink)
	 */

	return inode;
}

static int myfs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct inode *root_inode;
	struct dentry *root_dentry;

	/* TODO 2: fill super_block
	 *   - blocksize, blocksize_bits
	 *   - magic
	 *   - super operations
	 *   - maxbytes
	 */

	/* mode = directory & access rights (755) */
	root_inode = myfs_get_inode(sb, S_IFDIR | S_IRWXU | S_IRGRP |
			S_IXGRP | S_IROTH | S_IXOTH);
	root_inode->i_ino = 1;

	printk(LOG_LEVEL "root inode has %d link(s)\n", root_inode->i_nlink);

	if (!root_inode)
		return -ENOMEM;

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
	/* TODO 1: call superblock mount function */
}


/* TODO 1: define file_system_type structure */

static int __init myfs_init(void)
{
	int err;

	/* TODO 1: register */
	if (err) {
		printk(LOG_LEVEL "register_filesystem failed\n");
		return err;
	}

	return 0;
}

static void __exit myfs_exit(void)
{
	/* TODO 1: unregister */
}

module_init(myfs_init);
module_exit(myfs_exit);
