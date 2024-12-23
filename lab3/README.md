# 3 类EXT2文件系统的设计
## 代码：[3.c]
```c
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>

#define FOPEN_TABLE_MAX 16
#define EXT2_NAME_LEN 255
#define DATA_BLOCK_COUNT 4096
#define INODE_COUNT 4096
#define DISK_SIZE 4611 
#define BLOCK_SIZE 512
#define GROUP_DESC_START 0
#define BLOCK_BITMAP_START 512
#define INODE_BITMAP_START 1024
#define INODE_TABLE_START 1536
#define DATA_BLOCK_START 263680

/* 组描述符 */
typedef struct GroupDesc {              
    char bg_volume_name[16];            
    uint16_t bg_block_bitmap;           
    uint16_t bg_inode_bitmap;           
    uint16_t bg_inode_table;
    uint16_t bg_free_blocks_count;
    uint16_t bg_free_inodes_count;
    uint16_t bg_used_dirs_count;
    char password[20];
    char bg_pad[464];
} GroupDesc;

/* 索引节点 */
typedef struct Inode {
    uint16_t i_mode;
    uint16_t i_blocks;
    uint32_t i_size;
    uint64_t i_atime;
    uint64_t i_ctime;
    uint64_t i_mtime;
    uint64_t i_dtime;
    uint16_t i_block[8];
    char i_pad[8];
} Inode;

/* 目录体 */
typedef struct DirEntry {
    uint16_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
    char name[EXT2_NAME_LEN];
} DirEntry;

/* 文件类型 */
typedef enum FileType {
    FT_UNKNOWN,
    FT_REG_FILE,
    FT_DIR,
    FT_CHRDEV,
    FT_BLKDEV,
    FT_FIFO,
    FT_SOCK,
    FT_SYMLINK
} FileType;

typedef void (*OperationFunc)(uint16_t);
typedef uint16_t (*SearchFunc)(char *, FileType, uint16_t);

FILE *fp = NULL;

/* 常驻内存的数据结构 */
uint16_t fopen_table[16];
uint16_t last_alloc_inode;
uint16_t last_alloc_block;
uint16_t current_dir;
char current_path[256];

/** 使用的函数 */
void update_group_desc(GroupDesc *group_desc);
GroupDesc *load_group_desc();
GroupDesc *new_group_desc();
void update_block_bitmap(uint8_t *block_bitmap);
uint8_t *load_block_bitmap();
void update_inode_bitmap(uint8_t *inode_bitmap);
Inode *load_inode_table(uint16_t index);
void reset_inode_table(uint16_t index);
uint8_t *load_block_entry(uint16_t index);
void update_block_entry(uint8_t* block_entry, uint16_t index);
void reset_block_entry(uint16_t index);
int append_dir_entry(DirEntry* dir_entry, uint16_t index);
uint16_t delete_dir_entry(char *name, FileType file_type, uint16_t index);
void handle_append_directory(DirEntry *dir_entry, uint16_t index);
int is_open(uint16_t inodeId);
void print_dir_entry(DirEntry *dir_entry);
void print_dir_entries_in_block(uint16_t index);
uint16_t handle_search_inode(char *name, FileType file_type, uint16_t index, SearchFunc func);
void handle_read_inode(uint16_t index, OperationFunc func);
void print_file_in_block(uint16_t index);
uint16_t new_block();
void stop_write();
char *input();
int try_write(uint16_t index, char *data_to_write);
void initialize_memory();
void initialize_disk();
uint16_t search_file_name(char *name, FileType file_type, uint16_t index);
void remove_file(char *name, FileType file_type);
void handle_write_inode(uint16_t index);
Inode * init_inode();
uint16_t new_inode();
DirEntry *init_dir_entry_with_inode(char file_name[], FileType type, uint16_t inodeId);
DirEntry *init_dir_entry(char file_name[], FileType type);
DirEntry *new_dir_entry(char file_name[], FileType type);

/** 底层 */
void update_group_desc(GroupDesc *group_desc) {
    fseek(fp, 0, SEEK_SET);
    fwrite(group_desc, sizeof(GroupDesc), 1, fp);
    fflush(fp);
}

GroupDesc *load_group_desc() {
    GroupDesc *group_desc = malloc(sizeof(GroupDesc));
    fseek(fp, 0, SEEK_SET);
    fread(group_desc, sizeof(GroupDesc), 1, fp);
    return group_desc;
}

GroupDesc *new_group_desc() {
    GroupDesc *group_desc = malloc(sizeof(GroupDesc));
    memset(group_desc->bg_volume_name, 0, sizeof(group_desc->bg_volume_name));  // 默认卷名为空
    // 组描述符块号为0
    group_desc->bg_block_bitmap = 1;         // 块位图块号为1 
    group_desc->bg_inode_bitmap = 2;         // 索引结点位图块号为2
    group_desc->bg_inode_table = 3;          // 索引节点表起始块号为3
    group_desc->bg_free_blocks_count = DATA_BLOCK_COUNT;
    group_desc->bg_free_inodes_count = INODE_COUNT;
    group_desc->bg_used_dirs_count = 0;
    memset(group_desc->password, 0, sizeof(group_desc->password));  // 默认卷名为空
    printf("Please set your password\n");
    char password[20];
    fgets(password, 20, stdin);
    password[strlen(password)-1] = '\0';
    strcpy(group_desc->password, password);
    printf("Password Saved\n");
    memset(group_desc->bg_pad, 0xFF, sizeof(group_desc->bg_pad));
    return group_desc;
}

void update_block_bitmap(uint8_t *block_bitmap) {
    fseek(fp, BLOCK_BITMAP_START, SEEK_SET);
    fwrite(block_bitmap, BLOCK_SIZE, 1, fp);
    fflush(fp);
}

uint8_t *load_block_bitmap() {
    uint8_t *block_bitmap = malloc(BLOCK_SIZE);
    fseek(fp, BLOCK_BITMAP_START, SEEK_SET);
    fread(block_bitmap, BLOCK_SIZE, 1, fp);
    return block_bitmap;
}

void update_inode_bitmap(uint8_t *inode_bitmap) {
    fseek(fp, INODE_BITMAP_START, SEEK_SET);
    fwrite(inode_bitmap, BLOCK_SIZE, 1, fp);
    fflush(fp);
}

uint8_t * load_inode_bitmap() {
    uint8_t *inode_bitmap = malloc(BLOCK_SIZE);
    fseek(fp, INODE_BITMAP_START, SEEK_SET);
    fread(inode_bitmap, BLOCK_SIZE, 1, fp);
    return inode_bitmap;
}

void update_inode_table(Inode *inode, uint16_t index) {
    index--;
    fseek(fp, INODE_TABLE_START + sizeof(Inode)*index, SEEK_SET);
    fwrite(inode, sizeof(Inode), 1, fp);
    fflush(fp);
}

Inode *load_inode_table(uint16_t index) {
    index--;
    Inode *inode = malloc(sizeof(Inode));
    fseek(fp, INODE_TABLE_START + sizeof(Inode)*index, SEEK_SET);
    fread(inode, sizeof(Inode), 1, fp);
    return inode;
}

void reset_inode_table(uint16_t index) {
    index--;
    Inode *inode = malloc(sizeof(Inode));
    memset(inode, 0, sizeof(Inode));
    fseek(fp, INODE_TABLE_START + sizeof(Inode)*index, SEEK_SET);
    fwrite(inode, sizeof(Inode), 1, fp);
    free(inode);
    uint8_t *inode_bitmap = load_inode_bitmap();
    uint8_t j = 0b10000000;
    j >>= index % 8;
    j = ~j;
    inode_bitmap[index/8] &= j;
    update_inode_bitmap(inode_bitmap);
    free(inode_bitmap);
}

uint8_t *load_block_entry(uint16_t index) {
    uint8_t *block_entry = malloc(BLOCK_SIZE);
    fseek(fp, DATA_BLOCK_START + BLOCK_SIZE*index, SEEK_SET);
    fread(block_entry, BLOCK_SIZE, 1, fp);
    return block_entry;
}

void update_block_entry(uint8_t* block_entry, uint16_t index) {
    fseek(fp, DATA_BLOCK_START + BLOCK_SIZE*index, SEEK_SET);
    fwrite(block_entry, BLOCK_SIZE, 1, fp);
    fflush(fp);
}

void reset_block_entry(uint16_t index) {
    uint8_t *zeros = malloc(BLOCK_SIZE);
    memset(zeros, 0, BLOCK_SIZE);
    fseek(fp, DATA_BLOCK_START + BLOCK_SIZE*index, SEEK_SET);
    fwrite(zeros, BLOCK_SIZE, 1, fp);
    free(zeros);
    uint8_t *block_bitmap = load_block_bitmap();
    uint8_t j = 0b10000000;
    j >>= index % 8;
    j = ~j;
    block_bitmap[index/8] &= j;
    update_block_bitmap(block_bitmap);
    free(block_bitmap);
}

int append_dir_entry(DirEntry* dir_entry, uint16_t index) {
    uint16_t i = 0;
    uint8_t *block_entry = load_block_entry(index);
    DirEntry temp = ((DirEntry *)block_entry)[0];
    while (temp.inode > 0) {
        uint16_t rsize = temp.rec_len - temp.name_len - 7;
        if (rsize >= dir_entry->rec_len) {
            temp.rec_len = temp.name_len + 7;
            ((DirEntry *)(block_entry + i))[0].rec_len = temp.rec_len;
            dir_entry->rec_len = rsize;
            ((DirEntry *)(block_entry + i + temp.rec_len))[0].inode = dir_entry->inode;
            ((DirEntry *)(block_entry + i + temp.rec_len))[0].rec_len = dir_entry->rec_len;
            ((DirEntry *)(block_entry + i + temp.rec_len))[0].name_len = dir_entry->name_len;
            ((DirEntry *)(block_entry + i + temp.rec_len))[0].file_type = dir_entry->file_type;
            strcpy(((DirEntry *)(block_entry + i + temp.rec_len))[0].name, dir_entry->name);

            update_block_entry(block_entry, index);
            free(block_entry);
            return 1;
        }
        i += temp.rec_len;
        temp = ((DirEntry *)(block_entry + i))[0];
    }
    if (BLOCK_SIZE - i > dir_entry->rec_len) {
        ((DirEntry *)(block_entry + i))[0] = *dir_entry;
        update_block_entry(block_entry, index);
        free(block_entry);
        return 1;
    }
    free(block_entry);
    return 0;
}

uint16_t delete_dir_entry(char *name, FileType file_type, uint16_t index) {
    uint16_t i = 0;
    uint8_t *block_entry = load_block_entry(index);
    DirEntry temp = ((DirEntry *)block_entry)[0];
    DirEntry next = ((DirEntry *)(block_entry + temp.rec_len))[0];
    while (next.inode > 0) {
        uint16_t rsize = temp.rec_len - temp.name_len - 7;
        if (!strcmp(name, next.name) && next.file_type == file_type) {
            ((DirEntry *)(block_entry + i + temp.rec_len))[0].inode = 0;
            ((DirEntry *)(block_entry + i))[0].rec_len += next.rec_len;
            update_block_entry(block_entry, index);
            free(block_entry);
            return 1;
        }
        i += temp.rec_len;
        temp = ((DirEntry *)(block_entry + i))[0];
        next = ((DirEntry *)(block_entry + i + temp.rec_len))[0];
    }
    free(block_entry);
    return 0;
}

void handle_append_directory(DirEntry *dir_entry, uint16_t index) {
    uint8_t *allZero = malloc(sizeof(Inode));
    memset(allZero, 0, sizeof(allZero));
    Inode *inode = load_inode_table(index);
    inode->i_ctime = time(NULL);
    inode->i_mtime = time(NULL);
    if (inode->i_blocks == 0) {
        uint16_t newBlock = new_block();
        inode->i_blocks++;
        inode->i_block[0] = newBlock;
    }
    for (int b = 0; b < inode->i_blocks; b++) {
        if (b < 6) {
            if (append_dir_entry(dir_entry, inode->i_block[b])) {
                break;
            }
            if (b == inode->i_blocks - 1) {
                uint16_t block = new_block();
                inode->i_blocks++;
                inode->i_block[b+1] = block;
            }
        } else if (b == 6) {
            uint8_t *block_entry = load_block_entry(inode->i_block[b]);
            Inode *subInode = (Inode *) block_entry;
            if (!memcmp(subInode, allZero, sizeof(Inode))) {
                subInode[0] = *init_inode();
                subInode[0].i_blocks++;
                subInode[0].i_block[0] = new_block();
            }
            for (int idx = 0; !memcmp(subInode+idx, allZero, sizeof(Inode)); idx++) {
                for (int sb = 0; sb < subInode[idx].i_blocks; sb++) {
                    if (append_dir_entry(dir_entry, inode->i_block[b])) {
                        break;
                    }
                    if (sb == subInode[idx].i_blocks - 1) {
                        if (subInode[idx].i_blocks == 7) {
                            break;
                        }
                        subInode[idx].i_blocks++;
                        subInode[idx].i_block[sb+1] = new_block();
                    }
                }
                if (!memcmp(subInode+idx+1, allZero, sizeof(Inode))) {
                    if (idx == BLOCK_SIZE / sizeof(Inode) - 1) {
                        break;
                    }
                    subInode[idx+1] = *init_inode();
                    subInode[idx+1].i_blocks++;
                    subInode[idx+1].i_block[0] = new_block();
                }
            }
            update_block_entry(block_entry, inode->i_block[b]);
            free(block_entry);
        } else if (b == 7) {
            uint8_t *block_entry = load_block_entry(inode->i_block[b]);
            uint16_t *sub_block = (uint16_t *) block_entry;
            for (int bid = 0; sub_block[bid] != 0; bid++) {
                uint8_t *sub_block_entry = load_block_entry(sub_block[bid]);
                Inode *subInode = (Inode *) sub_block_entry;
                for (int idx = 0; subInode[idx].i_blocks > 0; idx++) {
                    for (int sb = 0; sb < subInode[idx].i_blocks; sb++) {
                        if (append_dir_entry(dir_entry, inode->i_block[b])) {
                            break;
                        }
                        if (sb == subInode[idx].i_blocks - 1) {
                            if (subInode[idx].i_blocks == 7) {
                                break;
                            }
                            subInode[idx].i_blocks++;
                            subInode[idx].i_block[sb+1] = new_block();
                        }
                    }
                    if (!memcmp(subInode+idx+1, allZero, sizeof(Inode))) {
                        if (idx == BLOCK_SIZE / sizeof(Inode) - 1) {
                            break;
                        }
                        subInode[idx+1] = *init_inode();
                        subInode[idx+1].i_blocks++;
                        subInode[idx+1].i_block[0] = new_block();
                    }
                }
                if (sub_block[bid+1] == 0) {
                    if (bid == BLOCK_SIZE / 16 - 1) {
                        break;
                    }
                    sub_block[bid+1] = new_block();
                }
                free(sub_block_entry);
            }
            update_block_entry(block_entry, inode->i_block[b]);
            free(block_entry);
        }
    }
    update_inode_table(inode, index);
    free(inode);
    free(allZero);
}

int is_open(uint16_t inodeId) {
    for (int i = 0; i < FOPEN_TABLE_MAX; ++i) {
        if (fopen_table[i] == inodeId) {;
            return 1;
        }
    }
    return 0;
}

void print_dir_entry(DirEntry *dir_entry) {
    Inode *inode = load_inode_table(dir_entry->inode);
    if (strlen(dir_entry->name) < 8)
        printf("%s\t\t", dir_entry->name);
    else
        printf("%s\t", dir_entry->name);
    switch (dir_entry->file_type) {
        case FT_DIR:
            printf("d");
            break;
        case FT_BLKDEV:
            printf("b");
            break;
        case FT_CHRDEV:
            printf("c");
            break;
        case FT_FIFO:
            printf("p");
            break;
        case FT_SOCK:
            printf("s");
            break;
        case FT_SYMLINK:
            printf("l");
            break;
        default:
            printf("-");
            break;   
    }
    switch(inode->i_mode & 0x00ff) {
        case 0:
            printf("---------");
            break;
        case 1:
            printf("--x--x--x");
            break;
        case 2:
            printf("-w--w--w-");
            break;
        case 3:
            printf("-wx-wx-wx");
            break;
        case 4:
            printf("r--r--r--");
            break;
        case 5:
            printf("r-xr-xr-x");
            break;
        case 6:
            printf("rw-rw-rw-");
            break;
        default:
            printf("rwxrwxrwx");
            break;
    }
    printf("  ");
    printf("%u\t\t", inode->i_size);
    char *str = ctime(&inode->i_ctime);
    str[24] = '\0';
    printf("%s  ", str);
    str = ctime(&inode->i_atime);
    str[24] = '\0';
    printf("%s  ", str);
    printf("%s", ctime(&inode->i_mtime));
    free(inode);
}

void print_dir_entries_in_block(uint16_t index) {
    uint8_t *block_entry = load_block_entry(index);
    for (DirEntry *tmp = (DirEntry *)block_entry; tmp->inode > 0; tmp = (DirEntry *)(((uint8_t *)tmp) + tmp->rec_len)) {
        print_dir_entry(tmp);
    }
    free(block_entry);
}

uint16_t handle_search_inode(char *name, FileType file_type, uint16_t index, SearchFunc func) {
    Inode *inode = load_inode_table(index);
    if (func == delete_dir_entry) {
        inode->i_ctime = time(NULL);
        inode->i_mtime = time(NULL);
        update_inode_table(inode, index);
    }
    for (int b = 0; b < inode->i_blocks; b++) {
        if (b < 6) {
            uint16_t result = func(name, file_type, inode->i_block[b]);
            if (result != 0) {
                free(inode);
                return result;
            }
        } else if (b == 6) {
            uint8_t *block_entry = load_block_entry(inode->i_block[b]);
            Inode *subInode = (Inode *) block_entry;
            for (int idx = 0; subInode[idx].i_blocks > 0; idx++) {
                for (int sb = 0; sb < subInode[idx].i_blocks; sb++) {
                    uint16_t result = func(name, file_type, subInode[idx].i_block[sb]);
                    if (result != 0) {
                        free(block_entry);
                        free(inode);
                        return result;
                    }
                }
            }
            free(block_entry);
        } else if (b == 7) {
            uint8_t *block_entry = load_block_entry(inode->i_block[b]);
            uint16_t *sub_block = (uint16_t *) block_entry;
            for (int bid = 0; sub_block[bid] != 0; bid++) {
                uint8_t *sub_block_entry = load_block_entry(sub_block[bid]);
                Inode *subInode = (Inode *) sub_block_entry;
                for (int idx = 0; subInode[idx].i_blocks > 0; idx++) {
                    for (int sb = 0; sb < subInode[idx].i_blocks; sb++) {
                        uint16_t result = func(name, file_type, subInode[idx].i_block[sb]);
                        if (result != 0) {
                            free(block_entry);
                            free(sub_block_entry);
                            free(inode);
                            return result;
                        }
                    }
                }
                free(sub_block_entry);
            }
            free(block_entry);
        }
    }
    free(inode);
    return 0;
}

void handle_read_inode(uint16_t index, OperationFunc func) {
    Inode *inode = load_inode_table(index);
    if (func == print_file_in_block) {
        inode->i_atime = time(NULL);
        update_inode_table(inode, index);
    }
    if (func == print_dir_entries_in_block) {
        printf("File Name\t");
        printf("File Mode   ");
        printf("Size\t");
        printf("Change Time               ");
        printf("Access Time               ");
        printf("Modification Time\n");
    }
    for (int b = 0; b < inode->i_blocks; b++) {
        if (b < 6) {
            func(inode->i_block[b]);
        } else if (b == 6) {
            uint8_t *block_entry = load_block_entry(inode->i_block[b]);
            Inode *subInode = (Inode *) block_entry;
            for (int idx = 0; subInode[idx].i_blocks > 0; idx++) {
                for (int sb = 0; sb < subInode[idx].i_blocks; sb++) {
                    func(subInode[idx].i_block[sb]);
                }
            }
            free(block_entry);
        } else if (b == 7) {
            uint8_t *block_entry = load_block_entry(inode->i_block[b]);
            uint16_t *sub_block = (uint16_t *) block_entry;
            for (int bid = 0; sub_block[bid] != 0; bid++) {
                uint8_t *sub_block_entry = load_block_entry(sub_block[bid]);
                Inode *subInode = (Inode *) sub_block_entry;
                for (int idx = 0; subInode[idx].i_blocks > 0; idx++) {
                    for (int sb = 0; sb < subInode[idx].i_blocks; sb++) {
                        func(subInode[idx].i_block[sb]);
                    }
                }
                free(sub_block_entry);
            }
            free(block_entry);
        }
    }
    free(inode);
}

void print_file_in_block(uint16_t index) {
    uint8_t *block_entry = load_block_entry(index);
    for (int i = 0; block_entry[i] != 0; i++) {
        printf("%c", (char) block_entry[i]);
    }
    free(block_entry);
}

uint16_t new_block() {
    GroupDesc *group_desc = load_group_desc();
    if (group_desc->bg_free_blocks_count <= 0) {
        printf("No free blocks.");
        free(group_desc);
        return DATA_BLOCK_COUNT;
    }
    uint8_t *block_bitmap = load_block_bitmap();
    int i;
    for (i = 0; i < BLOCK_SIZE; i++) {
        if (block_bitmap[i] == (uint8_t) -1) {
            continue;
        }
        break;
    }
    uint8_t j = 0b10000000;
    int num = 0;
    while ((block_bitmap[i] | j) == block_bitmap[i]) {
        j >>= 1;
        num++;
    }
    block_bitmap[i] |= j;
    update_block_bitmap(block_bitmap);
    group_desc->bg_free_blocks_count--;
    update_group_desc(group_desc);
    free(block_bitmap);
    free(group_desc);
    return 8*i+num;
}

int writing = 0;
void stop_write() {
    writing = 0;
}
char *input() {
    writing = 1;
    char *buf = malloc(1024);
    char c;
    int blen = 0;
    signal(SIGQUIT, stop_write);
    while (writing == 1 && (c = getchar()) != EOF) {
        buf[blen++] = c;
        if (blen >= 1024)
            break;
    }
    buf[blen-1] = '\0';
    return buf;
}

int try_write(uint16_t index, char *data_to_write) {
    char *block_entry = (char *)load_block_entry(index);
    if (strlen(block_entry) == BLOCK_SIZE-1) {
        free(block_entry);
        return 0;
    }
    int written = BLOCK_SIZE - 1 - strlen(block_entry);
    strncat(block_entry, data_to_write, written);
    update_block_entry(block_entry, index);
    free(block_entry);
    return written;
}

void handle_write_inode(uint16_t index) {
    uint8_t *allZero = malloc(sizeof(Inode));
    memset(allZero, 0, sizeof(allZero));
    Inode *inode = load_inode_table(index);
    
    inode->i_atime = time(NULL);
    inode->i_mtime = time(NULL);
    if (inode->i_blocks == 0) {
        uint16_t newBlock = new_block();
        inode->i_blocks++;
        inode->i_block[0] = newBlock;
    }
    char *buf = input();
    inode->i_size += strlen(buf) + 1;
    printf("%luB written\n", strlen(buf) + 1);
    for (int b = 0; b < inode->i_blocks; b++) {
        if (b < 6) {
            int written = try_write(inode->i_block[b], buf);
            if (strlen(buf) <= written) {
                break;
            }
            buf += written;
            if (b == inode->i_blocks - 1) {
                uint16_t block = new_block();
                inode->i_blocks++;
                inode->i_block[b+1] = block;
            }
        } else if (b == 6) {
            uint8_t *block_entry = load_block_entry(inode->i_block[b]);
            Inode *subInode = (Inode *) block_entry;
            if (!memcmp(subInode, allZero, sizeof(Inode))) {
                subInode[0] = *init_inode();
                subInode[0].i_blocks++;
                subInode[0].i_block[0] = new_block();
            }
            for (int idx = 0; !memcmp(subInode+idx, allZero, sizeof(Inode)); idx++) {
                for (int sb = 0; sb < subInode[idx].i_blocks; sb++) {
                    int written = try_write(subInode[0].i_block[sb+1], buf);
                    if (strlen(buf) <= written) {
                        break;
                    }
                    buf += written;
                    if (sb == subInode[idx].i_blocks - 1) {
                        if (subInode[idx].i_blocks == 7) {
                            break;
                        }
                        subInode[idx].i_blocks++;
                        subInode[idx].i_block[sb+1] = new_block();
                    }
                }
                if (!memcmp(subInode+idx+1, allZero, sizeof(Inode))) {
                    if (idx == BLOCK_SIZE / sizeof(Inode) - 1) {
                        break;
                    }
                    subInode[idx+1] = *init_inode();
                    subInode[idx+1].i_blocks++;
                    subInode[idx+1].i_block[0] = new_block();
                }
            }
            update_block_entry(block_entry, inode->i_block[b]);
            free(block_entry);
        } else if (b == 7) {
            uint8_t *block_entry = load_block_entry(inode->i_block[b]);
            uint16_t *sub_block = (uint16_t *) block_entry;
            for (int bid = 0; sub_block[bid] != 0; bid++) {
                uint8_t *sub_block_entry = load_block_entry(sub_block[bid]);
                Inode *subInode = (Inode *) sub_block_entry;
                for (int idx = 0; !memcmp(subInode+idx, allZero, sizeof(Inode)); idx++) {
                    for (int sb = 0; sb < subInode[idx].i_blocks; sb++) {
                        int written = try_write(subInode[0].i_block[sb+1], buf);
                        if (strlen(buf) <= written) {
                            break;
                        }
                        buf += written;
                        if (sb == subInode[idx].i_blocks - 1) {
                            if (subInode[idx].i_blocks == 7) {
                                break;
                            }
                            subInode[idx].i_blocks++;
                            subInode[idx].i_block[sb+1] = new_block();
                        }
                    }
                    if (!memcmp(subInode+idx+1, allZero, sizeof(Inode))) {
                        if (idx == BLOCK_SIZE / sizeof(Inode) - 1) {
                            break;
                        }
                        subInode[idx+1] = *init_inode();
                        subInode[idx+1].i_blocks++;
                        subInode[idx+1].i_block[0] = new_block();
                    }
                }
                if (sub_block[bid+1] == 0) {
                    if (bid == BLOCK_SIZE / 16 - 1) {
                        update_block_entry(sub_block_entry, sub_block[bid]);
                        free(sub_block_entry);
                        break;
                    }
                    sub_block[bid+1] = new_block();
                }
                update_block_entry(sub_block_entry, sub_block[bid]);
                free(sub_block_entry);
            }
            update_block_entry(block_entry, inode->i_block[b]);
            free(block_entry);
        }
    }
    update_inode_table(inode, index);
    free(inode);
    free(allZero);
    free(buf);
}

Inode * init_inode() {
    Inode * inode = malloc(sizeof(Inode));
    inode->i_mode = FT_UNKNOWN;
    inode->i_mode <<= 8;
    inode->i_mode += 0b110;      // 默认可读可写不可执行。
    inode->i_blocks = 0;
    inode->i_size = 0;
    inode->i_atime = time(NULL);
    inode->i_ctime = time(NULL);
    inode->i_mtime = time(NULL);
    inode->i_dtime = 0;
    memset(inode->i_block, 0, sizeof(inode->i_block));
    memset(inode->i_pad, 0xFF, sizeof(inode->i_pad));
    return inode;
}

uint16_t new_inode() {
    GroupDesc *group_desc = load_group_desc();
    if (group_desc->bg_free_inodes_count <= 0) {
        printf("No free inodes.");
        return 0;
    }
    uint8_t *inode_bitmap = load_inode_bitmap();
    uint16_t i;
    for (i = 0; i < BLOCK_SIZE; i++) {
        if (inode_bitmap[i] == 0b11111111) {
            continue;
        }
        break;
    }
    uint8_t j = 0b10000000;
    int num = 0;
    while ((inode_bitmap[i] | j) == inode_bitmap[i]) {
        j >>= 1;
        num++;
    }
    inode_bitmap[i] |= j;
    update_inode_bitmap(inode_bitmap);

    Inode *inode = init_inode();
    update_inode_table(inode, 8*i+num+1);

    group_desc->bg_free_inodes_count--;
    update_group_desc(group_desc);
    free(inode_bitmap);
    free(inode);
    free(group_desc);
    return 8*i+num+1;
}

DirEntry *init_dir_entry_with_inode(char file_name[], FileType type, uint16_t inodeId) {
    DirEntry *dirEntry = malloc(sizeof(DirEntry));
    memset(dirEntry, 0, sizeof(DirEntry));
    dirEntry->inode = inodeId;
    dirEntry->name_len = strlen(file_name);
    dirEntry->rec_len = 7 + dirEntry->name_len;
    dirEntry->file_type = type;
    strcpy(dirEntry->name, file_name);
    return dirEntry;
}

DirEntry *init_dir_entry(char file_name[], FileType type) {
    uint16_t inodeId = new_inode();
    if (inodeId == 0) {
        return NULL;
    }
    DirEntry *dirEntry = malloc(sizeof(DirEntry));
    memset(dirEntry, 0, sizeof(DirEntry));
    dirEntry->inode = inodeId;
    dirEntry->name_len = strlen(file_name);
    dirEntry->rec_len = 7 + dirEntry->name_len;
    dirEntry->file_type = type;
    strcpy(dirEntry->name, file_name);
    return dirEntry;
}

DirEntry *new_dir_entry(char file_name[], FileType type) {
    DirEntry *dir_entry = init_dir_entry(file_name, type);
    if (dir_entry == NULL)
        return NULL;
    Inode *inode = load_inode_table(dir_entry->inode);
    inode->i_mode |= type << 8;
    if (type == FT_DIR) {
        GroupDesc *group_desc = load_group_desc();
        group_desc->bg_used_dirs_count++;
        update_group_desc(group_desc);
        uint16_t newBlock = new_block();
        if (newBlock != DATA_BLOCK_COUNT) {
            DirEntry *dot_entry = init_dir_entry_with_inode(".", FT_DIR, dir_entry->inode);
            DirEntry *dotdot_entry = init_dir_entry_with_inode("..", FT_DIR, current_dir);
            append_dir_entry(dot_entry, newBlock);
            append_dir_entry(dotdot_entry, newBlock);
            inode->i_blocks = 1;
            inode->i_size = dot_entry->rec_len + dotdot_entry->rec_len;
            inode->i_block[0] = newBlock;
            update_inode_table(inode, dir_entry->inode);
            free(dot_entry);
            free(dotdot_entry);
        }
        free(group_desc);
        free(inode);
        return dir_entry;
    }
    char *ext = strchr(file_name, '.');
    if (ext == NULL || !strcmp(ext, ".exe") || !strcmp(ext, ".bin") || !strcmp(ext, ".com")) {
        inode->i_mode += 1;
        update_inode_table(inode, dir_entry->inode);
    }
    free(inode);
    return dir_entry;
}

/** 初始化文件系统 */

/* 初始化文件系统的内存数据 */
void initialize_memory() {
    memset(fopen_table, 0, sizeof(fopen_table));
    last_alloc_block = 1;
    last_alloc_inode = 1;
    current_dir = 1;
    strcpy(current_path, "root");
}
/* 建立文件系统 */
void initialize_disk() {
    int initial_data[128];
    memset(initial_data, 0, sizeof(initial_data));
    for (int i = 0; i < DISK_SIZE; i++) {
        fseek(fp, i * BLOCK_SIZE, SEEK_SET);
        fwrite(initial_data, BLOCK_SIZE, 1, fp);
        fflush(fp);
    } 
    GroupDesc *group_desc = new_group_desc();
    update_group_desc(group_desc);
    free(new_dir_entry("root", FT_DIR));
    free(group_desc);
}

uint16_t search_file_name(char *name, FileType file_type, uint16_t index) {
    uint8_t *block_entry = load_block_entry(index);
    uint16_t i = 0;
    DirEntry temp = ((DirEntry *)block_entry)[0];
    while (temp.inode > 0) {
        if (!strcmp(temp.name, name) && temp.file_type == file_type) {
            free(block_entry);
            return temp.inode;
        }
        i += temp.rec_len;
        temp = ((DirEntry *)(block_entry + i))[0];
    }
    free(block_entry);
    return 0;
}

void remove_file(char *name, FileType file_type) {
    handle_search_inode(name, current_dir, file_type, delete_dir_entry);
}

void dir() {
    handle_read_inode(current_dir, print_dir_entries_in_block);
}

void mkdir(char *name) {
    name[strlen(name)-1] = '\0';
    uint16_t inodeId = handle_search_inode(name, FT_DIR, current_dir, search_file_name);
    if (inodeId != 0) {
        printf("mkdir: cannot create directory '%s': File exists\n", name);
        return;
    }
    DirEntry *newDir = new_dir_entry(name, FT_DIR);
    Inode *inode = load_inode_table(current_dir);
    inode->i_size += 7 + strlen(name);
    update_inode_table(inode, current_dir);
    handle_append_directory(newDir, current_dir);
    free(inode);
    free(newDir);
}

void rmdir(char *name) {
    name[strlen(name)-1] = '\0';
    if (!strcmp(name, ".") || !strcmp(name, "..")) {
        printf("rmdir: failed to remove '%s': Invalid argument\n", name);
        return;
    }
    uint16_t inodeId = handle_search_inode(name, FT_DIR, current_dir, search_file_name);
    if (inodeId == 0) {
        printf("rmdir: failed to remove '%s': No such file or directory\n", name);
        return;
    }
    Inode *inode = load_inode_table(inodeId);
    if (inode->i_size != 17) {
        printf("rmdir: failed to remove '%s': Directory not empty\n", name);
        free(inode);
        return;
    }
    update_inode_table(inode, inodeId);
    free(inode);
    handle_read_inode(inodeId, reset_block_entry);
    inode = load_inode_table(inodeId);
    reset_inode_table(inodeId);
    GroupDesc *group_desc = load_group_desc();
    Inode *curnode = load_inode_table(current_dir);
    curnode->i_size -= 7 + strlen(name);
    update_inode_table(curnode, current_dir);
    free(curnode);
    group_desc->bg_free_blocks_count += inode->i_blocks;
    group_desc->bg_free_inodes_count++;
    group_desc->bg_used_dirs_count--;
    update_group_desc(group_desc);
    handle_search_inode(name, FT_DIR, current_dir, delete_dir_entry);
    free(group_desc);
    free(inode);
}

void create(char *name) {
    name[strlen(name)-1] = '\0';
    uint16_t inodeId = handle_search_inode(name, FT_REG_FILE, current_dir, search_file_name);
    if (inodeId != 0) {
        printf("create: cannot create file '%s': File exists\n", name);
        return;
    }
    DirEntry *newDir = new_dir_entry(name, FT_REG_FILE);
    Inode *inode = load_inode_table(current_dir);
    inode->i_size += 7 + strlen(name);
    update_inode_table(inode, current_dir);
    handle_append_directory(newDir, current_dir);
    free(newDir);
    free(inode);
}

void delete(char *name) {
    name[strlen(name)-1] = '\0';
    uint16_t inodeId = handle_search_inode(name, FT_REG_FILE, current_dir, search_file_name);
    if (inodeId == 0) {
        printf("delete: failed to remove '%s': No such file or directory\n", name);
        return;
    }
    if (is_open(inodeId) == 1) {
        printf("delete: failed to remove '%s': File opened\n", name);
        return;
    }
    Inode *inode = load_inode_table(inodeId);
    update_inode_table(inode, inodeId);
    free(inode);
    handle_read_inode(inodeId, reset_block_entry);
    inode = load_inode_table(inodeId);
    reset_inode_table(inodeId);
    GroupDesc *group_desc = load_group_desc();
    group_desc->bg_free_blocks_count += inode->i_blocks;
    group_desc->bg_free_inodes_count++;
    Inode *curnode = load_inode_table(current_dir);
    curnode->i_size -= 7 + strlen(name);
    update_inode_table(curnode, current_dir);
    free(curnode);
    update_group_desc(group_desc);
    handle_search_inode(name, FT_REG_FILE, current_dir, delete_dir_entry);
    free(group_desc);
    free(inode);
}

void cd(char *name) {
    name[strlen(name)-1] = '\0';
    uint16_t inodeId = handle_search_inode(name, FT_DIR, current_dir, search_file_name);
    if (inodeId == 0) {
        printf("cd: %s: No such file or directory\n", name);
        return;
    }
    Inode *inode = load_inode_table(inodeId);
    if ((inode->i_mode >> 8) != FT_DIR) {
        printf("cd: %s: Not a directory\n", name);
        free(inode);
        return;
    }
    if (!strcmp(name, ".")) {
        free(inode);
        return;
    }
    if(!strcmp(name, "..")) {
        char *lastx = strrchr(current_path, '/');
        if (lastx == NULL) {
            strcpy(current_path, "root");
            current_dir = inodeId;
            return;
        }
        lastx[0] = '\0';
        current_dir = inodeId;
        free(inode);
        return;
    }
    strcat(current_path, "/");
    strcat(current_path, name);
    current_dir = inodeId;
    free(inode);
}

void open(char name[]) {
    name[strlen(name)-1] = '\0';
    uint16_t inodeId = handle_search_inode(name, FT_REG_FILE, current_dir, search_file_name);
    if (inodeId == 0) {
        printf("open: %s: No such file or directory\n", name);
        return;
    }

    if (is_open(inodeId) == 1) {
        printf("open: %s: file opened\n", name);
        return;
    }
    Inode *inode = load_inode_table(inodeId);
    uint16_t permission = inode->i_mode & 0x00ff;
    if (permission == 4 || permission == 6 || permission == 7) {
        inode->i_atime = time(NULL);
        update_inode_table(inode, inodeId);
        for (int i = 0; i < FOPEN_TABLE_MAX; i++) {
            if (fopen_table[i] == 0) {
                fopen_table[i] = inodeId;
                free(inode);
                return;
            }
        }
        printf("open: cannot open '%s': too much opened files\n", name);
    } else {
        printf("open: cannot open '%s': Permission denied\n", name);
    }
    free(inode);
}

void pwd() {
    GroupDesc *group_desc = load_group_desc();
    printf("Please set your password\n");
    char password[20];
    fgets(password, 20, stdin);
    password[strlen(password)-1] = '\0';
    strcpy(group_desc->password, password);
    update_group_desc(group_desc);
    free(group_desc);
}

void login() {
    GroupDesc *group_desc = load_group_desc();
    printf("password for file system:\n");
    while(1) {
        char password[20];
        fgets(password, 20, stdin);
        password[strlen(password)-1] = '\0';  
        if (!strcmp(group_desc->password, password))
            break;
        printf("Sorry, try again.\n");
    }
    free(group_desc);
}

void close(char name[]) {
    name[strlen(name)-1] = '\0';
    uint16_t inodeId = handle_search_inode(name, FT_REG_FILE, current_dir, search_file_name);
    if (inodeId == 0) {
        printf("close: %s: No such file or directory\n", name);
        return;
    }

    if (is_open(inodeId) == 0) {
        printf("close: %s: File not opened\n", name);
        return;
    }
    for (int i = 0; i < FOPEN_TABLE_MAX; i++) {
        if (fopen_table[i] == inodeId) {
            fopen_table[i] = 0;
            return;
        }
    }
}

void read(char name[]) {
    name[strlen(name)-1] = '\0';
    uint16_t inodeId = handle_search_inode(name, FT_REG_FILE, current_dir, search_file_name);
    if (inodeId == 0) {
        printf("read: %s: No such file or directory\n", name);
        return;
    }

    if (is_open(inodeId) == 0) {
        printf("read: %s: File not opened\n", name);
        return;
    }
    Inode *inode = load_inode_table(inodeId);
    uint16_t permission = inode->i_mode & 0x00ff;
    if (permission == 4 || permission == 5 || permission == 6 || permission == 7) {
        inode->i_atime = time(NULL);
        update_inode_table(inode, inodeId);
        handle_read_inode(inodeId, print_file_in_block);
    } else {
        printf("read: cannot read '%s': Permission denied\n", name);
    }
    free(inode);
}

void write(char name[]) {
    name[strlen(name)-1] = '\0';
    uint16_t inodeId = handle_search_inode(name, FT_REG_FILE, current_dir, search_file_name);
    if (inodeId == 0) {
        printf("write: %s: No such file or directory\n", name);
        return;
    }

    if (is_open(inodeId) == 0) {
        printf("write: %s: File not opened\n", name);
        return;
    }
    Inode *inode = load_inode_table(inodeId);
    uint16_t permission = inode->i_mode & 0x00ff;
    if (permission == 2 || permission == 3 || permission == 6 || permission == 7) {
        handle_write_inode(inodeId);
    } else {
        printf("write: cannot write '%s': Permission denied\n", name);
    }
    free(inode);
}

void format() {
    initialize_memory();
    initialize_disk();
    printf("Format succeeded\n");
}

void chmod(char *command) {
    command[strlen(command)-1] = '\0';
    int permission = 0;
    int i = 1;
    for (i = 1; command[i] != ' '; i++) {
        if (command[i] == 'r') {
            permission += 4;
        }
        if (command[i] == 'w') {
            permission += 2;
        }
        if (command[i] == 'x') {
            permission += 1;
        }
    }
    char *name = command + i+1;
    uint16_t inodeId = handle_search_inode(name, FT_REG_FILE, current_dir, search_file_name);
    if (inodeId == 0) {
        printf("chmod: %s: No such file or directory\n", name);
        return;
    }
    Inode *inode = load_inode_table(inodeId);
    inode->i_ctime = time(NULL);
    switch(command[0]) {
        case '+':
            inode->i_mode |= permission;
            break;
        case '-':
            permission = ~permission;
            inode->i_mode &= permission;
            break;
        default:
            printf("chmod: Invalid mode\n");
            free(inode);
            return;
    }
    update_inode_table(inode, inodeId);
    free(inode);
}

void shell() {
    char instr[256];
    while (1) {
        printf("[%s]$ ", current_path);
        fgets(instr, sizeof(instr), stdin);
        if (!strncmp(instr, "ls", 2)) {
            dir();
        } else if (!strncmp(instr, "cd", 2)) {
            cd(instr + 3);
        } else if (!strncmp(instr, "passwd", 3)) {
            pwd();
        } else if (!strncmp(instr, "mkdir", 5)) {
            mkdir(instr + 6);
        } else if (!strncmp(instr, "create", 6)) {
            create(instr + 7);
        } else if (!strncmp(instr, "rmdir", 5)) {
            rmdir(instr + 6);
        } else if (!strncmp(instr, "delete", 6)) {
            delete(instr + 7);
        } else if (!strncmp(instr, "open", 4)) {
            open(instr + 5);
        } else if (!strncmp(instr, "close", 5)) {
            close(instr + 6);
        } else if (!strncmp(instr, "read", 4)) {
            read(instr + 5);
        } else if (!strncmp(instr, "write", 5)) {
            write(instr + 6);
        } else if (!strncmp(instr, "chmod", 5)) {
            chmod(instr + 6);
        } else if (!strncmp(instr, "format", 6)) {
            format();
        } else if (!strncmp(instr, "quit", 4)) {
            break;
        } else {
            instr[strlen(instr)-1] = '\0';
            printf("%s: command not found\n", instr);
        }
    }
}

int main() {
    initialize_memory();

    fp = fopen("./FS.txt", "rb+");
    if (fp == NULL) {
        fp = fopen("./FS.txt", "wb+");
        initialize_disk();
    } 
    login();
    shell();
    fclose(fp);
    return 0;
}
```
## 单元测试：
在本次实验中，我实现了几个简单的shell命令，包括：
### ls
用于显示当前目录下所有的目录文件。
- 测试：
```shell
[root]$ ls
File Name       File Mode   Size        Change Time               Access Time               Modification Time
.               drw-rw-rw-  17          Sun Dec 22 21:42:48 2024  Sun Dec 22 21:42:48 2024  Sun Dec 22 21:42:48 2024
..              drw-rw-rw-  17          Sun Dec 22 21:42:48 2024  Sun Dec 22 21:42:48 2024  Sun Dec 22 21:42:48 2024
```
- 实现相关函数：
```c
void dir() {
    handle_read_inode(current_dir, print_dir_entries_in_block);
}
```
命令层函数，调用handle_read_inode函数。
```c
void handle_read_inode(uint16_t index, OperationFunc func) {
    Inode *inode = load_inode_table(index);
    if (func == print_file_in_block) {
        inode->i_atime = time(NULL);
        update_inode_table(inode, index);
    }
    if (func == print_dir_entries_in_block) {
        printf("File Name\t");
        printf("File Mode   ");
        printf("Size\t");
        printf("Change Time               ");
        printf("Access Time               ");
        printf("Modification Time\n");
    }
    for (int b = 0; b < inode->i_blocks; b++) {
        if (b < 6) {
            func(inode->i_block[b]);
        } else if (b == 6) {
            uint8_t *block_entry = load_block_entry(inode->i_block[b]);
            Inode *subInode = (Inode *) block_entry;
            for (int idx = 0; subInode[idx].i_blocks > 0; idx++) {
                for (int sb = 0; sb < subInode[idx].i_blocks; sb++) {
                    func(subInode[idx].i_block[sb]);
                }
            }
            free(block_entry);
        } else if (b == 7) {
            uint8_t *block_entry = load_block_entry(inode->i_block[b]);
            uint16_t *sub_block = (uint16_t *) block_entry;
            for (int bid = 0; sub_block[bid] != 0; bid++) {
                uint8_t *sub_block_entry = load_block_entry(sub_block[bid]);
                Inode *subInode = (Inode *) sub_block_entry;
                for (int idx = 0; subInode[idx].i_blocks > 0; idx++) {
                    for (int sb = 0; sb < subInode[idx].i_blocks; sb++) {
                        func(subInode[idx].i_block[sb]);
                    }
                }
                free(sub_block_entry);
            }
            free(block_entry);
        }
    }
    free(inode);
}
```
用于遍历当前索引节点的每个数据块，并对数据块调用指定函数（func）。在这里调用的是print_dir_entries_in_block函数
```c
void print_dir_entries_in_block(uint16_t index) {
    uint8_t *block_entry = load_block_entry(index);
    for (DirEntry *tmp = (DirEntry *)block_entry; tmp->inode > 0; tmp = (DirEntry *)(((uint8_t *)tmp) + tmp->rec_len)) {
        print_dir_entry(tmp);
    }
    free(block_entry);
}
```
这个函数用于遍历单个数据块中的每个目录项，调用print_dir_entry函数输出信息。
```c
void print_dir_entry(DirEntry *dir_entry) {
    Inode *inode = load_inode_table(dir_entry->inode);
    if (strlen(dir_entry->name) < 8)
        printf("%s\t\t", dir_entry->name);
    else
        printf("%s\t", dir_entry->name);
    switch (dir_entry->file_type) {
        case FT_DIR:
            printf("d");
            break;
        case FT_BLKDEV:
            printf("b");
            break;
        case FT_CHRDEV:
            printf("c");
            break;
        case FT_FIFO:
            printf("p");
            break;
        case FT_SOCK:
            printf("s");
            break;
        case FT_SYMLINK:
            printf("l");
            break;
        default:
            printf("-");
            break;   
    }
    switch(inode->i_mode & 0x00ff) {
        case 0:
            printf("---------");
            break;
        case 1:
            printf("--x--x--x");
            break;
        case 2:
            printf("-w--w--w-");
            break;
        case 3:
            printf("-wx-wx-wx");
            break;
        case 4:
            printf("r--r--r--");
            break;
        case 5:
            printf("r-xr-xr-x");
            break;
        case 6:
            printf("rw-rw-rw-");
            break;
        default:
            printf("rwxrwxrwx");
            break;
    }
    printf("  ");
    printf("%u\t\t", inode->i_size);
    char *str = ctime(&inode->i_ctime);
    str[24] = '\0';
    printf("%s  ", str);
    str = ctime(&inode->i_atime);
    str[24] = '\0';
    printf("%s  ", str);
    printf("%s", ctime(&inode->i_mtime));
    free(inode);
}
```
这个函数用于解析目录项的信息并依次进行输出。

### cd
这个命令用于进入指定目录。
- 测试：
```shell
[root]$ mkdir 1111
[root]$ ls
File Name       File Mode   Size        Change Time               Access Time               Modification Time
.               drw-rw-rw-  28          Sun Dec 22 21:51:47 2024  Sun Dec 22 21:42:48 2024  Sun Dec 22 21:51:47 2024
..              drw-rw-rw-  28          Sun Dec 22 21:51:47 2024  Sun Dec 22 21:42:48 2024  Sun Dec 22 21:51:47 2024
1111            drw-rw-rw-  17          Sun Dec 22 21:51:47 2024  Sun Dec 22 21:51:47 2024  Sun Dec 22 21:51:47 2024
[root]$ cd 1111
[root/1111]$ ls
File Name       File Mode   Size        Change Time               Access Time               Modification Time
.               drw-rw-rw-  17          Sun Dec 22 21:51:47 2024  Sun Dec 22 21:51:47 2024  Sun Dec 22 21:51:47 2024
..              drw-rw-rw-  28          Sun Dec 22 21:51:47 2024  Sun Dec 22 21:42:48 2024  Sun Dec 22 21:51:47 2024
[root/1111]$ cd ..
[root]$ ls
File Name       File Mode   Size        Change Time               Access Time               Modification Time
.               drw-rw-rw-  28          Sun Dec 22 21:51:47 2024  Sun Dec 22 21:42:48 2024  Sun Dec 22 21:51:47 2024
..              drw-rw-rw-  28          Sun Dec 22 21:51:47 2024  Sun Dec 22 21:42:48 2024  Sun Dec 22 21:51:47 2024
1111            drw-rw-rw-  17          Sun Dec 22 21:51:47 2024  Sun Dec 22 21:51:47 2024  Sun Dec 22 21:51:47 2024
[root]$ cd .
[root]$ ls
File Name       File Mode   Size        Change Time               Access Time               Modification Time
.               drw-rw-rw-  28          Sun Dec 22 21:51:47 2024  Sun Dec 22 21:42:48 2024  Sun Dec 22 21:51:47 2024
..              drw-rw-rw-  28          Sun Dec 22 21:51:47 2024  Sun Dec 22 21:42:48 2024  Sun Dec 22 21:51:47 2024
1111            drw-rw-rw-  17          Sun Dec 22 21:51:47 2024  Sun Dec 22 21:51:47 2024  Sun Dec 22 21:51:47 2024
```
这里新建了一个新目录1111，并依次通过cd 1111进入该目录，通过cd ..返回上一级目录, cd .选择当前目录。
- 相关函数：
```c
void cd(char *name) {
    name[strlen(name)-1] = '\0';
    uint16_t inodeId = handle_search_inode(name, FT_DIR, current_dir, search_file_name);
    if (inodeId == 0) {
        printf("cd: %s: No such file or directory\n", name);
        return;
    }
    Inode *inode = load_inode_table(inodeId);
    if ((inode->i_mode >> 8) != FT_DIR) {
        printf("cd: %s: Not a directory\n", name);
        free(inode);
        return;
    }
    if (!strcmp(name, ".")) {
        free(inode);
        return;
    }
    if(!strcmp(name, "..")) {
        char *lastx = strrchr(current_path, '/');
        if (lastx == NULL) {
            strcpy(current_path, "root");
            current_dir = inodeId;
            return;
        }
        lastx[0] = '\0';
        current_dir = inodeId;
        free(inode);
        return;
    }
    strcat(current_path, "/");
    strcat(current_path, name);
    current_dir = inodeId;
    free(inode);
}
```
命令层函数，首先检查参数文件名是否符合要求，然后修改current_dir为要进入的目录，并修改current_path为新的路径。

### passwd
重置当前密码
- 测试：
```shell
[root]$ passwd
Please set your password
123
[root]$ quit
[yuyueryuyu@Yu lab3]$ ./3
password for file system:
223
Sorry, try again.
123
[root]$ 
```

- 相关函数：
```c
void pwd() {
    GroupDesc *group_desc = load_group_desc();
    printf("Please set your password\n");
    char password[20];
    fgets(password, 20, stdin);
    password[strlen(password)-1] = '\0';
    strcpy(group_desc->password, password);
    update_group_desc(group_desc);
    free(group_desc);
}
```
重置密码的函数。密码储存在组描述符中。

### mkdir
新建一个目录
- 测试：
```shell
[root]$ mkdir 1111
[root]$ ls
File Name       File Mode   Size        Change Time               Access Time               Modification Time
.               drw-rw-rw-  28          Sun Dec 22 21:51:47 2024  Sun Dec 22 21:42:48 2024  Sun Dec 22 21:51:47 2024
..              drw-rw-rw-  28          Sun Dec 22 21:51:47 2024  Sun Dec 22 21:42:48 2024  Sun Dec 22 21:51:47 2024
1111            drw-rw-rw-  17          Sun Dec 22 21:51:47 2024  Sun Dec 22 21:51:47 2024  Sun Dec 22 21:51:47 2024
```
这里新建了一个新目录1111。

- 相关函数：
```c
void mkdir(char *name) {
    name[strlen(name)-1] = '\0';
    uint16_t inodeId = handle_search_inode(name, FT_DIR, current_dir, search_file_name);
    if (inodeId != 0) {
        printf("mkdir: cannot create directory '%s': File exists\n", name);
        return;
    }
    DirEntry *newDir = new_dir_entry(name, FT_DIR);
    Inode *inode = load_inode_table(current_dir);
    inode->i_size += 7 + strlen(name);
    update_inode_table(inode, current_dir);
    handle_append_directory(newDir, current_dir);
    free(inode);
    free(newDir);
}
```
命令层函数，首先搜索索引节点，然后新建目录项，然后调用handle_append_directroy函数更新数据块。
```c
void handle_append_directory(DirEntry *dir_entry, uint16_t index) {
    uint8_t *allZero = malloc(sizeof(Inode));
    memset(allZero, 0, sizeof(allZero));
    Inode *inode = load_inode_table(index);
    inode->i_ctime = time(NULL);
    inode->i_mtime = time(NULL);
    if (inode->i_blocks == 0) {
        uint16_t newBlock = new_block();
        inode->i_blocks++;
        inode->i_block[0] = newBlock;
    }
    for (int b = 0; b < inode->i_blocks; b++) {
        if (b < 6) {
            if (append_dir_entry(dir_entry, inode->i_block[b])) {
                break;
            }
            if (b == inode->i_blocks - 1) {
                uint16_t block = new_block();
                inode->i_blocks++;
                inode->i_block[b+1] = block;
            }
        } else if (b == 6) {
            uint8_t *block_entry = load_block_entry(inode->i_block[b]);
            Inode *subInode = (Inode *) block_entry;
            if (!memcmp(subInode, allZero, sizeof(Inode))) {
                subInode[0] = *init_inode();
                subInode[0].i_blocks++;
                subInode[0].i_block[0] = new_block();
            }
            for (int idx = 0; !memcmp(subInode+idx, allZero, sizeof(Inode)); idx++) {
                for (int sb = 0; sb < subInode[idx].i_blocks; sb++) {
                    if (append_dir_entry(dir_entry, inode->i_block[b])) {
                        break;
                    }
                    if (sb == subInode[idx].i_blocks - 1) {
                        if (subInode[idx].i_blocks == 7) {
                            break;
                        }
                        subInode[idx].i_blocks++;
                        subInode[idx].i_block[sb+1] = new_block();
                    }
                }
                if (!memcmp(subInode+idx+1, allZero, sizeof(Inode))) {
                    if (idx == BLOCK_SIZE / sizeof(Inode) - 1) {
                        break;
                    }
                    subInode[idx+1] = *init_inode();
                    subInode[idx+1].i_blocks++;
                    subInode[idx+1].i_block[0] = new_block();
                }
            }
            update_block_entry(block_entry, inode->i_block[b]);
            free(block_entry);
        } else if (b == 7) {
            uint8_t *block_entry = load_block_entry(inode->i_block[b]);
            uint16_t *sub_block = (uint16_t *) block_entry;
            for (int bid = 0; sub_block[bid] != 0; bid++) {
                uint8_t *sub_block_entry = load_block_entry(sub_block[bid]);
                Inode *subInode = (Inode *) sub_block_entry;
                for (int idx = 0; subInode[idx].i_blocks > 0; idx++) {
                    for (int sb = 0; sb < subInode[idx].i_blocks; sb++) {
                        if (append_dir_entry(dir_entry, inode->i_block[b])) {
                            break;
                        }
                        if (sb == subInode[idx].i_blocks - 1) {
                            if (subInode[idx].i_blocks == 7) {
                                break;
                            }
                            subInode[idx].i_blocks++;
                            subInode[idx].i_block[sb+1] = new_block();
                        }
                    }
                    if (!memcmp(subInode+idx+1, allZero, sizeof(Inode))) {
                        if (idx == BLOCK_SIZE / sizeof(Inode) - 1) {
                            break;
                        }
                        subInode[idx+1] = *init_inode();
                        subInode[idx+1].i_blocks++;
                        subInode[idx+1].i_block[0] = new_block();
                    }
                }
                if (sub_block[bid+1] == 0) {
                    if (bid == BLOCK_SIZE / 16 - 1) {
                        break;
                    }
                    sub_block[bid+1] = new_block();
                }
                free(sub_block_entry);
            }
            update_block_entry(block_entry, inode->i_block[b]);
            free(block_entry);
        }
    }
    update_inode_table(inode, index);
    free(inode);
    free(allZero);
}
```
遍历每个数据块，调用append_dir_entry寻找合适的位置插入目录项。
```c
int append_dir_entry(DirEntry* dir_entry, uint16_t index) {
    uint16_t i = 0;
    uint8_t *block_entry = load_block_entry(index);
    DirEntry temp = ((DirEntry *)block_entry)[0];
    while (temp.inode > 0) {
        uint16_t rsize = temp.rec_len - temp.name_len - 7;
        if (rsize >= dir_entry->rec_len) {
            temp.rec_len = temp.name_len + 7;
            ((DirEntry *)(block_entry + i))[0].rec_len = temp.rec_len;
            dir_entry->rec_len = rsize;
            ((DirEntry *)(block_entry + i + temp.rec_len))[0].inode = dir_entry->inode;
            ((DirEntry *)(block_entry + i + temp.rec_len))[0].rec_len = dir_entry->rec_len;
            ((DirEntry *)(block_entry + i + temp.rec_len))[0].name_len = dir_entry->name_len;
            ((DirEntry *)(block_entry + i + temp.rec_len))[0].file_type = dir_entry->file_type;
            strcpy(((DirEntry *)(block_entry + i + temp.rec_len))[0].name, dir_entry->name);

            update_block_entry(block_entry, index);
            free(block_entry);
            return 1;
        }
        i += temp.rec_len;
        temp = ((DirEntry *)(block_entry + i))[0];
    }
    if (BLOCK_SIZE - i > dir_entry->rec_len) {
        ((DirEntry *)(block_entry + i))[0] = *dir_entry;
        update_block_entry(block_entry, index);
        free(block_entry);
        return 1;
    }
    free(block_entry);
    return 0;
}
```
在数据块内寻找合适的位置进行插入，具体使用rec_len进行判断。

### rmdir
删除目录。
- 测试
```shell
[root]$ mkdir yyy
[root]$ ls
File Name       File Mode   Size        Change Time               Access Time               Modification Time
.               drw-rw-rw-  38          Sun Dec 22 23:16:30 2024  Sun Dec 22 21:42:48 2024  Sun Dec 22 23:16:30 2024
..              drw-rw-rw-  38          Sun Dec 22 23:16:30 2024  Sun Dec 22 21:42:48 2024  Sun Dec 22 23:16:30 2024
1111            drw-rw-rw-  17          Sun Dec 22 21:51:47 2024  Sun Dec 22 21:51:47 2024  Sun Dec 22 21:51:47 2024
yyy             drw-rw-rw-  17          Sun Dec 22 23:16:30 2024  Sun Dec 22 23:16:30 2024  Sun Dec 22 23:16:30 2024
[root]$ rmdir yyy
[root]$ ls
File Name       File Mode   Size        Change Time               Access Time               Modification Time
.               drw-rw-rw-  28          Sun Dec 22 23:38:10 2024  Sun Dec 22 21:42:48 2024  Sun Dec 22 23:38:10 2024
..              drw-rw-rw-  28          Sun Dec 22 23:38:10 2024  Sun Dec 22 21:42:48 2024  Sun Dec 22 23:38:10 2024
1111            drw-rw-rw-  17          Sun Dec 22 21:51:47 2024  Sun Dec 22 21:51:47 2024  Sun Dec 22 21:51:47 2024
```
- 相关函数：
```c
void rmdir(char *name) {
    name[strlen(name)-1] = '\0';
    if (!strcmp(name, ".") || !strcmp(name, "..")) {
        printf("rmdir: failed to remove '%s': Invalid argument\n", name);
        return;
    }
    uint16_t inodeId = handle_search_inode(name, FT_DIR, current_dir, search_file_name);
    if (inodeId == 0) {
        printf("rmdir: failed to remove '%s': No such file or directory\n", name);
        return;
    }
    Inode *inode = load_inode_table(inodeId);
    if (inode->i_size != 17) {
        printf("rmdir: failed to remove '%s': Directory not empty\n", name);
        free(inode);
        return;
    }
    update_inode_table(inode, inodeId);
    free(inode);
    handle_read_inode(inodeId, reset_block_entry);
    inode = load_inode_table(inodeId);
    reset_inode_table(inodeId);
    GroupDesc *group_desc = load_group_desc();
    Inode *curnode = load_inode_table(current_dir);
    curnode->i_size -= 7 + strlen(name);
    update_inode_table(curnode, current_dir);
    free(curnode);
    group_desc->bg_free_blocks_count += inode->i_blocks;
    group_desc->bg_free_inodes_count++;
    group_desc->bg_used_dirs_count--;
    update_group_desc(group_desc);
    handle_search_inode(name, FT_DIR, current_dir, delete_dir_entry);
    free(group_desc);
    free(inode);
}
```
命令层函数，搜寻文件名是否有效，若有效，则释放每个数据块，释放inode，并调用handle_search_inode，delete_dir_entry作为参数，把当前目录项从当前目录的数据块中删除。

```c
uint16_t delete_dir_entry(char *name, FileType file_type, uint16_t index) {
    uint16_t i = 0;
    uint8_t *block_entry = load_block_entry(index);
    DirEntry temp = ((DirEntry *)block_entry)[0];
    DirEntry next = ((DirEntry *)(block_entry + temp.rec_len))[0];
    while (next.inode > 0) {
        uint16_t rsize = temp.rec_len - temp.name_len - 7;
        if (!strcmp(name, next.name) && next.file_type == file_type) {
            ((DirEntry *)(block_entry + i + temp.rec_len))[0].inode = 0;
            ((DirEntry *)(block_entry + i))[0].rec_len += next.rec_len;
            update_block_entry(block_entry, index);
            free(block_entry);
            return 1;
        }
        i += temp.rec_len;
        temp = ((DirEntry *)(block_entry + i))[0];
        next = ((DirEntry *)(block_entry + i + temp.rec_len))[0];
    }
    free(block_entry);
    return 0;
}
```
用于从单个数据块中寻找对应的目录项进行删除。

### create
用于创建普通文件

测试：
```shell
[root]$ create yyy
[root]$ ls
File Name       File Mode   Size        Change Time               Access Time               Modification Time
.               drw-rw-rw-  38          Sun Dec 22 23:43:57 2024  Sun Dec 22 21:42:48 2024  Sun Dec 22 23:43:57 2024
..              drw-rw-rw-  38          Sun Dec 22 23:43:57 2024  Sun Dec 22 21:42:48 2024  Sun Dec 22 23:43:57 2024
1111            drw-rw-rw-  17          Sun Dec 22 21:51:47 2024  Sun Dec 22 21:51:47 2024  Sun Dec 22 21:51:47 2024
yyy             -rwxrwxrwx  0           Sun Dec 22 23:43:57 2024  Sun Dec 22 23:43:57 2024  Sun Dec 22 23:43:57 2024
```

相关函数：
```c
void create(char *name) {
    name[strlen(name)-1] = '\0';
    uint16_t inodeId = handle_search_inode(name, FT_REG_FILE, current_dir, search_file_name);
    if (inodeId != 0) {
        printf("create: cannot create file '%s': File exists\n", name);
        return;
    }
    DirEntry *newDir = new_dir_entry(name, FT_REG_FILE);
    Inode *inode = load_inode_table(current_dir);
    inode->i_size += 7 + strlen(name);
    update_inode_table(inode, current_dir);
    handle_append_directory(newDir, current_dir);
    free(newDir);
    free(inode);
}
```
底层函数，生成一个新的目录项，和mkdir逻辑类似。

### delete
用于删除普通文件

- 测试：
```shell
[root]$ create yyy
[root]$ ls
File Name       File Mode   Size        Change Time               Access Time               Modification Time
.               drw-rw-rw-  38          Sun Dec 22 23:43:57 2024  Sun Dec 22 21:42:48 2024  Sun Dec 22 23:43:57 2024
..              drw-rw-rw-  38          Sun Dec 22 23:43:57 2024  Sun Dec 22 21:42:48 2024  Sun Dec 22 23:43:57 2024
1111            drw-rw-rw-  17          Sun Dec 22 21:51:47 2024  Sun Dec 22 21:51:47 2024  Sun Dec 22 21:51:47 2024
yyy             -rwxrwxrwx  0           Sun Dec 22 23:43:57 2024  Sun Dec 22 23:43:57 2024  Sun Dec 22 23:43:57 2024
[root]$ delete yyy
[root]$ ls
File Name       File Mode   Size        Change Time               Access Time               Modification Time
.               drw-rw-rw-  28          Sun Dec 22 23:46:05 2024  Sun Dec 22 21:42:48 2024  Sun Dec 22 23:46:05 2024
..              drw-rw-rw-  28          Sun Dec 22 23:46:05 2024  Sun Dec 22 21:42:48 2024  Sun Dec 22 23:46:05 2024
1111            drw-rw-rw-  17          Sun Dec 22 21:51:47 2024  Sun Dec 22 21:51:47 2024  Sun Dec 22 21:51:47 2024

```

- 相关函数：
```c
void delete(char *name) {
    name[strlen(name)-1] = '\0';
    uint16_t inodeId = handle_search_inode(name, FT_REG_FILE, current_dir, search_file_name);
    if (inodeId == 0) {
        printf("delete: failed to remove '%s': No such file or directory\n", name);
        return;
    }
    if (is_open(inodeId) == 1) {
        printf("delete: failed to remove '%s': File opened\n", name);
        return;
    }
    Inode *inode = load_inode_table(inodeId);
    update_inode_table(inode, inodeId);
    free(inode);
    handle_read_inode(inodeId, reset_block_entry);
    inode = load_inode_table(inodeId);
    reset_inode_table(inodeId);
    GroupDesc *group_desc = load_group_desc();
    group_desc->bg_free_blocks_count += inode->i_blocks;
    group_desc->bg_free_inodes_count++;
    Inode *curnode = load_inode_table(current_dir);
    curnode->i_size -= 7 + strlen(name);
    update_inode_table(curnode, current_dir);
    free(curnode);
    update_group_desc(group_desc);
    handle_search_inode(name, FT_REG_FILE, current_dir, delete_dir_entry);
    free(group_desc);
    free(inode);
}
```
命令层函数，实现逻辑与rmdir类似。

### open
打开文件

- 测试：
```shell
[root]$ create yyy
[root]$ read yyy
read: yyy: File not opened
[root]$ open yyy
[root]$ read yyy
```

- 相关代码：
```c
void open(char name[]) {
    name[strlen(name)-1] = '\0';
    uint16_t inodeId = handle_search_inode(name, FT_REG_FILE, current_dir, search_file_name);
    if (inodeId == 0) {
        printf("open: %s: No such file or directory\n", name);
        return;
    }

    if (is_open(inodeId) == 1) {
        printf("open: %s: file opened\n", name);
        return;
    }
    Inode *inode = load_inode_table(inodeId);
    uint16_t permission = inode->i_mode & 0x00ff;
    if (permission == 4 || permission == 6 || permission == 7) {
        inode->i_atime = time(NULL);
        update_inode_table(inode, inodeId);
        for (int i = 0; i < FOPEN_TABLE_MAX; i++) {
            if (fopen_table[i] == 0) {
                fopen_table[i] = inodeId;
                free(inode);
                return;
            }
        }
        printf("open: cannot open '%s': too much opened files\n", name);
    } else {
        printf("open: cannot open '%s': Permission denied\n", name);
    }
    free(inode);
}
```
命令层函数，首先检查文件名是否合法，若合法则将文件的inode序号加入fopen_table。

### close
关闭文件

- 测试
```shell
[root]$ delete yyy
delete: failed to remove 'yyy': File opened
[root]$ close yyy
[root]$ delete yyy
[root]$ ls
File Name       File Mode   Size        Change Time               Access Time               Modification Time
.               drw-rw-rw-  28          Sun Dec 22 23:51:12 2024  Sun Dec 22 21:42:48 2024  Sun Dec 22 23:51:12 2024
..              drw-rw-rw-  28          Sun Dec 22 23:51:12 2024  Sun Dec 22 21:42:48 2024  Sun Dec 22 23:51:12 2024
1111            drw-rw-rw-  17          Sun Dec 22 21:51:47 2024  Sun Dec 22 21:51:47 2024  Sun Dec 22 21:51:47 2024
```

- 相关函数
```c
void close(char name[]) {
    name[strlen(name)-1] = '\0';
    uint16_t inodeId = handle_search_inode(name, FT_REG_FILE, current_dir, search_file_name);
    if (inodeId == 0) {
        printf("close: %s: No such file or directory\n", name);
        return;
    }

    if (is_open(inodeId) == 0) {
        printf("close: %s: File not opened\n", name);
        return;
    }
    for (int i = 0; i < FOPEN_TABLE_MAX; i++) {
        if (fopen_table[i] == inodeId) {
            fopen_table[i] = 0;
            return;
        }
    }
}
```
首先检查文件是否合法并被打开，若打开则把fopen_table对应的项置0。

### write
写入文件。

- 测试
```shell
[root]$ open yyy
[root]$ write yyy
010101010101010101010101
111111111111111111111111
222222222222222222222222
333333333333333333333333
444444444444444444444444
555555555555555555555555
666666666666666666666666
777777777777777777777777
888888888888888888888888
999999999999999999999999
000000000000000000000000
AAAAAAAAAAAAAAAAAAAAAAAA
BBBBBBBBBBBBBBBBBBBBBBBB
CCCCCCCCCCCCCCCCCCCCCCCC
DDDDDDDDDDDDDDDDDDDDDDDD
EEEEEEEEEEEEEEEEEEEEEEEE
FFFFFFFFFFFFFFFFFFFFFFFF
^\
426B written
[root]$ read yyy
010101010101010101010101
111111111111111111111111
222222222222222222222222
333333333333333333333333
444444444444444444444444
555555555555555555555555
666666666666666666666666
777777777777777777777777
888888888888888888888888
999999999999999999999999
000000000000000000000000
AAAAAAAAAAAAAAAAAAAAAAAA
BBBBBBBBBBBBBBBBBBBBBBBB
CCCCCCCCCCCCCCCCCCCCCCCC
DDDDDDDDDDDDDDDDDDDDDDDD
EEEEEEEEEEEEEEEEEEEEEEEE
FFFFFFFFFFFFFFFFFFFFFFFF

```

- 相关函数：
```c
void write(char name[]) {
    name[strlen(name)-1] = '\0';
    uint16_t inodeId = handle_search_inode(name, FT_REG_FILE, current_dir, search_file_name);
    if (inodeId == 0) {
        printf("write: %s: No such file or directory\n", name);
        return;
    }

    if (is_open(inodeId) == 0) {
        printf("write: %s: File not opened\n", name);
        return;
    }
    Inode *inode = load_inode_table(inodeId);
    uint16_t permission = inode->i_mode & 0x00ff;
    if (permission == 2 || permission == 3 || permission == 6 || permission == 7) {
        handle_write_inode(inodeId);
    } else {
        printf("write: cannot write '%s': Permission denied\n", name);
    }
    free(inode);
}
```
命令层函数，首先检查文件名是否合法，然后检查文件是否打开，检查文件权限，若均满足则调用handle_write_inode函数进行写处理
```c
void handle_write_inode(uint16_t index) {
    uint8_t *allZero = malloc(sizeof(Inode));
    memset(allZero, 0, sizeof(allZero));
    Inode *inode = load_inode_table(index);
    
    inode->i_atime = time(NULL);
    inode->i_mtime = time(NULL);
    if (inode->i_blocks == 0) {
        uint16_t newBlock = new_block();
        inode->i_blocks++;
        inode->i_block[0] = newBlock;
    }
    char *buf = input();
    inode->i_size += strlen(buf) + 1;
    printf("%luB written\n", strlen(buf) + 1);
    for (int b = 0; b < inode->i_blocks; b++) {
        if (b < 6) {
            int written = try_write(inode->i_block[b], buf);
            if (strlen(buf) <= written) {
                break;
            }
            buf += written;
            if (b == inode->i_blocks - 1) {
                uint16_t block = new_block();
                inode->i_blocks++;
                inode->i_block[b+1] = block;
            }
        } else if (b == 6) {
            uint8_t *block_entry = load_block_entry(inode->i_block[b]);
            Inode *subInode = (Inode *) block_entry;
            if (!memcmp(subInode, allZero, sizeof(Inode))) {
                subInode[0] = *init_inode();
                subInode[0].i_blocks++;
                subInode[0].i_block[0] = new_block();
            }
            for (int idx = 0; !memcmp(subInode+idx, allZero, sizeof(Inode)); idx++) {
                for (int sb = 0; sb < subInode[idx].i_blocks; sb++) {
                    int written = try_write(subInode[0].i_block[sb+1], buf);
                    if (strlen(buf) <= written) {
                        break;
                    }
                    buf += written;
                    if (sb == subInode[idx].i_blocks - 1) {
                        if (subInode[idx].i_blocks == 7) {
                            break;
                        }
                        subInode[idx].i_blocks++;
                        subInode[idx].i_block[sb+1] = new_block();
                    }
                }
                if (!memcmp(subInode+idx+1, allZero, sizeof(Inode))) {
                    if (idx == BLOCK_SIZE / sizeof(Inode) - 1) {
                        break;
                    }
                    subInode[idx+1] = *init_inode();
                    subInode[idx+1].i_blocks++;
                    subInode[idx+1].i_block[0] = new_block();
                }
            }
            update_block_entry(block_entry, inode->i_block[b]);
            free(block_entry);
        } else if (b == 7) {
            uint8_t *block_entry = load_block_entry(inode->i_block[b]);
            uint16_t *sub_block = (uint16_t *) block_entry;
            for (int bid = 0; sub_block[bid] != 0; bid++) {
                uint8_t *sub_block_entry = load_block_entry(sub_block[bid]);
                Inode *subInode = (Inode *) sub_block_entry;
                for (int idx = 0; !memcmp(subInode+idx, allZero, sizeof(Inode)); idx++) {
                    for (int sb = 0; sb < subInode[idx].i_blocks; sb++) {
                        int written = try_write(subInode[0].i_block[sb+1], buf);
                        if (strlen(buf) <= written) {
                            break;
                        }
                        buf += written;
                        if (sb == subInode[idx].i_blocks - 1) {
                            if (subInode[idx].i_blocks == 7) {
                                break;
                            }
                            subInode[idx].i_blocks++;
                            subInode[idx].i_block[sb+1] = new_block();
                        }
                    }
                    if (!memcmp(subInode+idx+1, allZero, sizeof(Inode))) {
                        if (idx == BLOCK_SIZE / sizeof(Inode) - 1) {
                            break;
                        }
                        subInode[idx+1] = *init_inode();
                        subInode[idx+1].i_blocks++;
                        subInode[idx+1].i_block[0] = new_block();
                    }
                }
                if (sub_block[bid+1] == 0) {
                    if (bid == BLOCK_SIZE / 16 - 1) {
                        update_block_entry(sub_block_entry, sub_block[bid]);
                        free(sub_block_entry);
                        break;
                    }
                    sub_block[bid+1] = new_block();
                }
                update_block_entry(sub_block_entry, sub_block[bid]);
                free(sub_block_entry);
            }
            update_block_entry(block_entry, inode->i_block[b]);
            free(block_entry);
        }
    }
    update_inode_table(inode, index);
    free(inode);
    free(allZero);
    free(buf);
}
```
首先调用input函数进行输入，然后遍历每个数据块，调用try_write函数尝试进行写入，不断重复直到写入完成。

```c
int writing = 0;
void stop_write() {
    writing = 0;
}
char *input() {
    writing = 1;
    char *buf = malloc(1024);
    char c;
    int blen = 0;
    signal(SIGQUIT, stop_write);
    while (writing == 1 && (c = getchar()) != EOF) {
        buf[blen++] = c;
        if (blen >= 1024)
            break;
    }
    buf[blen-1] = '\0';
    return buf;
}
```
输入函数，采用了SIGQUIT信号进行退出。

```c
int try_write(uint16_t index, char *data_to_write) {
    char *block_entry = (char *)load_block_entry(index);
    if (strlen(block_entry) == BLOCK_SIZE-1) {
        free(block_entry);
        return 0;
    }
    int written = BLOCK_SIZE - 1 - strlen(block_entry);
    strncat(block_entry, data_to_write, written);
    update_block_entry(block_entry, index);
    free(block_entry);
    return written;
}
```
在单个数据块内进行写入的函数，如果写的数据的长度超过数据块可用空间大小则返回已经写入的长度。

### read
读取文件内容

- 测试：
```shell
[root]$ read yyy
010101010101010101010101
111111111111111111111111
222222222222222222222222
333333333333333333333333
444444444444444444444444
555555555555555555555555
666666666666666666666666
777777777777777777777777
888888888888888888888888
999999999999999999999999
000000000000000000000000
AAAAAAAAAAAAAAAAAAAAAAAA
BBBBBBBBBBBBBBBBBBBBBBBB
CCCCCCCCCCCCCCCCCCCCCCCC
DDDDDDDDDDDDDDDDDDDDDDDD
EEEEEEEEEEEEEEEEEEEEEEEE
FFFFFFFFFFFFFFFFFFFFFFFF

```

- 相关函数：
```c
void read(char name[]) {
    name[strlen(name)-1] = '\0';
    uint16_t inodeId = handle_search_inode(name, FT_REG_FILE, current_dir, search_file_name);
    if (inodeId == 0) {
        printf("read: %s: No such file or directory\n", name);
        return;
    }

    if (is_open(inodeId) == 0) {
        printf("read: %s: File not opened\n", name);
        return;
    }
    Inode *inode = load_inode_table(inodeId);
    uint16_t permission = inode->i_mode & 0x00ff;
    if (permission == 4 || permission == 5 || permission == 6 || permission == 7) {
        inode->i_atime = time(NULL);
        update_inode_table(inode, inodeId);
        handle_read_inode(inodeId, print_file_in_block);
    } else {
        printf("read: cannot read '%s': Permission denied\n", name);
    }
    free(inode);
}
```
命令层函数，检查文件名、检查文件是否打开、检查文件权限，均满足则更新访问时间并且调用handle_read_inode，调用print_file_in_block输出数据块内的文件内容。
```c
void print_file_in_block(uint16_t index) {
    uint8_t *block_entry = load_block_entry(index);
    for (int i = 0; block_entry[i] != 0; i++) {
        printf("%c", (char) block_entry[i]);
    }
    free(block_entry);
}
```
输出单个数据块内的内容。

### chmod
更改文件权限。
- 测试
```shell
[root]$ ls
File Name       File Mode   Size        Change Time               Access Time               Modification Time
.               drw-rw-rw-  38          Sun Dec 22 23:53:48 2024  Sun Dec 22 21:42:48 2024  Sun Dec 22 23:53:48 2024
..              drw-rw-rw-  38          Sun Dec 22 23:53:48 2024  Sun Dec 22 21:42:48 2024  Sun Dec 22 23:53:48 2024
1111            drw-rw-rw-  17          Sun Dec 22 21:51:47 2024  Sun Dec 22 21:51:47 2024  Sun Dec 22 21:51:47 2024
yyy             -rwxrwxrwx  426         Sun Dec 22 23:53:48 2024  Sun Dec 22 23:54:51 2024  Sun Dec 22 23:53:59 2024
[root]$ chmod -rwx yyy
[root]$ ls
File Name       File Mode   Size        Change Time               Access Time               Modification Time
.               drw-rw-rw-  38          Sun Dec 22 23:53:48 2024  Sun Dec 22 21:42:48 2024  Sun Dec 22 23:53:48 2024
..              drw-rw-rw-  38          Sun Dec 22 23:53:48 2024  Sun Dec 22 21:42:48 2024  Sun Dec 22 23:53:48 2024
1111            drw-rw-rw-  17          Sun Dec 22 21:51:47 2024  Sun Dec 22 21:51:47 2024  Sun Dec 22 21:51:47 2024
yyy             ----------  426         Mon Dec 23 00:05:55 2024  Sun Dec 22 23:54:51 2024  Sun Dec 22 23:53:59 2024
[root]$ chmod +wx yyy
[root]$ ls
File Name       File Mode   Size        Change Time               Access Time               Modification Time
.               drw-rw-rw-  38          Sun Dec 22 23:53:48 2024  Sun Dec 22 21:42:48 2024  Sun Dec 22 23:53:48 2024
..              drw-rw-rw-  38          Sun Dec 22 23:53:48 2024  Sun Dec 22 21:42:48 2024  Sun Dec 22 23:53:48 2024
1111            drw-rw-rw-  17          Sun Dec 22 21:51:47 2024  Sun Dec 22 21:51:47 2024  Sun Dec 22 21:51:47 2024
yyy             --wx-wx-wx  426         Mon Dec 23 00:06:03 2024  Sun Dec 22 23:54:51 2024  Sun Dec 22 23:53:59 2024
```

- 相关代码
```c
void chmod(char *command) {
    command[strlen(command)-1] = '\0';
    int permission = 0;
    int i = 1;
    for (i = 1; command[i] != ' '; i++) {
        if (command[i] == 'r') {
            permission += 4;
        }
        if (command[i] == 'w') {
            permission += 2;
        }
        if (command[i] == 'x') {
            permission += 1;
        }
    }
    char *name = command + i+1;
    uint16_t inodeId = handle_search_inode(name, FT_REG_FILE, current_dir, search_file_name);
    if (inodeId == 0) {
        printf("chmod: %s: No such file or directory\n", name);
        return;
    }
    Inode *inode = load_inode_table(inodeId);
    inode->i_ctime = time(NULL);
    switch(command[0]) {
        case '+':
            inode->i_mode |= permission;
            break;
        case '-':
            permission = ~permission;
            inode->i_mode &= permission;
            break;
        default:
            printf("chmod: Invalid mode\n");
            free(inode);
            return;
    }
    update_inode_table(inode, inodeId);
    free(inode);
}
```

首先检查文件是否合法，若合法则根据选项进行权限的增加和权限的去除。

### format
格式化命令。
- 测试：
```shell
[root]$ ls
File Name       File Mode   Size        Change Time               Access Time               Modification Time
.               drw-rw-rw-  38          Sun Dec 22 23:53:48 2024  Sun Dec 22 21:42:48 2024  Sun Dec 22 23:53:48 2024
..              drw-rw-rw-  38          Sun Dec 22 23:53:48 2024  Sun Dec 22 21:42:48 2024  Sun Dec 22 23:53:48 2024
1111            drw-rw-rw-  17          Sun Dec 22 21:51:47 2024  Sun Dec 22 21:51:47 2024  Sun Dec 22 21:51:47 2024
yyy             --wx-wx-wx  426         Mon Dec 23 00:06:03 2024  Sun Dec 22 23:54:51 2024  Sun Dec 22 23:53:59 2024
[root]$ format
Please set your password
123
Password Saved
Format succeeded
[root]$ ls
File Name       File Mode   Size        Change Time               Access Time               Modification Time
.               drw-rw-rw-  17          Mon Dec 23 00:07:50 2024  Mon Dec 23 00:07:50 2024  Mon Dec 23 00:07:50 2024
..              drw-rw-rw-  17          Mon Dec 23 00:07:50 2024  Mon Dec 23 00:07:50 2024  Mon Dec 23 00:07:50 2024
```
- 相关函数：
```c
void format() {
    initialize_memory();
    initialize_disk();
    printf("Format succeeded\n");
}
```
重新初始化程序内存和磁盘文件。

### quit
退出程序，break掉shell的循环即可

## 问题
### 在设计文件系统时数据结构遇到的挑战问题
- dir_entry的变长结构的实现很有挑战。
遍历时不能单纯的用+1进行遍历（若如此做会因为对齐无法正确遍历），而需要使用+rec_len字节；删除的时候无需把全部都置为0，只需将rec_len合并到前项并把inode置0即可，这样极大的加快了文件系统的效率。

- 组描述符较小，如何将后续字符均填充为1比较复杂。
最后通过查询，发现bg_pad就是组描述符的填充字段，故直接加长bg_pad即可。

### 实现底层函数时遇到的挑战问题
- 一级索引和二级索引遍历实现较为复杂，且较多函数都需要使用。
最后经过观察，发现遍历模式较为固定，故想到可以将函数地址作为参数进行调用，无需复制粘贴大段函数。

- 写入文件时的输入的终止符。
因为不希望限制字符的写入，所以想到了实验二的软中断，通过查询发现SIGQUIT信号适合此类操作，故采用Ctrl+\即SIGQUIT信号作为输入停止的标志。

### 调试代码时遇到的挑战问题
- 内存泄漏
因为代码实现采用了大量malloc分配堆内存，需要在生命周期结束的时候进行free，较难进行排查。
通过valgrind可以检查内存free状况，最终进行修正。
- 内存初始化问题
在采用valgrind调试的时候，发现执行初始化的时候valgrind提出警告：
==96523== Syscall param write(buf) points to uninitialised byte(s)
==96523==    at 0x497A887: write (write.c:26)
==96523==    by 0x48F0EEC: _IO_file_write@@GLIBC_2.2.5 (fileops.c:1180)
==96523==    by 0x48F17CF: new_do_write (fileops.c:448)
==96523==    by 0x48F17CF: _IO_new_file_xsputn (fileops.c:1254)
==96523==    by 0x48F17CF: _IO_file_xsputn@@GLIBC_2.2.5 (fileops.c:1196)
==96523==    by 0x48E5FD6: fwrite (iofwrite.c:39)
==96523==    by 0x1094D1: update_group_desc (in /mnt/c/Users/余凯越/Documents/test/lab3/3)
==96523==    by 0x10C4B2: initialize_disk (in /mnt/c/Users/余凯越/Documents/test/lab3/3)
==96523==    by 0x10D4C9: format (in /mnt/c/Users/余凯越/Documents/test/lab3/3)
==96523==    by 0x10D99A: shell (in /mnt/c/Users/余凯越/Documents/test/lab3/3)
==96523==    by 0x10DA93: main (in /mnt/c/Users/余凯越/Documents/test/lab3/3)
==96523==  Address 0x4a92f61 is 1 bytes inside a block of size 512 alloc'd
==96523==    at 0x4848899: malloc (in /usr/libexec/valgrind/vgpreload_memcheck-amd64-linux.so)
==96523==    by 0x109561: new_group_desc (in /mnt/c/Users/余凯越/Documents/test/lab3/3)
==96523==    by 0x10C49C: initialize_disk (in /mnt/c/Users/余凯越/Documents/test/lab3/3)
==96523==    by 0x10D4C9: format (in /mnt/c/Users/余凯越/Documents/test/lab3/3)
==96523==    by 0x10D99A: shell (in /mnt/c/Users/余凯越/Documents/test/lab3/3)
==96523==    by 0x10DA93: main (in /mnt/c/Users/余凯越/Documents/test/lab3/3)
==96523==
根据函数调用路径，发现是组描述符中的password的一些字节没有进行初始化。最后在strcpy前对组描述符的password属性进行置零即可。

- write函数不符合预期
之前实现的write函数不符合预期，发现写入的字符和读出的字符不一样。
采用xxd工具xxd FS.txt FS.hex可以查看当前磁盘文件的每个字节，检查哪里出错。
最后通过调试发现是strncat函数的第三个参数是目标字符串可以写入的字符，而不是最大容纳的字符，将其修正后write函数测试正常。




