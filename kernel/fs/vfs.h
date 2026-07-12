#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stddef.h>

void fs_init(void);
int fs_ls(const char* path, char* buf, int buf_size);
int fs_cd(const char* path);
int fs_cat(const char* name, char* buf, int buf_size);
int fs_mkdir(const char* path);
int fs_rmdir(const char* path);
int fs_create(const char* path);
int fs_delete(const char* path);
int fs_rename(const char* oldpath, const char* newpath);
int fs_copy(const char* src, const char* dst);

#endif
