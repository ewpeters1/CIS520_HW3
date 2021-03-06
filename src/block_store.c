#include <stdio.h>
#include <stdint.h>
#include "bitmap.h"
#include "block_store.h"
#include <string.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <errno.h>// for errno handling
#include <stdlib.h>// for fopen
#include <math.h> //for ceil
// include more if you need

// You might find this handy.  I put it around unused parameters, but you should
// remove it before you submit. Just allows things to compile initially.
#define UNUSED(x) (void)(x)

struct block
{
    unsigned char block[BLOCK_SIZE_BYTES];
};

struct block_store
{
        bitmap_t* bit_array;        //our FBM using a bitmap
        block_t* block_data;        //data within the blocks
};

block_store_t *block_store_create()
{
    block_store_t* blocks = malloc(sizeof(block_store_t));
    blocks->bit_array = bitmap_create(BLOCK_STORE_NUM_BLOCKS); //also equal to BITMAP_SIZE_BYTES * 8
    int bitmapBlock = 126;
    bitmap_set(blocks->bit_array, bitmapBlock); //set the 127th bit since that's where our bitmap array is stored/being used
    int bitmapRemaining = BITMAP_SIZE_BYTES - BLOCK_SIZE_BYTES;
    int i = 1;
    while (bitmapRemaining > 0) //if bitmap takes up more blocks
    {
        bitmap_set(blocks->bit_array, bitmapBlock + i); //set the bit for the next block
        bitmapRemaining -= BLOCK_SIZE_BYTES; //see if any bitmap data is remaining
        i++; //increment to find index of next block if necessary
    }
    blocks->block_data = malloc(sizeof(block_t) * BLOCK_STORE_NUM_BLOCKS); //allocate memory for our block data
    return blocks;
}

void block_store_destroy(block_store_t *const bs)
{
    if(!bs || !bs->bit_array) //if block store or its bitmap don't exist, return
        return;      
    bitmap_destroy(bs->bit_array); //destroy the bitmap
    free(bs); //free the allocated memory for block store
}

size_t block_store_allocate(block_store_t *const bs)
{
    if (bs == NULL || bs->bit_array == NULL) //error if block store or bitmap don't exist
    {
        return SIZE_MAX;
    }
    size_t bit_index = bitmap_ffz(bs->bit_array); //find index of first zero bit
    if (bit_index != SIZE_MAX) //if successfully found free block
    {
        bitmap_set(bs->bit_array, bit_index); //set bit to used
    }
    return bit_index;
}

bool block_store_request(block_store_t *const bs, const size_t block_id)
{
    if (bs == NULL || bs->bit_array == NULL || block_id > BLOCK_STORE_AVAIL_BLOCKS) //error if bs and bitmap don't exist, or if block_id is bigger than block store
    {
        return false;
    }
    bool used = bitmap_test(bs->bit_array, block_id); //see if block is used
    if (used) //return unsuccessful if used
    {
        return false;
    }
    bitmap_set(bs->bit_array, block_id); //on success set bit to used
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
    if (bs == NULL || bs->bit_array == NULL) //error on bs or bitmap not existing
    {
        return SIZE_MAX;
    }
    int i = ceil((double)BITMAP_SIZE_BYTES / BLOCK_SIZE_BYTES);
    printf("Number of bitmap blocks: %d", i);
    return bitmap_total_set(bs->bit_array) - i; //subtracting i since bitmap is always using one or more blocks
}

size_t block_store_get_free_blocks(const block_store_t *const bs)
{
    if (bs == NULL || bs->bit_array == NULL) //error on bs or bitmap not existing
    {
        return SIZE_MAX;
    }
    return BLOCK_STORE_NUM_BLOCKS - bitmap_total_set(bs->bit_array); //total blocks - used blocks
}

size_t block_store_get_total_blocks()
{
    return BLOCK_STORE_AVAIL_BLOCKS; //constant
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

    block_store_t *bs = (block_store_t *)malloc(sizeof(block_store_t)); //allocate memory for a block store
    
    void *buffer = (bitmap_t *)malloc(BITMAP_SIZE_BYTES * sizeof(char)); //create a buffer to read in bitmap data
    
    int r = read(fd, buffer, BITMAP_SIZE_BYTES); //read file info for bitmap
    
    if(r < 0)
        return NULL;
    
    bs->bit_array = bitmap_import(BITMAP_SIZE_BYTES, buffer); //put bitmap into bit_array
    if(!bs->bit_array)
        return NULL;
    free(buffer); //free allocated buffer memory

    if (bs->block_data) //if block_data present, free that memory
    {
        free(bs->block_data);
    }
    bs->block_data = malloc(sizeof(block_t) * BLOCK_STORE_NUM_BLOCKS); //make new memory for block data
    read(fd, bs->block_data, BLOCK_STORE_NUM_BYTES); //read in block data from file
    
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
    
    FILE *file = fopen(filename, "w+"); // clear or create file as needed
    int ernsv = errno;
    if(errno != 0){
      printf("Error during fopen: %i", ernsv);
      return 0;// error during bitmap write cycle?
    }
    
    // write bitmap with BITMAP_SIZE_BYTES
    int w = fwrite(bitmap_export(bs->bit_array), BITMAP_SIZE_BYTES, 1, file);
    ernsv = errno;
    if(errno != 0){
      printf("Error during block_data write: %i", ernsv);
      return 0;// error during bitmap write cycle?
    }
    
    //write block_data with BLOCK_STORE_NUM_BYTES
    w = fwrite(bs->block_data, BLOCK_SIZE_BYTES, BLOCK_STORE_NUM_BLOCKS, file);
    ernsv = errno;
    if(errno != 0){
      printf("Error during block_store write cycle: %i", ernsv);
      return 0;// error during bs write cycle?
    }
    
    // close file
    fclose(file);
    ernsv = errno;
    if(errno != 0){
      printf("Error during close: %i", ernsv);
      return 0;// error during close?
    }
    return w * BLOCK_SIZE_BYTES; //w holds the number of blocks in block store, so we multiply by BLOCK_SIZE_BYTES for total bytes
}