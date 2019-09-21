#define FUSE_USE_VERSION 30
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif
char * rootdir;
size_t file1_length;
size_t file2_length;

int twofs_getattr(const char *path, struct stat *statbuf) {
  if (strcmp(path, "/") == 0) {
    statbuf->st_mode = 0755 | S_IFDIR;
    statbuf->st_nlink = 2;
    return 0;
  }  
  if(strcmp(path + 1, "file1") == 0) {
    statbuf->st_mode = S_IFREG | 0666;
    statbuf->st_nlink = 1;
    statbuf->st_ctime = 0;
    statbuf->st_blocks = 8;
    statbuf->st_size = file1_length;
    return 0;
  }
  if(strcmp(path + 1, "file2") == 0) {
    statbuf->st_mode = S_IFREG | 0666;
    statbuf->st_nlink = 1;
    statbuf->st_ctime = 0;
    statbuf->st_blocks = 8;
    statbuf->st_size = file2_length;
    return 0;
  }
  // -ENOENT: no suce file or directory
  return -ENOENT;
}
  
int twofs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
  if (strcmp(path, "/") == 0) {
    filler(buf, "file1", NULL, 0);
    filler(buf, "file2", NULL, 0);
  }
  return 0;
}


int twofs_open(const char *path, struct fuse_file_info *fi) {
  int fd;
  if (strcmp(path + 1, "file1") == 0) { 
    fd = open(rootdir, fi->flags);  
    if (fd == -1) {
      printf("Error Number % d\n", errno);
    }
    fi->fh = fd;
    return 0;
  }
  else if (strcmp(path + 1, "file2") == 0) {
    fd = open(rootdir, fi->flags);
    if (fd == -1) {
      printf("Error Number % d\n", errno);
    }
    fi->fh = fd;
    return 0;
  }
  return -ENOENT;
}


int twofs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  
  if (strcmp(path + 1, "file2") == 0) {
    offset += file1_length;

  }
  
  int res = pread(fi->fh, buf, size, offset);
  if (res == -1) {
    printf("Error Number % d\n", errno);
  }
  return res;
}

int twofs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  char * new_buffer = buf;
  int original_size = size;
  if (strcmp(path + 1, "file1") == 0) {    
    if (size > file1_length) {
	size = file1_length;
	memcpy(new_buffer, buf, size);
    }
  }

  if (strcmp(path + 1, "file2") == 0) {
    offset += file1_length;
    if (size > file2_length) {
      size = file2_length;
      memcpy(new_buffer, buf, size);
    }
  }
  
  int res = pwrite(fi->fh, new_buffer, size, offset);
  if (res == -1) {
    printf("Error Number % d\n", errno);
  }
  return original_size;
}

int twofs_access(const char *path, int mask) {
  int res;  
  res = access(path, mask);
  if (res == -1) {
    return -errno;
  }
  return 0;
}

int twofs_getxattr(const char *path, const char *name, char *value, size_t size) {
  if (strcmp(path + 1, "file1") == 0) {
    return 0;
  }
  if (strcmp(path + 1, "file2") == 0) {
    return 0;
  }
  return -errno;
  //return getxattr(path, name, value, size);
}


int twofs_truncate(const char *path, off_t size) {
  int res;
  if (strcmp(path + 1, "file1") == 0) {
    return 0;
  }
  if (strcmp(path + 1, "file2") == 0) {
    return 0;
  }
  else {
    res = truncate(path, size);
  }
  if (res == -1) {
    return -errno;
  } 
  return 0;
}

static struct fuse_operations operations = {
  .getattr = twofs_getattr,
  .readdir = twofs_readdir,
  .open = twofs_open,
  .read	= twofs_read,
  .write = twofs_write,
  .access = twofs_access,
  .getxattr = twofs_getxattr,
  .truncate = twofs_truncate,  
};

int main(int argc, char *argv[] ) {
  
  rootdir = realpath(argv[argc-2], NULL);
  argv[argc-2] = argv[argc-1];
  argv[argc-1] = NULL;
  argc--;
  printf("rootdir = %s\n", rootdir);

  struct stat buf;
  memset (&buf, 0, sizeof(buf));
  int res = lstat(rootdir, &buf);
  if (res == -1) {
    perror("stat error\n");
    exit(EXIT_FAILURE);
  }
  
  off_t total = buf.st_size;
  printf("total size = %ld\n", total);  
  file1_length = total / 2;
  file2_length = total - total / 2;
  int fuse_stat = fuse_main(argc, argv, &operations, rootdir);
  return fuse_stat;
}
