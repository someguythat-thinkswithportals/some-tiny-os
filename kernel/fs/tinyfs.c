#include "tinyfs.h"
#include "ata.h"
#include "../memory.h"

static tinyfs_superblock_t sb;

static int block_to_lba(uint32_t bnum) {
    return bnum * (TINYFS_BLOCK_SIZE / ATA_SECTOR_SIZE);
}

static int read_blocks(uint32_t bnum, uint8_t count, void* buf) {
    return ata_read_sectors(block_to_lba(bnum), count * (TINYFS_BLOCK_SIZE / ATA_SECTOR_SIZE), buf);
}

static int write_blocks(uint32_t bnum, uint8_t count, const void* buf) {
    return ata_write_sectors(block_to_lba(bnum), count * (TINYFS_BLOCK_SIZE / ATA_SECTOR_SIZE), buf);
}

static int bitmap_test(const uint8_t* bitmap, uint32_t bit) {
    return (bitmap[bit / 8] >> (bit % 8)) & 1;
}

int tinyfs_init(void) {
    uint8_t* buf = (uint8_t*)memory_alloc(TINYFS_BLOCK_SIZE);
    if (!buf) return -1;

    if (read_blocks(TINYFS_SUPER_BLOCK, 1, buf) < 0) {
        memory_free(buf);
        return -1;
    }

    tinyfs_superblock_t* sp = (tinyfs_superblock_t*)buf;
    if (sp->magic != TINYFS_MAGIC) {
        memory_free(buf);
        return -1;
    }

    sb = *sp;
    memory_free(buf);
    return 0;
}

int tinyfs_read_inode(uint32_t inum, tinyfs_inode_t* inode) {
    if (inum >= sb.inode_count) return -1;

    uint32_t block = sb.inode_table_block + inum / (TINYFS_BLOCK_SIZE / sizeof(tinyfs_inode_t));
    uint32_t offset = inum % (TINYFS_BLOCK_SIZE / sizeof(tinyfs_inode_t));

    uint8_t* buf = (uint8_t*)memory_alloc(TINYFS_BLOCK_SIZE);
    if (!buf) return -1;

    if (read_blocks(block, 1, buf) < 0) {
        memory_free(buf);
        return -1;
    }

    tinyfs_inode_t* table = (tinyfs_inode_t*)buf;
    *inode = table[offset];
    memory_free(buf);
    return 0;
}

int tinyfs_write_inode(uint32_t inum, const tinyfs_inode_t* inode) {
    if (inum >= sb.inode_count) return -1;

    uint32_t block = sb.inode_table_block + inum / (TINYFS_BLOCK_SIZE / sizeof(tinyfs_inode_t));
    uint32_t offset = inum % (TINYFS_BLOCK_SIZE / sizeof(tinyfs_inode_t));

    uint8_t* buf = (uint8_t*)memory_alloc(TINYFS_BLOCK_SIZE);
    if (!buf) return -1;

    if (read_blocks(block, 1, buf) < 0) {
        memory_free(buf);
        return -1;
    }

    tinyfs_inode_t* table = (tinyfs_inode_t*)buf;
    table[offset] = *inode;

    int ret = write_blocks(block, 1, buf);
    memory_free(buf);
    return ret;
}

int tinyfs_read_block(uint32_t bnum, void* buf) {
    if (bnum >= sb.total_blocks) return -1;
    return read_blocks(bnum, 1, buf);
}

int tinyfs_write_block(uint32_t bnum, const void* buf) {
    if (bnum >= sb.total_blocks) return -1;
    return write_blocks(bnum, 1, buf);
}

int tinyfs_block_alloc(void) {
    uint8_t* bitmap = (uint8_t*)memory_alloc(TINYFS_BLOCK_SIZE);
    if (!bitmap) return -1;

    if (read_blocks(sb.block_bitmap_block, 1, bitmap) < 0) {
        memory_free(bitmap);
        return -1;
    }

    for (uint32_t i = 0; i < sb.total_blocks; i++) {
        if (!bitmap_test(bitmap, i)) {
            bitmap[i / 8] |= (1 << (i % 8));
            write_blocks(sb.block_bitmap_block, 1, bitmap);
            memory_free(bitmap);
            return i;
        }
    }

    memory_free(bitmap);
    return -1;
}

void tinyfs_block_free(uint32_t bnum) {
    uint8_t* bitmap = (uint8_t*)memory_alloc(TINYFS_BLOCK_SIZE);
    if (!bitmap) return;

    if (read_blocks(sb.block_bitmap_block, 1, bitmap) < 0) {
        memory_free(bitmap);
        return;
    }

    bitmap[bnum / 8] &= ~(1 << (bnum % 8));
    write_blocks(sb.block_bitmap_block, 1, bitmap);
    memory_free(bitmap);
}

int tinyfs_inode_alloc(void) {
    uint8_t* bitmap = (uint8_t*)memory_alloc(TINYFS_BLOCK_SIZE);
    if (!bitmap) return -1;

    if (read_blocks(sb.inode_bitmap_block, 1, bitmap) < 0) {
        memory_free(bitmap);
        return -1;
    }

    for (uint32_t i = 0; i < sb.inode_count; i++) {
        if (!bitmap_test(bitmap, i)) {
            bitmap[i / 8] |= (1 << (i % 8));
            write_blocks(sb.inode_bitmap_block, 1, bitmap);
            memory_free(bitmap);

            tinyfs_inode_t inode;
            memory_set(&inode, 0, sizeof(inode));
            inode.refcount = 1;
            tinyfs_write_inode(i, &inode);

            return i;
        }
    }

    memory_free(bitmap);
    return -1;
}

void tinyfs_inode_free(uint32_t inum) {
    if (inum >= sb.inode_count) return;

    uint8_t* bitmap = (uint8_t*)memory_alloc(TINYFS_BLOCK_SIZE);
    if (!bitmap) return;

    if (read_blocks(sb.inode_bitmap_block, 1, bitmap) < 0) {
        memory_free(bitmap);
        return;
    }

    bitmap[inum / 8] &= ~(1 << (inum % 8));
    write_blocks(sb.inode_bitmap_block, 1, bitmap);
    memory_free(bitmap);
}

static int name_match(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

int tinyfs_add_dirent(uint32_t dir_inum, uint32_t ent_inum, const char* name) {
    tinyfs_inode_t dir_inode;
    if (tinyfs_read_inode(dir_inum, &dir_inode) < 0) return -1;
    if (!(dir_inode.mode & TINYFS_TYPE_DIR)) return -1;

    if (dir_inode.direct_block == 0) {
        int block = tinyfs_block_alloc();
        if (block < 0) return -1;
        dir_inode.direct_block = block;
        dir_inode.size = 0;
    }

    uint8_t* block = (uint8_t*)memory_alloc(TINYFS_BLOCK_SIZE);
    if (!block) return -1;

    if (tinyfs_read_block(dir_inode.direct_block, block) < 0) {
        memory_free(block);
        return -1;
    }

    tinyfs_dirent_t* entries = (tinyfs_dirent_t*)block;
    int found = -1;

    for (int i = 0; i < TINYFS_DIRENTS_PER_BLOCK; i++) {
        if (entries[i].inode != 0) continue;
        found = i;
        break;
    }

    if (found < 0) {
        memory_free(block);
        return -2;
    }

    entries[found].inode = ent_inum;
    int j;
    for (j = 0; name[j] && j < 27; j++) {
        entries[found].name[j] = name[j];
    }
    entries[found].name[j] = 0;

    int new_size = (found + 1) * sizeof(tinyfs_dirent_t);
    if (new_size > (int)dir_inode.size) {
        dir_inode.size = new_size;
    }

    if (tinyfs_write_block(dir_inode.direct_block, block) < 0) {
        memory_free(block);
        return -1;
    }
    memory_free(block);

    if (tinyfs_write_inode(dir_inum, &dir_inode) < 0) return -1;
    return 0;
}

int tinyfs_remove_dirent(uint32_t dir_inum, const char* name) {
    tinyfs_inode_t dir_inode;
    if (tinyfs_read_inode(dir_inum, &dir_inode) < 0) return -1;
    if (!(dir_inode.mode & TINYFS_TYPE_DIR)) return -1;
    if (dir_inode.direct_block == 0) return -1;

    uint8_t* block = (uint8_t*)memory_alloc(TINYFS_BLOCK_SIZE);
    if (!block) return -1;

    if (tinyfs_read_block(dir_inode.direct_block, block) < 0) {
        memory_free(block);
        return -1;
    }

    tinyfs_dirent_t* entries = (tinyfs_dirent_t*)block;
    int found = -1;

    for (int i = 0; i < TINYFS_DIRENTS_PER_BLOCK; i++) {
        if (entries[i].inode == 0) continue;
        if (name_match(name, entries[i].name) == 0) {
            found = i;
            break;
        }
    }

    if (found < 0) {
        memory_free(block);
        return -1;
    }

    entries[found].inode = 0;
    entries[found].name[0] = 0;

    if (tinyfs_write_block(dir_inode.direct_block, block) < 0) {
        memory_free(block);
        return -1;
    }
    memory_free(block);

    if (tinyfs_write_inode(dir_inum, &dir_inode) < 0) return -1;
    return 0;
}
