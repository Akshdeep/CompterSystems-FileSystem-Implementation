/*
 * file:        homework.c
 * description: Programming assignment 4 for CS 5600 file system
 *
 * Akshdeep Rungta, Hongxiang Wang
 */

#define FUSE_USE_VERSION 27

#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <fuse.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <math.h>
#include <libgen.h>
#include "fsx600.h"
#include "blkdev.h"


extern int homework_part;       /* set by '-part n' command-line option */

/* 
 * disk access - the global variable 'disk' points to a blkdev
 * structure which has been initialized to access the image file.
 *
 * NOTE - blkdev access is in terms of 1024-byte blocks
 */
extern struct blkdev *disk;

/* by defining bitmaps as 'fd_set' pointers, you can use existing
 * macros to handle them. 
 *   FD_ISSET(##, inode_map);
 *   FD_CLR(##, block_map);
 *   FD_SET(##, block_map);
 */
fd_set *inode_map;              /* = malloc(sb.inode_map_size * FS_BLOCK_SIZE); */
fd_set *block_map;

fd_set *inode_map;
int     inode_map_base;

struct fs_inode *inodes;
int   n_inodes;
int   inode_base;

fd_set *block_map;
int     block_map_base;

int   n_blocks;
int   root_inode;

struct fs_super sb;

#define INDIRECT_INODES 256

/* init - this is called once by the FUSE framework at startup. Ignore
 * the 'conn' argument.
 * recommended actions:
 *   - read superblock
 *   - allocate memory, read bitmaps and inodes
 */
void* fs_init(struct fuse_conn_info *conn)
{
    if (disk->ops->read(disk, 0, 1, &sb) < 0)
        exit(1);

    /* The inode map and block map are written directly to the disk after the superblock */

    inode_map_base = 1;
    inode_map = malloc(sb.inode_map_sz * FS_BLOCK_SIZE);
    if (disk->ops->read(disk, inode_map_base, sb.inode_map_sz, inode_map) < 0)
        exit(1);

    block_map_base = inode_map_base + sb.inode_map_sz;
    block_map = malloc(sb.block_map_sz * FS_BLOCK_SIZE);
    if (disk->ops->read(disk, block_map_base, sb.block_map_sz, block_map) < 0)
        exit(1);

    /* The inode data is written to the next set of blocks */

    inode_base = block_map_base + sb.block_map_sz;
    n_inodes = sb.inode_region_sz * INODES_PER_BLK;
    inodes = malloc(sb.inode_region_sz * FS_BLOCK_SIZE);
    if (disk->ops->read(disk, inode_base, sb.inode_region_sz, inodes) < 0)
        exit(1);

    return NULL;
}

void write_metadata(){
    if (disk->ops->write(disk, inode_map_base, sb.inode_map_sz, inode_map) < 0)
        exit(1);
    if (disk->ops->write(disk, block_map_base, sb.block_map_sz, block_map) < 0)
        exit(1);
    if (disk->ops->write(disk, inode_base, sb.inode_region_sz, inodes) < 0)
        exit(1);
}


/* Note on path translation errors:
 * In addition to the method-specific errors listed below, almost
 * every method can return one of the following errors if it fails to
 * locate a file or directory corresponding to a specified path.
 *
 * ENOENT - a component of the path is not present.
 * ENOTDIR - an intermediate component of the path (e.g. 'b' in
 *           /a/b/c) is not a directory
 */
int lookup(int inode, char * name) {
    //printf("lookup\n");
    struct fs_inode directory = inodes[inode];
    int directory_block = directory.direct[0];
    int num_entries = 32;
    struct fs_dirent entries[num_entries];
    if (disk->ops->read(disk, directory_block, 1, &entries) < 0)
        exit(1);
    for(int i = 0; i < num_entries; i++) {
        if (entries[i].valid == 1 && (strcmp(entries[i].name, name) == 0)) {
            //printf("inode %d\n", entries[i].inode);
            return entries[i].inode;
        }
    }
   return -ENOENT;
}

int translate(const char * path, int last_flag){
    //printf("translate\n");
    char *_path = strdup(path);
    int inode = 1;

    if (last_flag) {
        _path = dirname(_path);
    }

    if (strcmp(path, "/") == 0) {
        return inode;
    }

    char* token = strtok(_path, "/"); 
    //printf("while\n");
    while(token != NULL) {
        //printf("token %s\n", token);
        
        if(S_ISREG(inodes[inode].mode)) {
            return -ENOTDIR;
        }
        inode = lookup(inode, token);
        token = strtok(NULL, "/");
    }

    free(_path);

    return inode;
}
/* note on splitting the 'path' variable:
 * the value passed in by the FUSE framework is declared as 'const',
 * which means you can't modify it. The standard mechanisms for
 * splitting strings in C (strtok, strsep) modify the string in place,
 * so you have to copy the string and then free the copy when you're
 * done. One way of doing this:
 *
 *    char *_path = strdup(path);
 *    int inum = translate(_path);
 *    free(_path);
 */



/* getattr - get file or directory attributes. For a description of
 *  the fields in 'struct stat', see 'man lstat'.
 *
 * Note - fields not provided in fsx600 are:
 *    st_nlink - always set to 1
 *    st_atime, st_ctime - set to same value as st_mtime
 *
 * errors - path translation, ENOENT
 */
static void fill(int inode, struct stat *sb)
{
    struct fs_inode attr = inodes[inode];
    memset(sb,0,sizeof(sb));
    sb->st_uid = attr.uid;
    sb->st_gid = attr.gid;
    sb->st_mode = attr.mode;
    sb->st_mtime = attr.mtime;
    sb->st_atime = attr.mtime;
    sb->st_ctime = attr.ctime;
    sb->st_size = attr.size;
    sb->st_ino = inode;
    sb->st_nlink =1;
    if((attr.size % FS_BLOCK_SIZE) > 0)
        sb->st_blocks = (attr.size/FS_BLOCK_SIZE) + 1;
    else
        sb->st_blocks = (attr.size/FS_BLOCK_SIZE);
    sb->st_blksize = FS_BLOCK_SIZE;
    return;

}
static int fs_getattr(const char *path, struct stat *sb)
{
    int inode = translate(path, 0);
    if (inode < 0) {
        return inode;
    }
    fill(inode, sb);

    return 0;
}

/* readdir - get directory contents.
 *
 * for each entry in the directory, invoke the 'filler' function,
 * which is passed as a function pointer, as follows:
 *     filler(buf, <name>, <statbuf>, 0)
 * where <statbuf> is a struct stat, just like in getattr.
 *
 * Errors - path resolution, ENOTDIR, ENOENT
 */
static int fs_readdir(const char *path, void *ptr, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
    struct stat sb;
    int inode = translate(path, 0);
    if (inode < 0) {
        return inode;
    }

    struct fs_inode directory = inodes[inode];

    if (!S_ISDIR(directory.mode))
        return -ENOTDIR;

    int directory_block = directory.direct[0];
    int num_entries = 32;
    struct fs_dirent entries[num_entries];
    if (disk->ops->read(disk, directory_block, 1, &entries) < 0)
        exit(1);

    for(int i = 0; i < num_entries; i++) {
        if(entries[i].valid == 0) {
            continue;
        }
        fill(entries[i].inode, &sb);
        filler(ptr, entries[i].name, &sb, 0);
    }
    return 0;
}

/* see description of Part 2. In particular, you can save information 
 * in fi->fh. If you allocate memory, free it in fs_releasedir.
 */
static int fs_opendir(const char *path, struct fuse_file_info *fi)
{
    return 0;
}

static int fs_releasedir(const char *path, struct fuse_file_info *fi)
{
    return 0;
}

int allocate_inode(){
    for (int i = 0; i < sb.inode_map_sz * FS_BLOCK_SIZE; ++i)
    {
        if(!FD_ISSET(i, inode_map)) {
            FD_SET(i, inode_map);
            return i;
        }
    }

    return -ENOSPC;
}

int allocate_block(){
    for (int i = 0; i < sb.block_map_sz * FS_BLOCK_SIZE; ++i)
    {
        if(!FD_ISSET(i, block_map)) {
            FD_SET(i, block_map);
            return i;
        }
    }

    return -ENOSPC;
}

/* mknod - create a new file with permissions (mode & 01777)
 *
 * Errors - path resolution, EEXIST
 *          in particular, for mknod("/a/b/c") to succeed,
 *          "/a/b" must exist, and "/a/b/c" must not.
 *
 * If a file or directory of this name already exists, return -EEXIST.
 * If this would result in >32 entries in a directory, return -ENOSPC
 * if !S_ISREG(mode) return -EINVAL [i.e. 'mode' specifies a device special
 * file or other non-file object]
 */
static int fs_mknod(const char *path, mode_t mode, dev_t dev)
{
    char *_path = strdup(path);
    if (!S_ISREG(mode))
        return -EINVAL;

    int inode_dir = translate(path, 1);
    //printf("inode_dir%d\n", inode_dir);
    if (inode_dir < 0) {
        return inode_dir;
    }

    int inode_file = translate(path, 0);
    if (inode_file > 0) {
        return -EEXIST;
    }

    struct fs_inode directory = inodes[inode_dir];

    if (!S_ISDIR(directory.mode))
        return -ENOTDIR;

    int directory_block = directory.direct[0];
    int num_entries = 32;
    struct fs_dirent entries[num_entries];
    if (disk->ops->read(disk, directory_block, 1, &entries) < 0)
        exit(1);

    for(int i = 0; i < num_entries; i++) {
        if(entries[i].valid == 0) {
            int inode_allocated = allocate_inode();
            if (inode_allocated < 0)
                return inode_allocated;
            entries[i].inode = inode_allocated;
            entries[i].isDir = 0;
            entries[i].valid = 1;
            strcpy(entries[i].name, basename(_path));
            inodes[entries[i].inode].uid = getuid();
            inodes[entries[i].inode].gid = getgid();
            inodes[entries[i].inode].size = 0;
            inodes[entries[i].inode].mode = mode | S_IFREG;
            inodes[entries[i].inode].ctime = time(NULL);
            inodes[entries[i].inode].mtime = inodes[entries[i].inode].ctime;

            if (disk->ops->write(disk, directory_block, 1, &entries) < 0)
                exit(1);
            write_metadata();
            return 0;
        }
    }


    return -ENOSPC;
}

/* mkdir - create a directory with the given mode.
 * Errors - path resolution, EEXIST
 * Conditions for EEXIST are the same as for create. 
 * If this would result in >32 entries in a directory, return -ENOSPC
 *
 * Note that you may want to combine the logic of fs_mknod and
 * fs_mkdir. 
 */ 
static int fs_mkdir(const char *path, mode_t mode)
{
    char *_path = strdup(path);
    
    int inode_dir = translate(path, 1);
    //printf("Path%s\n", _path);
    //printf("inode_dir%d\n", inode_dir);
    if (inode_dir < 0) {
        return inode_dir;
    }

    int inode_file = translate(path, 0);
    if (inode_file > 0) {
        return -EEXIST;
    }

    struct fs_inode directory = inodes[inode_dir];

    if (!S_ISDIR(directory.mode))
        return -ENOTDIR;

    int directory_block = directory.direct[0];
    int num_entries = 32;
    struct fs_dirent entries[num_entries];
    if (disk->ops->read(disk, directory_block, 1, &entries) < 0)
        exit(1);

    for(int i = 0; i < num_entries; i++) {
        if(entries[i].valid == 0) {  
            int inode_allocated = allocate_inode();
            int block_allocated = allocate_block();
            //printf("inode_allocated %d block_allocated %d\n", inode_allocated, block_allocated);
            if ((inode_allocated < 0) || (block_allocated < 0)) {
                return inode_allocated;
            }          
            entries[i].inode = inode_allocated;
            entries[i].isDir = 1;
            strcpy(entries[i].name, basename(_path));
            inodes[entries[i].inode].uid = getuid();
            inodes[entries[i].inode].gid = getgid();
            inodes[entries[i].inode].size = 0;
            inodes[entries[i].inode].mode = mode | S_IFDIR;
            inodes[entries[i].inode].ctime = time(NULL);
            inodes[entries[i].inode].mtime = inodes[entries[i].inode].ctime;
            inodes[entries[i].inode].direct[0] = block_allocated;
            entries[i].valid = 1;
            if (disk->ops->write(disk, directory_block, 1, &entries) < 0)
                exit(1);
            write_metadata();
            return 0;
        }
    }


    return -ENOSPC;
}

int fs_utime(const char *path, struct utimbuf *ut)
{
    char *_path = strdup(path);
    int inode = translate(path, 0);
    if (inode < 0)
        return inode;

    struct fs_inode file = inodes[inode];
    file.mtime =  ut->modtime;
    inodes[inode] = file;
    write_metadata();
    return 0;
}

int get_block_number(struct fs_inode * file, int block_number, int allocate) {
    int new_block_num;
    if (allocate){
        new_block_num = allocate_block();
        if (new_block_num < 0)
            return -ENOSPC;
    }
    if (block_number < N_DIRECT) {
        if (allocate)
            file->direct[block_number] = new_block_num;
        return file->direct[block_number];
    } 

    block_number -= N_DIRECT;

    if (file->indir_1 == 0) {
        int new_block_indir_1 = allocate_block();
        if (new_block_indir_1 < 0)
            return -ENOSPC;
        file->indir_1 = new_block_indir_1;
    }        

    if (block_number < INDIRECT_INODES) {
        int indirect_1[INDIRECT_INODES] = {0};
        if (disk->ops->read(disk, file->indir_1, 1, indirect_1) < 0) {
            exit(1);
        }

        if (allocate)
        {
            indirect_1[block_number] = new_block_num;
            if (disk->ops->write(disk, file->indir_1, 1, indirect_1) < 0) 
                exit(1);
        }
        return indirect_1[block_number];
    } 

    block_number-=INDIRECT_INODES;

    if (file->indir_2 == 0) {
        int new_block_indir_2 = allocate_block();
        if (new_block_indir_2 < 0)
            return -ENOSPC;
        file->indir_2 = new_block_indir_2;
    }   

    int indirect_2[INDIRECT_INODES] = {0};
    if (disk->ops->read(disk, file->indir_2, 1, indirect_2) < 0) {
        exit(1);
    }
    
    int indirect_2_level2[INDIRECT_INODES] = {0};
    if (indirect_2[block_number/INDIRECT_INODES] == 0)
    {
        int new_block_indir_2_level = allocate_block();
        if (new_block_indir_2_level < 0)
            return -ENOSPC;
        indirect_2[block_number/INDIRECT_INODES] = new_block_indir_2_level;
        if (disk->ops->write(disk, file->indir_2, 1, indirect_2) < 0) {
            exit(1);
        }
    }
    if (disk->ops->read(disk, indirect_2[block_number/INDIRECT_INODES], 1, indirect_2_level2) < 0) {
        exit(1);
    }

    if (allocate)
    {   
        indirect_2_level2[block_number%INDIRECT_INODES] = new_block_num;
        if (disk->ops->write(disk, indirect_2[block_number/INDIRECT_INODES], 1, indirect_2_level2) < 0) 
                exit(1);
    }
    return indirect_2_level2[block_number%INDIRECT_INODES];
}

/* truncate - truncate file to exactly 'len' bytes
 * Errors - path resolution, ENOENT, EISDIR, EINVAL
 *    return EINVAL if len > 0.
 */
static int fs_truncate(const char *path, off_t len)
{
    /* you can cheat by only implementing this for the case of len==0,
     * and an error otherwise.
     */
    if (len != 0)
	return -EINVAL;		/* invalid argument */

     int inode = translate(path, 0);
    if (inode < 0) {
        return inode;
    }

    struct fs_inode file = inodes[inode];

    int start_block = 0;
    int end_block = file.size /FS_BLOCK_SIZE;
    char * temp = malloc(FS_BLOCK_SIZE);
    memset(temp, 0, FS_BLOCK_SIZE);

    //printf("offset %jd len %zu start_block %d end_block %d\n", offset, len, start_block, end_block);

    for (int i = start_block; i <= end_block; ++i)
    {
        //printf("i %d Block %d\n", i, get_block_number(&file, i));
        int block_number = get_block_number(&file, i, 0);
        if (disk->ops->write(disk, block_number, 1, temp) < 0) {
            exit(1);
        }        
        FD_CLR(block_number, block_map);
    }
    file.size = 0;
    file.mtime = time(NULL);
    inodes[inode] = file;
    write_metadata();
    free(temp);

    return 0;
}

/* unlink - delete a file
 *  Errors - path resolution, ENOENT, EISDIR
 * Note that you have to delete (i.e. truncate) all the data.
 */
static int fs_unlink(const char *path)
{
    char *_path = strdup(path);
    int truncated = fs_truncate(path, 0);
    if (truncated < 0) 
        return truncated;
    int dir_inode = translate(path, 1);
    int file_inode = translate(path, 0);
    FD_CLR(file_inode, inode_map);

    struct fs_inode directory = inodes[dir_inode];

    if (!S_ISDIR(directory.mode))
        return -ENOTDIR;

    if (!S_ISREG(inodes[file_inode].mode))
        return -EISDIR;

    int directory_block = directory.direct[0];
    int num_entries = 32;
    struct fs_dirent entries[num_entries];
    if (disk->ops->read(disk, directory_block, 1, &entries) < 0)
        exit(1);

    for(int i=0; i<num_entries;i++)
    {
        if(entries[i].valid == 1 &&(strcmp(entries[i].name,basename(_path)) == 0)){

            entries[i].valid = 0;
            if (disk->ops->write(disk, directory_block, 1, &entries) < 0)
                exit(1);
            write_metadata();
            return 0;

        }
    }
    return -ENOTDIR;

}

/* rmdir - remove a directory
 *  Errors - path resolution, ENOENT, ENOTDIR, ENOTEMPTY
 */
static int fs_rmdir(const char *path)
{
    char *_path = strdup(path);
    int dir_inode = translate(_path, 1);
    if (dir_inode <0)
        return dir_inode;
    int dir_inode_rm = translate(_path, 0);
    if (dir_inode_rm<0)
        return dir_inode_rm;

    struct fs_inode directory = inodes[dir_inode_rm];

    if (!S_ISDIR(directory.mode))
        return -ENOTDIR;

    int directory_block = directory.direct[0];
    int num_entries = 32;
    struct fs_dirent entries[num_entries];
    if (disk->ops->read(disk, directory_block, 1, &entries) < 0)
        exit(1);

    for(int i =0; i<num_entries;i++)
    {
        if (entries[i].valid == 1)
            return -ENOTEMPTY;
    }

     FD_CLR(directory_block, block_map);
     FD_CLR(dir_inode_rm,inode_map);
     directory = inodes[dir_inode];

    if (!S_ISDIR(directory.mode))
        return -ENOTDIR;

    directory_block = directory.direct[0];
    if (disk->ops->read(disk, directory_block, 1, &entries) < 0)
        exit(1);

    for(int i=0; i<num_entries;i++)
    {
        if (entries[i].valid == 1 && (strcmp(entries[i].name, basename(_path)) == 0))
        {
            entries[i].valid = 0;
            if (disk->ops->write(disk, directory_block, 1, &entries) < 0)
                exit(1);
            write_metadata();
            return 0;
        }
    }

    return -ENOENT;
}

/* rename - rename a file or directory
 * Errors - path resolution, ENOENT, EINVAL, EEXIST
 *
 * ENOENT - source does not exist
 * EEXIST - destination already exists
 * EINVAL - source and destination are not in the same directory
 *
 * Note that this is a simplified version of the UNIX rename
 * functionality - see 'man 2 rename' for full semantics. In
 * particular, the full version can move across directories, replace a
 * destination file, and replace an empty directory with a full one.
 */
static int fs_rename(const char *src_path, const char *dst_path)
{
    char *_src_path = strdup(src_path);
    char *_dst_path = strdup(dst_path);
    int src_inode = translate(src_path, 0);
    if (src_inode < 0)
        return src_inode;
    int src_parant_inode = translate(src_path, 1);
    int dst_parant_inode = translate(dst_path, 1);
    if(src_parant_inode != dst_parant_inode)
        return -EINVAL;
    int dst_inode = translate(dst_path, 0);
    if (dst_inode > 0) 
        return -EEXIST;

    struct fs_inode directory = inodes[dst_parant_inode];

    if (!S_ISDIR(directory.mode))
        return -ENOTDIR;

    int directory_block = directory.direct[0];
    int num_entries = 32;
    struct fs_dirent entries[num_entries];
    if (disk->ops->read(disk, directory_block, 1, &entries) < 0)
        exit(1);

    for(int i=0; i<num_entries;i++)
        {
        if (entries[i].valid == 1 && (strcmp(entries[i].name, basename(_src_path)) == 0)) {
        strcpy(entries[i].name, basename(_dst_path));
        if (disk->ops->write(disk, directory_block, 1, &entries) < 0)
                exit(1);
        inodes[dst_inode].mtime = time(NULL);
        write_metadata();
        return 0;
        }
    }
    return -ENOENT;
}

/* chmod - change file permissions
 * utime - change access and modification times
 *         (for definition of 'struct utimebuf', see 'man utime')
 *
 * Errors - path resolution, ENOENT.
 */
static int fs_chmod(const char *path, mode_t mode)
{
    char *_path = strdup(path);
    int inode = translate(path, 0);
    if (inode < 0)
        return inode;

    struct fs_inode file = inodes[inode];
    file.mode = mode;
    file.mtime = time(NULL);
    inodes[inode] = file;
    write_metadata();


    return 0;
}

/* read - read data from an open file.
 * should return exactly the number of bytes requested, except:
 *   - if offset >= file len, return 0
 *   - if offset+len > file len, return bytes from offset to EOF
 *   - on error, return <0
 * Errors - path resolution, ENOENT, EISDIR
 */
static int fs_read(const char *path, char *buf, size_t len, off_t offset,
		    struct fuse_file_info *fi)
{
    int inode = translate(path, 0);
    if (inode < 0) {
        return inode;
    }

    struct fs_inode file = inodes[inode];

    if (offset >= file.size)
         return 0;

    int start_block = offset/FS_BLOCK_SIZE;
    int end_block;
    if (offset + len > file.size){
        end_block = file.size /FS_BLOCK_SIZE;
        len =file.size - offset;
    }
    else
        end_block = (offset + len) / FS_BLOCK_SIZE;
    int local_offset = offset % FS_BLOCK_SIZE;
    //int end_offset = (offset + len) % FS_BLOCK_SIZE;

    int total_blocks = (end_block - start_block + 1);
    char * temp = malloc(total_blocks*FS_BLOCK_SIZE); 
    char * temp_start = temp;

    //printf("offset %jd len %zu start_block %d end_block %d\n", offset, len, start_block, end_block);

    for (int i = start_block; i <= end_block; ++i)
    {
        //printf("i %d Block %d\n", i, get_block_number(&file, i));
        if (disk->ops->read(disk, get_block_number(&file, i, 0), 1, temp) < 0) {
            exit(1);
        }
        temp+=FS_BLOCK_SIZE;
    }

    memcpy(buf,temp_start+local_offset,len);

    return len;
}

/* write - write data to a file
 * It should return exactly the number of bytes requested, except on
 * error.
 * Errors - path resolution, ENOENT, EISDIR
 *  return EINVAL if 'offset' is greater than current file length.
 *  (POSIX semantics support the creation of files with "holes" in them, 
 *   but we don't)
 */
static int fs_write(const char *path, const char *buf, size_t len,
		     off_t offset, struct fuse_file_info *fi)
{
    int inode = translate(path, 0);
    if (inode < 0) {
        return inode;
    }
     struct fs_inode file = inodes[inode];

    if (offset > file.size)
         return -EINVAL;

    char *write_buf = buf;
    int start_block = offset/FS_BLOCK_SIZE;
    int end_block = (offset + len) / FS_BLOCK_SIZE;
    int end_block_offset = (offset + len) % FS_BLOCK_SIZE;
    int local_offset = offset % FS_BLOCK_SIZE;
    int over_block = file.size / FS_BLOCK_SIZE;

    int total_blocks = (end_block - start_block + 1);
    int total_blocks_file = file.size / FS_BLOCK_SIZE;
    total_blocks_file += (file.size % FS_BLOCK_SIZE)?1 : 0;
    
    //printf("offset %jd len %zu start_block %d end_block %d\n", offset, len, start_block, end_block);

    for (int i = start_block; i <= end_block; ++i)
    {
        char temp [FS_BLOCK_SIZE] = {0}; 
        //printf("i %d Block %d\n", i, get_block_number(&file, i));
        if( i == start_block && local_offset)
        {
            int  first_block_num = get_block_number(&file, i, 0);
            if (disk->ops->read(disk, first_block_num, 1, temp) < 0) {
                exit(1);
            }
            for (int j = local_offset; j< FS_BLOCK_SIZE;j++)
                temp[j] = write_buf[j-local_offset];
            if (disk->ops->write(disk, first_block_num, 1, temp) < 0) {
                exit(1);
            } 

            write_buf+=FS_BLOCK_SIZE -local_offset;
        } 
        else if (i == end_block && end_block_offset && ((offset + len) < file.size)) {
            int  last_block_num = get_block_number(&file, i, 0);
            if (disk->ops->read(disk, last_block_num, 1, temp) < 0) {
                exit(1);
            }
            for (int j = 0; j< end_block_offset;j++)
                temp[j] = write_buf[j];
            if (disk->ops->write(disk, last_block_num, 1, temp) < 0) {
                exit(1);
            } 
        }

        else{
            if(i < total_blocks_file){
                if (disk->ops->write(disk,get_block_number(&file, i, 0), 1, write_buf) < 0) {
                exit(1);
                }
            }
            else
            {
                 if (disk->ops->write(disk,get_block_number(&file, i, 1), 1, write_buf) < 0) {
                exit(1);
                }
            }
            write_buf += FS_BLOCK_SIZE;
        }
    }

    if(offset + len > file.size)
        file.size = len + offset;
    inodes[inode] = file;
    file.mtime = time(NULL);
    write_metadata();
    return len;


}

static int fs_open(const char *path, struct fuse_file_info *fi)
{
    return 0;
}

static int fs_release(const char *path, struct fuse_file_info *fi)
{
    return 0;
}

/* statfs - get file system statistics
 * see 'man 2 statfs' for description of 'struct statvfs'.
 * Errors - none. 
 */
static int fs_statfs(const char *path, struct statvfs *st)
{
    /* needs to return the following fields (set others to zero):
     *   f_bsize = BLOCK_SIZE
     *   f_blocks = total image - metadata
     *   f_bfree = f_blocks - blocks used
     *   f_bavail = f_bfree
     *   f_namelen = <whatever your max namelength is>
     *
     * this should work fine, but you may want to add code to
     * calculate the correct values later.
     */
    st->f_bsize = FS_BLOCK_SIZE;
    int free_blocks = 0;
    for (int i = 0; i < sb.block_map_sz * FS_BLOCK_SIZE; ++i)
    {
        if(!FD_ISSET(i, block_map)) {
            free_blocks++;
        }
    }
    st->f_blocks = sb.num_blocks;           /* probably want to */
    st->f_bfree = free_blocks;            /* change these */
    st->f_bavail = free_blocks;           /* values */
    st->f_namemax = 27;

    return 0;
}

/* operations vector. Please don't rename it, as the skeleton code in
 * misc.c assumes it is named 'fs_ops'.
 */
struct fuse_operations fs_ops = {
    .init = fs_init,
    .getattr = fs_getattr,
    .opendir = fs_opendir,
    .readdir = fs_readdir,
    .releasedir = fs_releasedir,
    .mknod = fs_mknod,
    .mkdir = fs_mkdir,
    .unlink = fs_unlink,
    .rmdir = fs_rmdir,
    .rename = fs_rename,
    .chmod = fs_chmod,
    .utime = fs_utime,
    .truncate = fs_truncate,
    .open = fs_open,
    .read = fs_read,
    .write = fs_write,
    .release = fs_release,
    .statfs = fs_statfs,
};

