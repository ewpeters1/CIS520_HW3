#include <stdio.h>
#include <stdint.h>
#include "bitmap.h"
#include "block_store.h"
#include <string.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
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
        block_t* block_data;        //data within the blocks
};

block_store_t *block_store_create()
{
    block_store_t* blocks = malloc(sizeof(block_store_t));
    blocks->bit_array = bitmap_create(BLOCK_STORE_NUM_BLOCKS); //also equal to BITMAP_SIZE_BYTES * 8
    bitmap_set(blocks->bit_array, 126); //set the 127th bit since that's where our bitmap array is stored/being used
    blocks->block_data = malloc(sizeof(block_t) * BLOCK_STORE_NUM_BLOCKS);
    return blocks;
}

void block_store_destroy(block_store_t *const bs)
{
    if(!bs || !bs->bit_array) 
        return;      
    bitmap_destroy(bs->bit_array);
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
    if(bitmap_test(bs->bit_array, block_id) == 0)
        return;

    // clear bit
    bitmap_reset(bs->bit_array, block_id);

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

	///
	/// Reads data from the specified block and writes it to the designated buffer
	/// \param bs BS device
	/// \param block_id Source block id
	/// \param buffer Data buffer to write to
	/// \return Number of bytes read, 0 on error
	///
size_t block_store_read(const block_store_t *const bs, const size_t block_id, void *buffer)
{
    if(!bs || block_id > BLOCK_STORE_NUM_BYTES || !buffer)
        return 0;
    // copy bs block_data at block_id into buffer
    if(!memcpy(buffer, &(bs->block_data[block_id]), BLOCK_SIZE_BYTES))
        return 0;
    // return bytes read
    return BLOCK_SIZE_BYTES;
}

	///
	/// Reads data from the specified buffer and writes it to the designated block
	/// \param bs BS device
	/// \param block_id Destination block id
	/// \param buffer Data buffer to read from
	/// \return Number of bytes written, 0 on error
	///
size_t block_store_write(block_store_t *const bs, const size_t block_id, const void *buffer)
{
     // error check parameters
    if(!bs || block_id >= BLOCK_STORE_NUM_BYTES || !buffer)
        return 0;
    // copy buffer into bs block_data at block_id
    memcpy(&(bs->block_data[block_id]), buffer, BLOCK_SIZE_BYTES);
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
    
    bs->bit_array = bitmap_import(BITMAP_SIZE_BYTES, buffer);
    if(!bs->bit_array)
        return NULL;
    free(buffer);

    if (bs->block_data)
    {
        free(bs->block_data);
    }
    bs->block_data = malloc(sizeof(block_t) * BLOCK_STORE_NUM_BLOCKS); //removed double pointers on bs (giving errors) along with char* casting
    read(fd, &(bs->block_data), BLOCK_STORE_NUM_BLOCKS * BLOCK_SIZE_BYTES); //removed double pointers on bs (giving errors)
    
    int c = close(fd);
    
    if(c < 0)
        return NULL;
    
    return bs;
}
	///
	/// Writes the entirety of the BS device to file, overwriting it if it exists - for grads/bonus
	/// \param bs BS device
	/// \param filename The file to write to
	/// \return w number of bytes written, 0 on error
	///
size_t block_store_serialize(const block_store_t *const bs, const char *const filename)
{
    
    if (filename == NULL || bs == NULL) //could sub in !filename || !bs
    {
        return 0;
    }
    
    // open file
    int flags = O_WRONLY | O_CREAT | O_TRUNC; // see https://pubs.opengroup.org/onlinepubs/7908799/xsh/open.html
    int fil = open(filename, flags, 0666); //pathname, flags, mode(-rw-rw-rw-) https://ss64.com/bash/chmod.html
    if(fil < 0) return 0;
    
    //perform write
    int w = write(fil, bs, BLOCK_STORE_NUM_BYTES);
    if(w < 0) return 0;// error during write cycle
    
    // close file
    int f = close(fil);
    int respects = 0;
    if(f < 0) return respects; // pay respects to the error during close.
    
    return w; // theoretical number of bytes written. May include 0, in theory, but no less.
}