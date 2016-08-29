#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

struct entry {
	char *path;
	struct stat stat;
};

static struct entry *entries;
static int entries_len;

struct entry* __nvmfs_entry_new(const char *path, st_mode mode)
{
	struct entry *entry = malloc(sizeof(struct entry));

	entry->stat.st_mode = mode;
	entry->stat.st_nlink = 1;
	entry->stat.st_size = 0;

	return entry;
}

struct entry* __nvmfs_entry_find(const char *path)
{
	int i;
	for (i=0; i<entries_len; i++) {
		if (0==strcmp(entries[i].path, path))
			return entries[i];
	}
	return NULL;
}

void _add_path(const char *path)
{
	paths[++paths_n] = path;
}

static int nvmfs_create(const char *path, mode_t mode,
			struct fuse_file_info *fi)
{
	int res = 0;

	return res;
}

static int nvmfs_getattr(const char *path, struct stat *stbuf)
{
	memset(stbuf, 0, sizeof(struct stat));

	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		return 0;
	}
	
	if (!_find_path(path)) {
		return -ENOENT;
	}

	stbuf->st_mode = S_IFREG | 0444;
	stbuf->st_nlink = 1;
	stbuf->st_size = strlen(hello_str);

	return 0;
}

static int nvmfs_truncate(const char *path, off_t size)
{
	(void) size;

	if(strcmp(path, "/") != 0)
		return -ENOENT;

	return 0;
}

static int nvmfs_open(const char *path, struct fuse_file_info *fi)
{
	if (!_find_path(path))
		return -ENOENT;

	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	return 0;
}

static int nvmfs_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	size_t len;
	(void) fi;
	if(strcmp(path, hello_path) != 0)
		return -ENOENT;

	len = strlen(hello_str);
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, hello_str + offset, size);
	} else
		size = 0;

	return size;
}

static int nvmfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;

	if (strcmp(path, "/") != 0)
		return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	filler(buf, hello_path + 1, NULL, 0);

	return 0;
}

static int nvmfs_write(const char *path, const char *buf, size_t size,
		      off_t offset, struct fuse_file_info *fi)
{
	(void) buf;
	(void) offset;
	(void) fi;

	if(strcmp(path, "/") != 0)
		return -ENOENT;

	return size;
}

static struct fuse_operations nvmfs_oper = {
	.getattr	= nvmfs_getattr,
	.truncate	= nvmfs_truncate,
	.open		= nvmfs_open,
	.readdir	= nvmfs_readdir,
	.read		= nvmfs_read,
	.write		= nvmfs_write,
	.create		= nvmfs_create,
};

int main(int argc, char *argv[])
{
	paths_len = 10;
	paths = malloc(sizeof(*paths) * paths_len);
	return fuse_main(argc, argv, &nvmfs_oper, NULL);
}
