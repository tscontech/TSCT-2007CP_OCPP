#include <errno.h>
#include <sys/ioctl.h>
#include <sys/reent.h>
#include <sys/syslimits.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "nor/mmp_nor.h"
#include "itp_cfg.h"
#include "littlefs/lfs.h"
#include "littlefs/lfs_util.h"

static char *itp_lfs_realpath(const char *path, char* _resolved);
#define ITP_LFS_TAG         "littlefssfelttil"
#define OFFSETOF(container_type, member) ((size_t) &((container_type *)0)->member)
#define CONTAINEROF(member_ptr, container_type, member)		\
	 ((container_type *)						                \
	  ((char *)(member_ptr)						                \
	   - offsetof(container_type, member))

#define ITP_LFS_PREPAREBYFD() \
    lfs_file_t              *file   =itp_files[fd];                 \
    int                     drive   =itp_drives[fd];                \
    struct lfs_config       *config =itp_lfs_get_config('A'+drive); \
    struct itp_lfs_context  *context=itp_lfs_get_context('A'+drive);\
    lfs_t                   *lfs    =&context->lfs

#define ITP_LFS_PREPAREBYPATH(path) \
    struct lfs_config       *config =itp_lfs_get_config(*path);     \
    struct itp_lfs_context  *context=itp_lfs_get_context(*path);    \
    lfs_t                   *lfs    =&context->lfs;                 \
    int                     drive   =toupper(*path)-'A'

struct itp_lfs_partition {
    uint32_t    block_start;
    uint32_t    blocks;
    uint32_t    reserved[2];
};

struct itp_lfs_partition_table {
    char                        tag[16];
    struct itp_lfs_partition    partitions[ITP_MAX_PARTITION];
};

struct itp_lfs_context {
    lfs_t               lfs;
    struct lfs_config   config;
    uint32_t            block_start;
    uint32_t            partition;
    pthread_mutex_t     lock;
    ITPDriveStatus*     drive_status;
    char                cwd[PATH_MAX];
};

static struct itp_lfs_context* drive_to_context['Z'-'A'+1]={0};

static int itp_lfs_nor_page_size;
static int itp_lfs_nor_block_size;
static int itp_lfs_nor_blocks;
extern int itpTaskGetVolume(void);

int itp_lfs_init()
{
    itp_lfs_nor_page_size=NorGetAttitude(SPI_0, SPI_CSN_0, NOR_ATTITUDE_PAGE_SIZE);
    itp_lfs_nor_block_size=NorGetAttitude(SPI_0, SPI_CSN_0, NOR_ATTITUDE_ERASE_UNIT);
    itp_lfs_nor_blocks=NorGetAttitude(SPI_0, SPI_CSN_0, NOR_ATTITUDE_BLOCK_SIZE)*NorGetAttitude(SPI_0, SPI_CSN_0, NOR_ATTITUDE_SECTOR_PER_BLOCK);
    memset(drive_to_context, 0, sizeof(drive_to_context));
    return 0;
}

#define HEXDUMP_COLS 16
void hexdump(char* msg, void *mem, unsigned int len)
{
        unsigned int i, j;
//return;
    ithPrintf(msg);
        for(i = 0; i < len + ((len % HEXDUMP_COLS) ? (HEXDUMP_COLS - len % HEXDUMP_COLS) : 0); i++)
        {
                /* print offset */
                if(i % HEXDUMP_COLS == 0)
                {
                        printf("0x%06x: ", i);
                }
 
                /* print hex data */
                if(i < len)
                {
                        printf("%02x ", 0xFF & ((char*)mem)[i]);
                }
                else /* end of block, just aligning for ASCII dump */
                {
                        printf("   ");
                }
                
                /* print ASCII dump */
                if(i % HEXDUMP_COLS == (HEXDUMP_COLS - 1))
                {
                        for(j = i - (HEXDUMP_COLS - 1); j <= i; j++)
                        {
                                if(j >= len) /* end of block, not really printing */
                                {
                                        putchar(' ');
                                }
                                else if(isalnum(((char*)mem)[j])) /* printable char */
                                {
                                        putchar(0xFF & ((char*)mem)[j]);        
                                }
                                else /* other char */
                                {
                                        putchar('.');
                                }
                        }
                        putchar('\n');
                }
        }
    ithPrintf("\n");
}


// Read a region in a block. Negative error codes are propogated
// to the user.
int itp_lfs_bd_read(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size)
{
    int n;
    struct itp_lfs_context *context=(struct itp_lfs_context*)c->context;
//ithPrintf("[i] itp_lfs_bd_read: block %04X, off: %d, size: %d\n", block, off,size);
//ithPrintf("[i] itp_lfs_bd_read: block_start %d(%04X), block_size: %d(%04X\n", context->block_start, context->block_start, c->block_size, c->block_size);
    n = NorRead(SPI_0, SPI_CSN_0, (context->block_start+block)*c->block_size+off, (uint8_t*)buffer, size);
//ithPrintf("[i] NorRead(%04X)=%d\n", (context->block_start+block)*c->block_size+off, n);
//if (block<=1) {
//    hexdump("itp_lfs_bd_read\n", buffer, size);
//}
    return 0;//block;
}

// Program a region in a block. The block must have previously
// been erased. Negative error codes are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
int itp_lfs_bd_prog(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size)
{
    int n;
    struct itp_lfs_context *context=(struct itp_lfs_context*)c->context;

//ithPrintf("[i] itp_lfs_bd_prog: block %04X, off: %d, size: %d\n", block, off,size);
//ithPrintf("[i] itp_lfs_bd_prog: block_start %d(%04X), block_size: %d(%04X\n", context->block_start, context->block_start, c->block_size, c->block_size);
    n=NorWriteWithoutErase(SPI_0, SPI_CSN_0, (context->block_start+block)*c->block_size+off, (uint8_t*)buffer, size);
//ithPrintf("[i] NorWriteWithoutErase(%04X)=%d\n", (context->block_start+block)*c->block_size+off, n);
    return 0;//block;
}

// Erase a block. A block must be erased before being programmed.
// The state of an erased block is undefined. Negative error codes
// are propogated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
int itp_lfs_bd_erase(const struct lfs_config *c, lfs_block_t block)
{
    int n;
    struct itp_lfs_context *context=(struct itp_lfs_context*)c->context;
//ithPrintf("[i] itp_lfs_bd_erase: block %04X\n", block);
//ithPrintf("[i] itp_lfs_bd_erase: block_start %d(%04X), block_size: %d(%04X\n", context->block_start, context->block_start, c->block_size, c->block_size);
    n=NorErase(SPI_0, SPI_CSN_0, (context->block_start+block)*c->block_size, c->block_size);
//ithPrintf("[i] NorErase(%04X)=%d\n", (context->block_start+block)*c->block_size, n);

    return 0;//block;
}

// Sync the state of the underlying block device. Negative error codes
// are propogated to the user.
int itp_lfs_bd_sync(const struct lfs_config *c)
{
    struct itp_lfs_context *context=(struct itp_lfs_context*)c->context;
//ithPrintf("[i] itp_lfs_bd_sync\n");
    return 0;
}

static struct itp_lfs_context* itp_lfs_get_context(int driveLetter)
{
    int drive = toupper(driveLetter)-'A';

    return drive_to_context[drive];
}

static struct lfs_config* itp_lfs_get_config(int driveLetter)
{
    struct itp_lfs_context *context=itp_lfs_get_context(driveLetter);

    return context?&context->config:NULL;
}

static int itp_lfs_lock(const struct lfs_config *c)
{
    struct itp_lfs_context* context=(struct itp_lfs_context*)c->context;
    return pthread_mutex_lock(&context->lock);
}

static int itp_lfs_unlock(const struct lfs_config *c)
{
    struct itp_lfs_context* context=(struct itp_lfs_context*)c->context;
    return pthread_mutex_unlock(&context->lock);
}

static int itp_lfs_get_norconfig(struct lfs_config *config, uint32_t blocks)
{
    config->read  = itp_lfs_bd_read;
    config->prog  = itp_lfs_bd_prog;
    config->erase = itp_lfs_bd_erase;
    config->sync  = itp_lfs_bd_sync;

    // todo: init values according to the NOR flash used
    config->read_size=itp_lfs_nor_page_size;
    config->prog_size=itp_lfs_nor_page_size;
    config->block_size=itp_lfs_nor_block_size;
    config->block_count=blocks;
    config->cache_size=itp_lfs_nor_block_size;
    config->lookahead_size=itp_lfs_nor_block_size/8;
    config->block_cycles=-1;
    config->lock=itp_lfs_lock;
    config->unlock=itp_lfs_unlock;
}

static struct itp_lfs_context* itp_lfs_new_norcontext(uint32_t block_start, uint32_t blocks, ITPDriveStatus*  drive_status)
{
    struct lfs_config *config;
    struct itp_lfs_context* context;

    context=(struct itp_lfs_context*)calloc(1, sizeof(struct itp_lfs_context));
    if (!context) {
        return NULL;
    }
    pthread_mutex_init(&context->lock, NULL);

    context->block_start=block_start;
    context->drive_status=drive_status;
    context->cwd[0]='/';
    context->cwd[1]='\0';
    context->config.context=(void*)context;
    itp_lfs_get_norconfig(&context->config, blocks);

    return context;
}

static int itp_lfs_free_context(struct itp_lfs_context** context)
{
    if (context && *context) {
        pthread_mutex_destroy(&(*context)->lock);
        free(*context);
        *context=NULL;
    }
    return 0;
}

static int itp_lfs_read_partition_table(ITPDisk disk, struct itp_lfs_partition_table* pt)
{
    int i;

    if (disk!=ITP_DISK_NOR) {
        return -1;
    }
    memset(pt, 0, sizeof(*pt));
    NorRead(SPI_0, SPI_CSN_0, CFG_LFS_NOR_PTADDRESS, (uint8_t*)pt, sizeof(*pt));

    hexdump("littlefs partition table (rd):\n", (unsigned char*)pt, sizeof(*pt));

    // check if valid
    if (memcmp(pt->tag, ITP_LFS_TAG, strlen(ITP_LFS_TAG))) {
        LOG_ERR "not a valid littlefs partition table\n" LOG_END
        return -1;
    }

    //ithPrintf("[i] itp_lfs_read_partition_table\n");
    for (i=0; i<sizeof(pt->partitions)/sizeof(pt->partitions[0]); i++) {
        if ((pt->partitions[i].block_start==-1)||(pt->partitions[i].blocks==-1)) {
            break;
        }
        pt->partitions[i].block_start/=itp_lfs_nor_block_size;
        pt->partitions[i].blocks/=itp_lfs_nor_block_size;
    }

    return 0;
}

static int itp_lfs_write_partition_table(ITPDisk disk, struct itp_lfs_partition_table* pt)
{
    struct itp_lfs_partition_table ptable;
    int i;

    if (disk!=ITP_DISK_NOR) {
        return -1;
    }

    memcpy(ptable.tag, ITP_LFS_TAG, strlen(ITP_LFS_TAG));
    for (i=0; i<sizeof(ptable.partitions)/sizeof(ptable.partitions[0]); i++) {
        ptable.partitions[i].block_start=pt->partitions[i].block_start*itp_lfs_nor_block_size;
        ptable.partitions[i].blocks     =pt->partitions[i].blocks*itp_lfs_nor_block_size;
    }
    hexdump("littlefs partition table (wr):\n", (unsigned char*)&ptable, sizeof(ptable));
    NorWrite(SPI_0, SPI_CSN_0, CFG_LFS_NOR_PTADDRESS, (uint8_t*)&ptable, sizeof(ptable));

    return 0;
}

static int itp_lfs_mount(ITPDisk disk)
{
    struct itp_lfs_partition_table pt;
    struct itp_lfs_context *context;
    int lfs_count = 0;
    int i, j;
    ITPDriveStatus* drive_status = NULL;
    ITPDriveStatus* driveStatusTable;

    switch (disk)
    {
    case ITP_DISK_NOR:
        break;
    case ITP_DISK_NAND:
    default:
        LOG_INFO "unsupport disk: %d\n", disk LOG_END
        return -1;
    }

    if (itp_lfs_read_partition_table(disk, &pt)<0) {
        return -1;
    }

    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);

    // mount lfs partitions
    for (i=j=0; i<sizeof(pt.partitions)/sizeof(pt.partitions[0]); i++) {
        if ((pt.partitions[i].block_start==0) || (pt.partitions[i].blocks==0)) {
            // no more partitions
            break;
        }

        // find an empty drive
        for (; j < ITP_MAX_DRIVE; j++)
        {
            drive_status = driveStatusTable+j;
            
            if (!drive_status->avail)
                break;
        }

        drive_status->name[0] = 'A' + j;

        // todo: get actual NOR flash params
        context=itp_lfs_new_norcontext(pt.partitions[i].block_start, pt.partitions[i].blocks, drive_status);
        if (lfs_mount(&context->lfs, &context->config)>=0) {
ithPrintf("[i] mount lfs %02d %c: off: %08X len: %08X\n", i, drive_status->name[0], pt.partitions[i].block_start*itp_lfs_nor_block_size, pt.partitions[i].blocks*itp_lfs_nor_block_size);
            drive_to_context[j]      = context;

            drive_status->disk       = disk;
            drive_status->device     = ITP_DEVICE_LFS;
            drive_status->avail      = true;
            drive_status->name[1]    = ':';
            drive_status->name[2]    = '/';
            write(ITP_DEVICE_DRIVE, drive_status, sizeof (ITPDriveStatus));
            context->partition=lfs_count++;
        }
        else {
ithPrintf("[X] mount lfs %02d %c: off: %08X len: %08X\n", i, drive_status->name[0], pt.partitions[i].block_start*itp_lfs_nor_block_size, pt.partitions[i].blocks*itp_lfs_nor_block_size);
            itp_lfs_free_context(&context);
        }
    }

    return lfs_count?0:-1;
}

static int itp_lfs_umount(ITPDisk disk)
{
    struct itp_lfs_context* context;
    int i;

    for (i=0; i<sizeof(drive_to_context)/sizeof(drive_to_context[0]); i++) {
        context=drive_to_context[i];
        if (context && (context->drive_status->disk==disk)) {
            lfs_unmount(&context->lfs);
            drive_to_context[i]=NULL;
            context->drive_status->avail=false;
            write(ITP_DEVICE_DRIVE, context->drive_status, sizeof (ITPDriveStatus));
            itp_lfs_free_context(&context);
        }
    }
    return 0;
}

static int itp_lfs_create_partitions(ITPPartition* par)
{
    struct itp_lfs_context* context;
    struct itp_lfs_partition_table pt;
    int i;

    switch (par->disk)
    {
    case ITP_DISK_NOR:
        break;
    case ITP_DISK_NAND:
    default:
        LOG_INFO "unsupport disk: %d\n", par->disk LOG_END
        return -1;
    }

    memset(&pt, 0, sizeof(pt));
    
    for (i = 0; i < par->count; i++)
    {
        pt.partitions[i].block_start=(uint32_t)(par->start[i]/itp_lfs_nor_block_size);
        pt.partitions[i].blocks=(uint32_t)(par->size[i]/itp_lfs_nor_block_size);
        if (pt.partitions[i].block_start && !pt.partitions[i].blocks) {
            pt.partitions[i].blocks=itp_lfs_nor_blocks-pt.partitions[i].block_start;
            break;
        }
        //context=itp_lfs_new_norcontext(pt.partitions[i].block_start, pt.partitions[i].blocks, NULL);
        //lfs_format(&context->lfs, &context->config);
        //itp_lfs_free_context(&context);
    }

    // TODO: write partition table
    itp_lfs_write_partition_table(par->disk, &pt);

    return 0;
}

static int itp_lfs_get_partitions(ITPPartition* par)
{
    struct itp_lfs_context* context;
    struct itp_lfs_partition_table pt;
    int i;

    switch (par->disk)
    {
    case ITP_DISK_NOR:
        break;
    case ITP_DISK_NAND:
    default:
        LOG_INFO "unsupport disk: %d\n", par->disk LOG_END
        return -1;
    }

    //TODO: read partition table
    if (itp_lfs_read_partition_table(par->disk, &pt)<0) {
        return -1;
    }

    par->count=0;
    memset(par->size, 0, sizeof(par->size));
    memset(par->start, 0, sizeof(par->start));

    for (i=0; pt.partitions[i].block_start || pt.partitions[i].blocks; i++) {
        par->start[i]=pt.partitions[i].block_start*itp_lfs_nor_block_size;
        par->size[i]=pt.partitions[i].blocks*itp_lfs_nor_block_size;
    }
    par->count=i;

    return 0;
}

static int itp_lfs_format_partition(ITPDisk disk, int partition)
{
   struct itp_lfs_context* context;
   struct itp_lfs_partition_table pt;
   int ret;

ithPrintf("[i] %s(%d,%d)\n", __FUNCTION__, disk, partition);
    if (disk!=ITP_DISK_NOR) {
        return -1;
    }

    if (partition>=sizeof(pt.partitions)/sizeof(pt.partitions[0])) {
        return -1;
    }

    //TODO: read partition table
    if (itp_lfs_read_partition_table(disk, &pt)<0) {
        return -1;
    }

    if (!pt.partitions[partition].block_start || !pt.partitions[partition].blocks) {
        return -1;
    }

    context=itp_lfs_new_norcontext(pt.partitions[partition].block_start, pt.partitions[partition].blocks, NULL);
    ret=lfs_format(&context->lfs, &context->config);
    itp_lfs_free_context(&context);

    return ret;
}

static lfs_file_t* itp_files[OPEN_MAX]={0};
static lfs_t* itp_lfses[OPEN_MAX];
static int   itp_drives[OPEN_MAX];
static int itp_lfs_open(const char* name, int flags, int mode, void* info)
{
    ITP_LFS_PREPAREBYPATH(name);
    lfs_file_t *file;
    int lfs_flags = 0;
    int i, err;
//ithPrintf("[i] %s: %s, 0x%04X\n", __FUNCTION__, name, flags);
    ithEnterCritical();
    for (i=0; i<OPEN_MAX; i++) {
        if (!itp_files[i]) {
            itp_files[i]=file=calloc(1, sizeof(lfs_file_t));
            itp_lfses[i]=lfs;
            itp_drives[i]=drive;
            break;
        }
    }
    ithExitCritical();

    if (!file) {
        return -1;
    }



    if (flags & O_RDONLY) lfs_flags |= LFS_O_RDONLY;
    if (flags & O_WRONLY) lfs_flags |= LFS_O_WRONLY;
    if (flags & O_RDWR)   lfs_flags |= LFS_O_RDWR;
    if (flags & O_CREAT)  lfs_flags |= LFS_O_CREAT;
    if (flags & O_EXCL)   lfs_flags |= LFS_O_EXCL;
    if (flags & O_TRUNC)  lfs_flags |= LFS_O_TRUNC;
    if (flags & O_APPEND) lfs_flags |= LFS_O_APPEND;
//ithPrintf("[i] lfs_flags=%04X\n", flags);
    err = lfs_file_open(lfs, file, name+2, lfs_flags);
    if (err) {
        itp_files[i]=NULL;
        free(file);
        return err;
    }


    return i;
}

static int itp_lfs_close(int fd, void* info)
{
    ITP_LFS_PREPAREBYFD();
    int err = lfs_file_close(lfs, file);
    free(file);
    itp_files[fd]=NULL;
    return err;
}

static int itp_lfs_read(int fd, char *ptr, int len, void* info)
{
    ITP_LFS_PREPAREBYFD();
    return lfs_file_read(lfs, file, ptr, len);
}

static int itp_lfs_write(int fd, char *ptr, int len, void* info)
{
    ITP_LFS_PREPAREBYFD();
    return lfs_file_write(lfs, file, ptr, len);
}

static int itp_lfs_seek(int fd, int ptr, int dir, void* info)
{
    ITP_LFS_PREPAREBYFD();
    return lfs_file_seek(lfs, file,  ptr, dir);
}

static int itp_lfs_remove(const char *path)
{
    ITP_LFS_PREPAREBYPATH(path);

    return lfs_remove(lfs, path+2);
}

static int itp_lfs_rename(const char *oldname, const char *newname)
{
    ITP_LFS_PREPAREBYPATH(oldname);

    return lfs_rename(lfs, oldname+2, newname+2);
}

static int itp_lfs_chdir(const char *path)
{
    struct itp_lfs_context* context;
    char *realpath;

    realpath=(char*)malloc(PATH_MAX+2);
    if (!realpath) {
        errno=ENOMEM;
        return -1;
    }

    if (!itp_lfs_realpath(path, realpath)) {
        free(realpath);
        return -1;
    }

    context=itp_lfs_get_context(realpath[0]);
    strcpy(context->cwd, realpath+2);
    free(realpath);

    return 0;
}

static int itp_lfs_chmod(const char *path, mode_t mode)
{
    return 0;
}

static int itp_lfs_mkdir(const char *path, mode_t mode)
{
    ITP_LFS_PREPAREBYPATH(path);

    return lfs_mkdir(lfs, path+2);
}

static int itp_lfs_rmdir(const char *path)
{
    ITP_LFS_PREPAREBYPATH(path);

    return lfs_remove(lfs, path+2);
}

static void itp_lfs_tostat(struct stat *s, struct lfs_info *info) {
    memset(s, 0, sizeof(struct stat));

    s->st_size = info->size;
    s->st_mode = S_IRWXU | S_IRWXG | S_IRWXO;

    switch (info->type) {
        case LFS_TYPE_DIR: s->st_mode |= S_IFDIR; break;
        case LFS_TYPE_REG: s->st_mode |= S_IFREG; break;
    }
}

static int itp_lfs_stat(const char *path, struct stat *sbuf)
{
    int n;
    ITP_LFS_PREPAREBYPATH(path);
    struct lfs_info lfs_info;

    n=lfs_stat(lfs, path+2, &lfs_info);
    if (n>=0) {
        itp_lfs_tostat(sbuf, &lfs_info);
    }
    return (n>=0)?0:-1;
}

static int itp_lfs_statvfs(const char *path, struct statvfs *sbuf)
{
    ITP_LFS_PREPAREBYPATH(path);
    
    lfs_ssize_t in_use = lfs_fs_size(lfs);
    if (in_use < 0) {
        return -1;
    }

    memset(sbuf, 0, sizeof(*sbuf));
    sbuf->f_bsize = config->block_size;
    sbuf->f_frsize = config->block_size;
    sbuf->f_blocks = config->block_count;
    sbuf->f_bfree = config->block_count - in_use;
    sbuf->f_bavail = config->block_count - in_use;
    sbuf->f_namemax = config->name_max;

    return 0;
}

static int itp_lfs_fstat(int fd, struct stat *st)
{
    ITP_LFS_PREPAREBYFD();

    itp_lfs_tostat(st, &(struct lfs_info){
        .size = lfs_file_size(lfs, file),
        .type = LFS_TYPE_REG,
    });

    return 0;
}

static char* itp_lfs_getcwd(char *buf, size_t size)
{
    struct itp_lfs_context* context=itp_lfs_get_context('A'+itpTaskGetVolume());

    if (!context) {
        return NULL;
    }

    buf[0]=context->drive_status->name[0];
    buf[1]=':';
    strlcpy(buf+2, context->cwd, size-2);
    return buf;
}

typedef struct itp_lfs_dir {
    lfs_dir_t dir;
    lfs_t *lfs;
    struct dirent dirent;
} itp_lfs_dir_t;

static DIR* itp_lfs_opendir(const char *path)
{
    ITP_LFS_PREPAREBYPATH(path);
    itp_lfs_dir_t *dir;
    int err;

    DIR* dirp = malloc(sizeof (DIR));
    if (!dirp)
    {
        errno = ENOMEM;
        return NULL;
    }

    dir = malloc(sizeof(*dir));
    if (!dir) {
        free(dirp);
        errno = ENOMEM;
        return NULL;
    }
    memset(dir, 0, sizeof(*dir));
    dir->lfs=lfs;

    err = lfs_dir_open(lfs, &dir->dir, path+2);
    if (err) {
        free(dir);
        free(dirp);
        errno=err;
        return NULL;
    }

    dirp->dd_fd     = ITP_DEVICE_LFS;
    dirp->dd_size   = sizeof (*dir);
    dirp->dd_buf    = (char*) dir;
    dirp->dd_loc    = -1;

    return dirp;
}

static struct dirent* itp_lfs_readdir(DIR *dirp)
{
    itp_lfs_dir_t *dir=(itp_lfs_dir_t*)dirp->dd_buf;
    struct dirent* d=&dir->dirent;
    struct lfs_info info;

    if (lfs_dir_read(dir->lfs, &dir->dir, &info)<=0) {
        return NULL;
    }

    memset(d, 0, sizeof(*d));
    d->d_type=(info.type==LFS_TYPE_DIR)?DT_DIR:DT_REG;
    d->d_namlen=strlen(info.name);
    strcpy(d->d_name, info.name);
//ithPrintf("[i] readdir: %s\n", d->d_name);
    return d;
}

static void itp_lfs_rewidndir(DIR *dirp)
{
    itp_lfs_dir_t *dir=(itp_lfs_dir_t*)dirp->dd_buf;
    struct dirent* d=&  dir->dirent;

    lfs_dir_rewind(dir->lfs, &dir->dir);
}

static int itp_lfs_closedir(DIR *dirp)
{
    itp_lfs_dir_t *dir=(itp_lfs_dir_t*)dirp->dd_buf;
    struct dirent* d=&dir->dirent;
    int err = lfs_dir_close(dir->lfs, &dir->dir);
    free(dir);
    free(dirp);
    return err;
}

static long itp_lfs_tell(int fd)
{
    ITP_LFS_PREPAREBYFD();

    return lfs_file_tell(lfs, file);
}

static int	itp_lfs_flush(int fd)
{
    ITP_LFS_PREPAREBYFD();

    return lfs_file_sync(lfs, file);
}

static int	itp_lfs_eof(int fd)
{
    ITP_LFS_PREPAREBYFD();

    return (file->pos>=file->ctz.size)?EOF:0;
}

static int itp_lfs_ioctl(int fd, unsigned long   request, void* ptr, void* info)
{
    int ret;
    
    switch (request)
    {
    case ITP_IOCTL_MOUNT:
        ret = itp_lfs_mount((ITPDisk)ptr);
        if (ret)
        {
            errno = (ITP_DEVICE_LFS << ITP_DEVICE_ERRNO_BIT) | ret;
            return -1;
        }
        break;

    case ITP_IOCTL_UNMOUNT:
        ret = itp_lfs_umount((ITPDisk)ptr);
        if (ret)
        {
            errno = (ITP_DEVICE_LFS << ITP_DEVICE_ERRNO_BIT) | ret;
            return -1;
        }
        break;

    case ITP_IOCTL_ENABLE:
        break;

    case ITP_IOCTL_DISABLE:
        break;

    case ITP_IOCTL_CREATE_PARTITION:
        ret = itp_lfs_create_partitions((ITPPartition*)ptr);
        if (ret)
        {
            errno = (ITP_DEVICE_LFS << ITP_DEVICE_ERRNO_BIT) | ret;
            return -1;
        }
        break;

    case ITP_IOCTL_GET_PARTITION:
        ret = itp_lfs_get_partitions((ITPPartition*)ptr);
        if (ret)
        {
            errno = (ITP_DEVICE_LFS << ITP_DEVICE_ERRNO_BIT) | ret;
            return -1;
        }
        break;

    case ITP_IOCTL_FORMAT_PARTITION:
        ret = itp_lfs_format_partition((ITPDisk)((((uint32_t)ptr)>>8)&0x00ff), (int)(((uint32_t)ptr)&0x00ff));
        if (ret)
        {
            errno = (ITP_DEVICE_LFS << ITP_DEVICE_ERRNO_BIT) | ret;
            return -1;
        }
        break;

    case ITP_IOCTL_INIT:
    ithPrintf("[i] itp_lfs_init\n");
        itp_lfs_init();
        break;

    default:
        errno = (ITP_DEVICE_LFS << ITP_DEVICE_ERRNO_BIT) | __LINE__;
        return -1;
    }
    return 0;
}

const ITPFSDevice itpLittlefsDevice =
{
    {
        ":lfs",
        itp_lfs_open,
        itp_lfs_close,
        itp_lfs_read,
        itp_lfs_write,
        itp_lfs_seek,
        itp_lfs_ioctl,
        NULL
    },
    itp_lfs_remove,
    itp_lfs_rename,
    itp_lfs_chdir,
    itp_lfs_chmod,
    itp_lfs_mkdir,
    itp_lfs_stat,
    itp_lfs_statvfs,
    itp_lfs_fstat,
    itp_lfs_getcwd,
    itp_lfs_rmdir,
    itp_lfs_closedir,
    itp_lfs_opendir,
    itp_lfs_readdir,
    itp_lfs_rewidndir,
    itp_lfs_tell,
    itp_lfs_flush,
    itp_lfs_eof
};

/*
 * char *realpath(const char *path, char resolved[PATH_MAX+2]);
 *
 * Find the real name of path, by removing all ".", ".." and symlink
 * components.  Returns (resolved) on success, or (NULL) on failure,
 * in which case the path which caused trouble is left in (resolved).
 */
static char *
itp_lfs_realpath(const char *path, char *_resolved)
{
	struct stat sb;
	char *p, *q, *s, *left, *next_token;
	size_t left_len, resolved_len;
	char *resolved=_resolved+2, *ret=NULL;
    int volume=itpTaskGetVolume();
    struct itp_lfs_context* context;

    left=(char*)malloc(PATH_MAX);
    next_token=(char*)malloc(PATH_MAX);
    if (!left || !next_token) {
        errno=ENOMEM;
        goto lExit;
    }

	if (path[0] == '/') {
        context=itp_lfs_get_context('A'+volume);
        if (!context) {
            errno=ENOENT;
            goto lExit;
        }
		resolved[0] = '/';
		resolved[1] = '\0';
		resolved_len = strlen(resolved);
		left_len = strlcpy(left, path + 1, PATH_MAX);
	} else if (path[1]==':') {
        context=itp_lfs_get_context(path[0]);
        if (!context) {
            errno=ENOENT;
            goto lExit;
        }
		if (path[2]=='/') {	
			resolved[0] = '/';
			resolved[1] = '\0';
			resolved_len = strlen(resolved);
			left_len = strlcpy(left, path + 3, PATH_MAX);
		}
		else {
            strcpy(resolved, context->cwd);
			resolved_len = strlen(resolved);
			left_len = strlcpy(left, path + 2, PATH_MAX);
		}
	}
	else {
        context=itp_lfs_get_context('A'+volume);
        if (!context) {
            errno=ENOENT;
            goto lExit;
        }
        strcpy(resolved, context->cwd);
		resolved_len = strlen(resolved);
		left_len = strlcpy(left, path, PATH_MAX);
	}

	if (left_len >= PATH_MAX || resolved_len >= PATH_MAX) {
		errno = ENAMETOOLONG;
        goto lExit;
	}

	/*
	 * Iterate over path components in `left'.
	 */
	while (left_len != 0) {
		/*
		 * Extract the next path component and adjust `left'
		 * and its length.
		 */
		p = strchr(left, '/');
		s = p ? p : left + left_len;
		if (s - left >= PATH_MAX) {
			errno = ENAMETOOLONG;
            goto lExit;
		}
		memcpy(next_token, left, s - left);
		next_token[s - left] = '\0';
		left_len -= s - left;
		if (p != NULL)
			memmove(left, s + 1, left_len + 1);
		if (resolved[resolved_len - 1] != '/') {
			if (resolved_len + 1 >= PATH_MAX) {
				errno = ENAMETOOLONG;
				goto lExit;
			}
			resolved[resolved_len++] = '/';
			resolved[resolved_len] = '\0';
		}
		if (next_token[0] == '\0')
			continue;
		else if (strcmp(next_token, ".") == 0)
			continue;
		else if (strcmp(next_token, "..") == 0) {
			/*
			 * Strip the last path component except when we have
			 * single "/"
			 */
			if (resolved_len > 1) {
				resolved[resolved_len - 1] = '\0';
				q = strrchr(resolved, '/') + 1;
				*q = '\0';
				resolved_len = q - resolved;
			}
			continue;
		}

		/*
		 * Append the next path component and lstat() it. If
		 * lstat() fails we still can return successfully if
		 * there are no more path components left.
		 */
		resolved_len = strlcat(resolved, next_token, PATH_MAX);
		if (resolved_len >= PATH_MAX) {
			errno = ENAMETOOLONG;
			goto lExit;
		}
	}
    
	/*
	 * Remove trailing slash except when the resolved pathname
	 * is a single "/".
	 */
	if (resolved_len > 1 && resolved[resolved_len - 1] == '/')
		resolved[resolved_len - 1] = '\0';
    _resolved[0]=context->drive_status->name[0];
    _resolved[1]=':';
    ret=_resolved;
lExit:
    if (left) free(left);
    if (next_token) free(next_token);
	return ret;
}
