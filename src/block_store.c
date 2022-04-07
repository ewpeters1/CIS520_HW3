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
    if(!bs || !bs->bitmap) 
        return;      
    bitmap_destroy(bs->bitmap);
    free(bs);       
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
    if (bs == NULL || bs->bit_array == NULL || block_id > BLOCK_STORE_AVAIL_BLOCKS)
    {
        return false;
    }
    bool used = bitmap_test(bs->bit_array, block_id);
    if (used)
    {
        return false;
    }
    bitmap_set(bs->bit_array, block_id);
    return true;
}

void block_store_release(block_store_t *const bs, const size_t block_id)
{
    if(!bs || block_id >= BLOCK_STORE_NUM_BLOCKS)
        return;

    // check if bit is already cleared
    if(bitmap_test(bs->bitmap, block_id) == 0)
        return;

    // clear bit
    bitmap_reset(bs->bitmap, block_id);

    return;
}

size_t block_store_get_used_blocks(const block_store_t *const bs)
{
    if (bs == NULL || bs->bit_array == NULL)
    {
        return SIZE_MAX;
    }
    return bitmap_total_set(bs->bit_array) - 1; //subtracting one since bitmap is always using a block
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
    if(!bs || block_id > BLOCK_STORE_NUM_BYTES || !buffer)
        return 0;
    // copy bs block data at block id into buffer
    if(!memcpy(buffer, bs->block_data[block_id], BLOCK_SIZE_BYTES))
        return 0;
    // return bytes read
    return BLOCK_SIZE_BYTES;
}

size_t block_store_write(block_store_t *const bs, const size_t block_id, const void *buffer)
{
     // error check parameters
    if(!bs || block_id >= BLOCK_STORE_NUM_BYTES || !buffer)
        return 0;
    // copy buffer into bs block data at block id
    memcpy(bs->block_data[block_id], buffer, BLOCK_SIZE_BYTES);
    // return bytes written
    return BLOCK_SIZE_BYTES;
}

block_store_t *block_store_deserialize(const char *const filename)
{
    if(!filename)
        return NULL;

    // open file to read
    int fd = open(filename, O_RDONLY);
    
    if(fd < 0)
        return NULL;

    block_store_t *bs = (block_store_t *)malloc(sizeof(block_store_t));
    
    void *buffer = (bitmap_t *)malloc(BITMAP_SIZE_BYTES * sizeof(char));
    
    int r = read(fd, buffer, BITMAP_SIZE_BYTES);
    
    if(r < 0)
        return NULL;
    
    bs->bitmap = bitmap_import(BITMAP_SIZE_BYTES, buffer);
    if(!bs->bitmap)
        return NULL;
    free(buffer);

    
    **(bs)->block_data = (char *)malloc(BLOCK_STORE_AVAIL_BLOCKS * BLOCK_STORE_NUM_BLOCKS * sizeof(char));
    read(fd, **bs->block_data, BLOCK_STORE_NUM_BLOCKS * BLOCK_STORE_AVAIL_BLOCKS);
    
    int c = close(fd);
    
    if(c < 0)
        return NULL;
    
    return bs;
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