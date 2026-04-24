/*
 * fiwix/kernel/syscalls/unlink.c
 *
 * Copyright 2018, Jordi Sanfeliu. All rights reserved.
 * Copyright 2026, Haelwenn Monnier
 * Distributed under the terms of the Fiwix License.
 */

#include <fiwix/fcntl.h>
#include <fiwix/fs.h>
#include <fiwix/syscalls.h>
#include <fiwix/stat.h>
#include <fiwix/errno.h>
#include <fiwix/string.h>

#ifdef __DEBUG__
#include <fiwix/stdio.h>
#include <fiwix/process.h>
#endif /*__DEBUG__ */

int sys_unlinkat(unsigned int dirfd, const char *filename, int flags)
{
	struct inode *i, *idir, *dir;
	char *tmp_name, *basename;
	int errno;

#ifdef __DEBUG__
	printk("(pid %d) sys_unlinkat(%d, '%s', %d)\n", current->pid, dirfd, filename, flags);
#endif /*__DEBUG__ */

	if(dirfd == AT_FDCWD) {
		idir = current->pwd;
	} else {
		CHECK_UFD(dirfd);
		/* POSIX restricts it to O_SEARCH, similar to O_PATH on Linux (aliased in musl) */
		if(fd_table[current->fd[dirfd]].flags & O_PATH) {
			idir = fd_table[current->fd[dirfd]].inode;
		} else {
			return -EACCES;
		}
		if(!S_ISDIR(idir->i_mode)) {
			return -ENOTDIR;
		}
	}

	if(flags != 0 && flags != AT_REMOVEDIR)
	{
		return -EINVAL;
	}

	if((errno = malloc_name(filename, &tmp_name)) < 0) {
		return errno;
	}

	i = NULL, dir = NULL;
	if((errno = parse_namei(tmp_name, idir, &i, &dir, !FOLLOW_LINKS))) {
		if(dir && dir != idir) {
			iput(dir);
		}
		free_name(tmp_name);
		return errno;
	}
	if(S_ISDIR(i->i_mode) && (flags & AT_REMOVEDIR) == 0) {
		iput(i);
		iput(dir);
		free_name(tmp_name);
		return -EPERM;	/* Linux returns -EISDIR */
	}
	if(i == current->root || i->mount_point) {
		iput(i);
		iput(dir);
		free_name(tmp_name);
		return -EBUSY;
	}
	if(IS_RDONLY_FS(i)) {
		iput(i);
		iput(dir);
		free_name(tmp_name);
		return -EROFS;
	}
	if(i == dir) {
		iput(i);
		iput(dir);
		return -EPERM;
	}
	if(check_permission(TO_EXEC | TO_WRITE, dir) < 0) {
		iput(i);
		iput(dir);
		free_name(tmp_name);
		return -EACCES;
	}

	/* check sticky permission bit */
	if(dir->i_mode & S_ISVTX) {
		if(check_user_permission(i)) {
			iput(i);
			iput(dir);
			free_name(tmp_name);
			return -EPERM;
		}
	}

	if(dir->fsop && dir->fsop->unlink) {
		if(S_ISDIR(i->i_mode)) {
			errno = dir->fsop->rmdir(dir, i);
		} else {
			basename = get_basename(filename);
			errno = dir->fsop->unlink(dir, i, basename);
		}
	} else {
		errno = -EPERM;
	}
	iput(i);
	iput(dir);
	free_name(tmp_name);
	return errno;
}
