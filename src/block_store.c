#include <stdio.h>
#include <stdint.h>
#include "bitmap.h"
#include "block_store.h"
// include more if you need

// You might find this handy.  I put it around unused parameters, but you should
// remove it before you submit. Just allows things to compile initially.
#define UNUSED(x) (void)(x)

struct block //Chandra suggested making this struct in Friday's class
{
    unsigned char block[BLOCK_SIZE_BYTES];
};

struct block_store
{
        /*uint32_t bitmap_size;       //size of bitmap in bytes
        uint32_t num_blocks;        //number of blocks in block store
        uint32_t avail_blocks;      //number of AVAILABLE blocks in block store
        uint32_t block_size_bytes;	//size of block in bytes
        uint32_t block_size_bits;	//size of block in bits
        uint32_t total_bytes;       //total number of bytes in whole block store*/
        //Probably don't need above fields since they're already macros?
        
        bitmap_t* bit_array;        //our FBM using a bitmap
        block_t blocks[BLOCK_STORE_NUM_BLOCKS];
};

block_store_t *block_store_create()
{
    block_store_t* blocks = malloc(sizeof(block_store_t));
    blocks->bit_array = bitmap_create(BLOCK_STORE_NUM_BLOCKS); //also equal to BITMAP_SIZE_BYTES * 8
    bitmap_set(blocks->bit_array, 126); //set the 127th bit since that's where our bitmap array is stored/being used
    return blocks;
}

void block_store_destroy(block_store_t *const bs)
{
    if (bs == NULL)
    {
        return;
    }
    UNUSED(bs);
}
size_t block_store_allocate(block_store_t *const bs)
{
    if (bs == NULL || bs->bit_array == NULL)
    {
        return SIZE_MAX;
    }
    size_t bit_index = bitmap_ffz(bs->bit_array);
    if (bit_index != SIZE_MAX)
    {
        bitmap_set(bs->bit_array, bit_index);
    }
    return bit_index;
}

bool block_store_request(block_store_t *const bs, const size_t block_id)
{
    if (bs == NULL || block_id > SIZE_MAX)
    {
        return false;
    }
    UNUSED(bs);
    UNUSED(block_id);
    return false;
}

void block_store_release(block_store_t *const bs, const size_t block_id)
{
    if (bs == NULL || block_id > SIZE_MAX)
    {
        return;
    }
    UNUSED(bs);
    UNUSED(block_id);
}

size_t block_store_get_used_blocks(const block_store_t *const bs)
{
    if (bs == NULL || bs->bit_array == NULL)
    {
        return SIZE_MAX;
    }
    return bitmap_total_set(bs->bit_array); //might need to subtract one since bitmap is always using a block?
}

size_t block_store_get_free_blocks(const block_store_t *const bs)
{
    if (bs == NULL || bs->bit_array == NULL)
    {
        return SIZE_MAX;
    }
    return BLOCK_STORE_NUM_BLOCKS - bitmap_total_set(bs->bit_array);
}

size_t block_store_get_total_blocks()
{
    return BLOCK_STORE_AVAIL_BLOCKS;
}

size_t block_store_read(const block_store_t *const bs, const size_t block_id, void *buffer)
{
    if (bs == NULL || block_id > SIZE_MAX || buffer == NULL)
    {
        return 0;
    }
    UNUSED(bs);
    UNUSED(block_id);
    UNUSED(buffer);
    return 0;
}

size_t block_store_write(block_store_t *const bs, const size_t block_id, const void *buffer)
{
    if (bs == NULL || block_id > SIZE_MAX || buffer == NULL)
    {
        return 0;
    }
    UNUSED(bs);
    UNUSED(block_id);
    UNUSED(buffer);
    return 0;
}

block_store_t *block_store_deserialize(const char *const filename)
{
    if (filename == NULL)
    {
        return NULL;
    }
    UNUSED(filename);
    return NULL;
}

size_t block_store_serialize(const block_store_t *const bs, const char *const filename)
{
    if (filename == NULL || bs == NULL)
    {
        return 0;
    }
    UNUSED(bs);
    UNUSED(filename);
    return 0;
}