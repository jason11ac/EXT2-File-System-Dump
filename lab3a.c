#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lab3a.h"



struct super_block sb;                      /* Stores the values to be reported in 'super.csv'       */ 
const int SUPER_OFFSET            = 1024;   /* Offset of super block from beginning of file system.  */
const int I_CNT_OFFSET            = 0;      /* Rest are offsets from beginning of super block:       */
const int B_CNT_OFFSET            = 4;          /* Total number of blocks                            */
const int FIRST_DATA_BLOCK_OFFSET = 20;         /* Superblock's block number                         */
const int B_SIZ_OFFSET            = 24;         /* Block size                                        */
const int F_SIZ_OFFSET            = 28;         /* Fragment size                                     */
const int B_GRP_OFFSET            = 32;         /* Blocks per group                                  */
const int F_GRP_OFFSET            = 36;         /* Fragments per group                               */
const int I_GRP_OFFSET            = 40;         /* Inodes per group                                  */
const int MAGIC_OFFSET            = 56;         /* Magic number                                      */
int END_OF_SUPER;                           /* Computed once we have found block size for system     */

unsigned int DESCRIPTOR_COUNT = 0;          /* Total number of groups / group descriptors in system  */
struct group_descr *gd;                     /* Stores fields of the group descriptor                 */
                                            /* Rest are offsets from beginning of group descriptor:  */
const int B_FREE_OFFSET   = 12;                 /* Number of free blocks per group                   */
const int I_FREE_OFFSET   = 14;                 /* Number of free inodes per group                   */
const int D_USED_OFFSET   = 16;                 /* Number of directories per group                   */
const int I_BITMAP_OFFSET = 4;                  /* Block id of inode bitmap for a group              */
const int B_BITMAP_OFFSET = 0;                  /* Block id of block bitmap for a group              */
const int I_TABLE_OFFSET  = 8;                  /* Block id of inode table for a group               */
const int GROUP_DESC_SIZE = 32;                 /* Size of a block group descriptor                  */

                                           /* These are all offsets from start of inode table entry  */ 
const int I_MODE_OFFSET       = 0;
const int I_UID_OFFSET        = 2;
const int I_GID_OFFSET        = 24;
const int I_LINK_COUNT_OFFSET = 26;
const int I_CREATE_OFFSET     = 12;
const int I_MOD_OFFSET        = 16;
const int I_ACCESS_OFFSET     = 8;
const int I_SIZE_OFFSET       = 4;
const int I_BLOCK_OFFSET      = 28;
const int B_PTRS_OFFSET       = 40;

/* Computes offset into file system, based on the given block number / block ID.  */
int compute_offset(int block_num) {
  return (SUPER_OFFSET + (block_num - 1) * sb.block_size);
}


/* Analyze file system image and output to six csv files.  */
int main(int argc, char* argv[]) {

  int imageFD;        /* File descriptor for file system image we read in.  */

  if (argc < 2) {
    fprintf(stderr, "%s: name for file system image not provided\n", argv[0]);
    exit(-1);
  }

  /* Read in the provided file system image.  */
  if ((imageFD=open(argv[1],O_RDONLY)) == -1) {
    fprintf(stderr, "%s: %s\n", argv[1], strerror(errno));
    exit(-1);
  }


  ////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////
  //                                                                        //
  //                                 SUPER.CSV                              //
  //                                                                        //          
  ////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////

  /* First, create / open 'super.csv' for writing.  */
  FILE* super = fopen("super.csv", "w");
  if (super == NULL) {
    perror("fopen"); exit(-1);
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

  /* MAGIC NUMBER - HEX FORMAT  */
  unsigned char magic[2];
  int ret;
  ret = pread(imageFD, magic, 2, SUPER_OFFSET + MAGIC_OFFSET);
  if (ret == -1) {
    perror("pread"); exit(-1);
  }

  /* We have to do bit-shifting b/c values in file system are little endian */
  sb.magic_number = (magic[0]<<0) | (magic[1]<<8);
  fprintf(super, "%x,", sb.magic_number);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  
  /* TOTAL NUMBER OF INODES - DEC FORMAT  */
  unsigned char inode_tot[4];
  ret = pread(imageFD, inode_tot, 4, SUPER_OFFSET);
  if (ret == -1) {
    perror("pread"); exit(-1);
  }
  sb.inode_total = (inode_tot[0]<<0) | (inode_tot[1]<<8) | (inode_tot[2]<<16) | (inode_tot[3]<<24);
  fprintf(super, "%d,", sb.inode_total);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

  /* TOTAL NUMBER OF BLOCKS - DEC FORMAT  */
  unsigned char block_tot[4];
  ret = pread(imageFD, block_tot, 4, SUPER_OFFSET + B_CNT_OFFSET);
  if (ret == -1) {
    perror("pread"); exit(-1);
  }
  sb.block_total = (block_tot[0]<<0) | (block_tot[1]<<8) | (block_tot[2]<<16) | (block_tot[3]<<24);
  fprintf(super, "%d,", sb.block_total);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

  /* BLOCK SIZE - DEC FORMAT  */
  unsigned char block_siz[4];
  ret = pread(imageFD, block_siz, 4, SUPER_OFFSET + B_SIZ_OFFSET);
  if (ret == -1) {
    perror("pread"); exit(-1);
  }

  sb.block_size = (block_siz[0]<<0) | (block_siz[1]<<8) | (block_siz[2]<<16) | (block_siz[3]<<24);
  sb.block_size = 1024 << sb.block_size;
  fprintf(super, "%d,", sb.block_size);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

  /* FRAGMENT SIZE - DEC FORMAT  */
  unsigned char frag_siz[4];
  ret = pread(imageFD, frag_siz, 4, SUPER_OFFSET + F_SIZ_OFFSET);
  if (ret == -1) {
    perror("pread"); exit(-1);
  }

  sb.fragment_size = (frag_siz[0]<<0) | (frag_siz[1]<<8) | (frag_siz[2]<<16) | (frag_siz[3]<<24);
  if ( sb.fragment_size >= 0)
    sb.fragment_size = 1024 << sb.fragment_size;
  else
    sb.fragment_size = 1024 >> -sb.fragment_size;
  fprintf(super, "%d,", sb.fragment_size);
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

  /* BLOCKS PER GROUP - DEC FORMAT  */
  unsigned char bpg[4];
  ret = pread(imageFD, bpg, 4, SUPER_OFFSET + B_GRP_OFFSET);
  if (ret == -1) {
    perror("pread"); exit(-1);
  }

  sb.blocks_per_group = (bpg[0]<<0) | (bpg[1]<<8) | (bpg[2]<<16) | (bpg[3]<<24);
  fprintf(super, "%d,", sb.blocks_per_group);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

  /* INODES PER GROUP - DEC FORMAT  */
  unsigned char ipg[4];
  ret = pread(imageFD, ipg, 4, SUPER_OFFSET + I_GRP_OFFSET);
  if (ret == -1) {
    perror("pread"); exit(-1);
  }

  sb.inodes_per_group = (ipg[0]<<0) | (ipg[1]<<8) | (ipg[2]<<16) | (ipg[3]<<24);
  fprintf(super, "%d,", sb.inodes_per_group);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

  /* FRAGMENTS PER GROUP - DEC FORMAT  */
  unsigned char fpg[4];
  ret = pread(imageFD, fpg, 4, SUPER_OFFSET + F_GRP_OFFSET);
  if (ret == -1) {
    perror("pread"); exit(-1);
  }

  sb.fragments_per_group = (fpg[0]<<0) | (fpg[1]<<8) | (fpg[2]<<16) | (fpg[3]<<24);
  fprintf(super, "%d,", sb.fragments_per_group);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

  /* FIRST DATA BLOCK - DEC FORMAT  */
  unsigned char fdb[4];
  ret = pread(imageFD, fdb, 4, SUPER_OFFSET + FIRST_DATA_BLOCK_OFFSET);
  if (ret == -1) {
    perror("pread"); exit(-1);
  }

  sb.first_data_block = (fdb[0]<<0) | (fdb[1]<<8) | (fdb[2]<<16) | (fdb[3]<<24);
  fprintf(super, "%d\n", sb.first_data_block);

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

  /* Now that we know block size, compute the offset to end of super block  */
  END_OF_SUPER = SUPER_OFFSET + sb.block_size;

  /* Lastly, close the file stream.  */
  if (fclose(super) != 0) {
    perror("fclose"); exit(-1);
  }

  
  ////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////
  //                                                                        //
  //                                 GROUP.CSV                              //
  //                                                                        //          
  ////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////

  /* First, create / open 'group.csv' for writing.  */
  FILE* group = fopen("group.csv", "w");
  if (group == NULL) {
    perror("fopen"); exit(-1);
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

  /* Create a list of group descriptors in the file system.  */
  DESCRIPTOR_COUNT = 1 + (sb.block_total - 1) / sb.blocks_per_group;
  gd = (struct group_descr *) malloc( sizeof(struct group_descr) * DESCRIPTOR_COUNT );
  if (gd == NULL) {
    perror("malloc"); exit(-1);
  }

  unsigned char fbpg[2]; //Free blocks per group
  unsigned char fipg[2]; //Free inodes per group
  unsigned char dpg[2];  //Number of directories per group
  unsigned char ibm[4];  //Inode bitmap id
  unsigned char bbm[4];  //Block bitmap id
  unsigned char its[4];  //Inode table start block
  
  for (unsigned int i = 0; i < DESCRIPTOR_COUNT; i++) {

    /* NUMBER OF CONTAINED BLOCKS - DEC FORMAT  */
    if (i == (DESCRIPTOR_COUNT - 1)) //Last block
      gd[i].contained_blocks = sb.block_total % sb.blocks_per_group;
    else
      gd[i].contained_blocks = sb.blocks_per_group;
    fprintf(group, "%d,", gd[i].contained_blocks);

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

    /* NUMBER OF FREE BLOCKS - DEC FORMAT  */
    ret = pread(imageFD, fbpg, 2, ((END_OF_SUPER + (i*GROUP_DESC_SIZE)) + B_FREE_OFFSET));
    if (ret == -1) {
      perror("pread"); exit(-1);
    }
    gd[i].free_blocks_per_group = (fbpg[0]<<0) | (fbpg[1]<<8);
    fprintf(group, "%d,", gd[i].free_blocks_per_group);

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

    /* NUMBER OF FREE INODES - DEC FORMAT  */
    ret = pread(imageFD, fipg, 2, ((END_OF_SUPER + (i*GROUP_DESC_SIZE)) + I_FREE_OFFSET));
    if (ret == -1) {
      perror("pread"); exit(-1);
    }
    gd[i].free_inodes_per_group = (fipg[0]<<0) | (fipg[1]<<8);
    fprintf(group, "%d,", gd[i].free_inodes_per_group);

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

    /* NUMBER OF DIRECTORIES - DEC FORMAT  */
    ret = pread(imageFD, dpg, 2, ((END_OF_SUPER + (i*GROUP_DESC_SIZE)) + D_USED_OFFSET));
    if (ret == -1) {
      perror("pread"); exit(-1);
    }
    gd[i].directories_per_group = (dpg[0]<<0) | (dpg[1]<<8);
    fprintf(group, "%d,", gd[i].directories_per_group);    

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

    /* (FREE) INODE BITMAP BLOCK - HEX FORMAT  */
    ret = pread(imageFD, ibm, 4, ((END_OF_SUPER + (i*GROUP_DESC_SIZE)) + I_BITMAP_OFFSET));
    if (ret == -1) {
      perror("pread"); exit(-1);
    }
    gd[i].inode_bitmap_block = (ibm[0]<<0) | (ibm[1]<<8) | (ibm[2]<<16) | (ibm[3]<<24);
    fprintf(group, "%x,", gd[i].inode_bitmap_block);

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

    /* (FREE) BLOCK BITMAP BLOCK - HEX FORMAT  */
    ret = pread(imageFD, bbm, 4, ((END_OF_SUPER + (i*GROUP_DESC_SIZE)) + B_BITMAP_OFFSET));
    if (ret == -1) {
      perror("pread"); exit(-1);
    }
    gd[i].block_bitmap_block = (bbm[0]<<0) | (bbm[1]<<8) | (bbm[2]<<16) | (bbm[3]<<24);
    fprintf(group, "%x,", gd[i].block_bitmap_block);
    
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

    /* INODE TABLE (START) BLOCK - HEX FORMAT  */
    ret = pread(imageFD, its, 4, ((END_OF_SUPER + (i*GROUP_DESC_SIZE)) + I_TABLE_OFFSET));
    if (ret == -1) {
      perror("pread"); exit(-1);
    }
    gd[i].inode_table_start_block = (its[0]<<0) | (its[1]<<8) | (its[2]<<16) | (its[3]<<24);
    fprintf(group, "%x\n", gd[i].inode_table_start_block);
    
  }  

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

  /* Lastly, close the file stream.  */
  if (fclose(group) != 0) {
    perror("fclose"); exit(-1);
  }


  ////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////
  //                                                                        //
  //                                BITMAP.CSV                              //
  //                                                                        //          
  ////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////

  /* First, create / open 'bitmap.csv' for writing.  */
  FILE* bitmap = fopen("bitmap.csv", "w");
  if (bitmap == NULL) {
    perror("fopen"); exit(-1);
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

  /* Variables used for getting / traversing through bitmap.  */
  unsigned char bitmap_block[sb.block_size];
  int bitmap_byte, bit_free, counter;

  for (unsigned int i = 0; i < DESCRIPTOR_COUNT; i++) {   // For each group...

    /* First, read in the block bitmap and determine which blocks are free  */
    ret = pread(imageFD, bitmap_block, sb.block_size, compute_offset(gd[i].block_bitmap_block));
    if (ret == -1) {
      perror("pread"); exit(-1);
    }
    counter = 1;
    for (unsigned int j = 0; j < sb.block_size; j++) { //TODO: Change to contained blocks?
      bitmap_byte = bitmap_block[j];
      for (unsigned int k = 1; k <= 128; k *= 2) {
	bit_free = !(bitmap_byte & k);
	if (bit_free)
	  fprintf(bitmap, "%x,%d\n", gd[i].block_bitmap_block, (sb.blocks_per_group*i)+(8*j)+counter);
	counter++;
      }
      counter = 1;
    }

    /* Next, read in the inode bitmap and find free inodes  */
    ret = pread(imageFD, bitmap_block, sb.block_size, compute_offset(gd[i].inode_bitmap_block));
    if (ret == -1) {
      perror("pread"); exit(-1);
    }
    counter = 1;
    for (unsigned int j = 0; j < sb.block_size; j++) { //TODO: Change to contained blocks?
      bitmap_byte = bitmap_block[j];
      for (unsigned int k = 1; k <= 128; k *= 2) {
	bit_free = !(bitmap_byte & k);
	if (bit_free)
	  fprintf(bitmap, "%x,%d\n", gd[i].inode_bitmap_block, (sb.inodes_per_group*i)+(8*j)+counter);
	counter++;
      }
      counter = 1;
    }
  }
    
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

  /* Lastly, close the file stream.  */
  if (fclose(bitmap) != 0) {
    perror("fclose"); exit(-1);
  }


  ////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////
  //                                                                        //
  //                                 INODE.CSV                              //
  //                                                                        //          
  ////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////

  /* First, create / open 'inode.csv' for writing.  */
  FILE* inode = fopen("inode.csv", "w");
  if (inode == NULL) {
    perror("fopen"); exit(-1);
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

  unsigned int table_offset;
  unsigned int table_index;
  unsigned int inode_number;

  unsigned char imod[2];
  unsigned int i_mode;
  unsigned int file_type;
  

  unsigned char i_uid[2];
  unsigned int i_uidf;

  unsigned char i_gid[2];
  unsigned int i_gidf;

  unsigned char i_link_count[2];
  unsigned int i_lcf;

  unsigned char i_ctime[4];
  unsigned int i_ctimef;
  
  unsigned char i_mtime[4];
  unsigned int i_mtimef;

  unsigned char i_atime[4];
  unsigned int i_atimef;

  unsigned char i_size[4];
  unsigned int i_sizef;

  unsigned char i_b[4];
  unsigned int i_blocks;

  unsigned char b_ptr[4];
  unsigned int block_id;

  unsigned char inodes[128];
  
  for (unsigned int i =  0; i < DESCRIPTOR_COUNT; i++) {

    ret = pread(imageFD, bitmap_block, sb.block_size, compute_offset(gd[i].inode_bitmap_block));
    if (ret == -1) {
      perror("pread"); exit(-1);
    }

    counter = 1;
    
    for (unsigned int j = 0; j < sb.block_size; j++) {
      bitmap_byte = bitmap_block[j];
      for (unsigned int k = 1; k <= 128; k *= 2) {
	bit_free = !(bitmap_byte & k);
	if (!bit_free) {
	  table_index = (8*j) + counter;
	  if (table_index < sb.inodes_per_group) {


	    inode_number = (sb.inodes_per_group * i) + table_index;
	    table_offset = compute_offset(gd[i].inode_table_start_block) + (128*(table_index-1));

	    
	    /* INODE NUMBER - DEC FORMAT  */
	    fprintf(inode, "%d,", inode_number);

	    ret = pread(imageFD, inodes, 128, table_offset);
	    if (ret == -1) {
	      perror("pread"); exit(-1);
	    }

	    int l;

	    /* FILE TYPE - CHAR FORMAT */
	    for (l = 0; l < 2; l++) {
	      imod[l] = inodes[I_MODE_OFFSET+l];
	    }
	    i_mode = (imod[0]<<0) | (imod[1]<<8);
	    file_type = (i_mode & 0xF000);
	    switch (file_type) {
	      case 0x8000:
		fprintf(inode, "f,");
		break;
	      case 0x4000:
		fprintf(inode, "d,");
		break;
	      case 0xA000:
		fprintf(inode, "s,");
		break;
	      default:
		fprintf(inode, "?,");
		break;
	    }

	    /* MODE - OCT FORMAT */
	    fprintf(inode, "%o,", i_mode);
	      

	    /* OWNER - DEC FORMAT */
	    for (l = 0; l < 2; l++) {
	      i_uid[l] = inodes[I_UID_OFFSET+l];
	    }
	    i_uidf = (i_uid[0]<<0) | (i_uid[1]<<8);
	    fprintf(inode, "%d,", i_uidf);


	    /* GROUP - DEC FORMAT */
	    for (l = 0; l < 2; l++) {
	      i_gid[l] = inodes[I_GID_OFFSET+l];
	    }
	    i_gidf = (i_gid[0]<<0) | (i_gid[1]<<8);
	    fprintf(inode, "%d,", i_gidf);

	    /* LINK COUNT - DEC FORMAT */
	    for (l = 0; l < 2; l++) {
	      i_link_count[l] = inodes[I_LINK_COUNT_OFFSET+l];
	    }
	    i_lcf = (i_link_count[0]<<0) | (i_link_count[1]<<8);
	    fprintf(inode, "%d,", i_lcf);

	    /* CREATION TIME - HEX FORMAT */
	    for (l = 0; l < 4; l++) {
	      i_ctime[l] = inodes[I_CREATE_OFFSET+l];
	    }
	    i_ctimef = (i_ctime[0]<<0) | (i_ctime[1]<<8) | (i_ctime[2]<<16) | (i_ctime[3]<<24);
	    fprintf(inode, "%x,", i_ctimef);

	    /* MODIFICATION TIME - HEX FORMAT */
	    for (l = 0; l < 4; l++) {
	      i_mtime[l] = inodes[I_MOD_OFFSET+l];
	    }
	    i_mtimef = (i_mtime[0]<<0) | (i_mtime[1]<<8) | (i_mtime[2]<<16) | (i_mtime[3]<<24);
	    fprintf(inode, "%x,", i_mtimef);

	    /* ACCESS TIME - HEX FORMAT */
	    for (l = 0; l < 4; l++) {
	      i_atime[l] = inodes[I_ACCESS_OFFSET+l];
	    }
	    i_atimef = (i_atime[0]<<0) | (i_atime[1]<<8) | (i_atime[2]<<16) | (i_atime[3]<<24);
	    fprintf(inode, "%x,", i_atimef);

	    /* FILE SIZE - DEC FORMAT */
	    for (l = 0; l < 4; l++) {
	      i_size[l] = inodes[I_SIZE_OFFSET+l];
	    }
	    i_sizef = (i_size[0]<<0) | (i_size[1]<<8) | (i_size[2]<<16) | (i_size[3]<<24);
	    fprintf(inode, "%d,", i_sizef);

	    /* NUMBER OF BLOCKS - DEC FORMAT */
	    for (l = 0; l < 4; l++) {
	      i_b[l] = inodes[I_BLOCK_OFFSET+l];
	    }
	    i_blocks = (i_b[0]<<0) | (i_b[1]<<8) | (i_b[2]<<16) | (i_b[3]<<24);
	    i_blocks /= (2 << sb.block_size);
	    fprintf(inode, "%d,", i_blocks);

	    /* BLOCK POINTERS * 15 - HEX FORMAT */
	    for (unsigned i = 0; i < 15; i++) {
	      for (l = 0; l < 4; l++) {
		b_ptr[l] = inodes[B_PTRS_OFFSET+(4*i)+l];
	      }
	      block_id = (b_ptr[0]<<0) | (b_ptr[1]<<8) | (b_ptr[2]<<16) | (b_ptr[3]<<24);
	      if (i == 14)
		fprintf(inode, "%x\n", block_id);
	      else
		fprintf(inode, "%x,", block_id);
	    }
	  }
	}
	counter++;
      }
      counter = 1;
    }
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

  /* Lastly, close the file stream.  */
  if (fclose(inode) != 0) {
    perror("fclose"); exit(-1);
  }

  
  ////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////
  //                                                                        //
  //                                 DIRECTORY.CSV                          //
  //                                                                        //          
  ////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////

  /* First, create / open 'directory.csv' for writing.  */
  FILE* directory = fopen("directory.csv", "w");
  if (directory == NULL) {
    perror("fopen"); exit(-1);
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

  unsigned int block_offset;
  unsigned int entry_offset = 0;
  unsigned char inum[4];
  unsigned char recl[2];
  unsigned char i_block[60];
  unsigned char i_block_num[4];
  unsigned int i_block_numf;
  unsigned int in_num;
  unsigned int rec_ln;
  unsigned char entry_namel[1];
  unsigned int enamel;
  unsigned char name[255];
  
  for (unsigned int i =  0; i < DESCRIPTOR_COUNT; i++) {

    ret = pread(imageFD, bitmap_block, sb.block_size, compute_offset(gd[i].inode_bitmap_block));
    if (ret == -1) {
      perror("pread"); exit(-1);
    }

    counter = 1;
    
    for (unsigned int j = 0; j < sb.block_size; j++) {
      bitmap_byte = bitmap_block[j];
      for (unsigned int k = 1; k <= 128; k *= 2) {
	bit_free = !(bitmap_byte & k);
	if (!bit_free) {
	  table_index = (8*j) + counter;
	  if (table_index < sb.inodes_per_group) {
	    
	    inode_number = (sb.inodes_per_group * i) + table_index;
	    table_offset = compute_offset(gd[i].inode_table_start_block) + (128*(table_index-1));
	    
	    int l;
	    
	    ret = pread(imageFD, inodes, 128, table_offset);
	    if (ret == -1) {
	      perror("pread"); exit(-1);
	    }

	    ret = pread(imageFD, i_block, 60, table_offset + 40);
	    if (ret == -1) {
	      perror("pread"); exit(-1);
	    }
	    
	    for (l = 0; l < 2; l++) {
	      imod[l] = inodes[I_MODE_OFFSET+l];
	    }
	    i_mode = (imod[0]<<0) | (imod[1]<<8);
	    file_type = (i_mode & 0xF000);
	    
	    if (file_type == 0x4000) {

	      for (l = 0; l < 4; l++) {
		i_b[l] = inodes[I_BLOCK_OFFSET+l];
	      }
	      i_blocks = (i_b[0]<<0) | (i_b[1]<<8) | (i_b[2]<<16) | (i_b[3]<<24);
	      i_blocks /= (2 << sb.block_size);

	      
	      for (unsigned int i = 0; i < i_blocks; i++) {
		for (unsigned int l = 0; l < 4; l++) {
		  i_block_num[l] = i_block[4*i+l];
		}
		i_block_numf = (i_block_num[0]<<0) | (i_block_num[1]<<8) | (i_block_num[2]<<16) | (i_block_num[3]<<24);
		
		block_offset = compute_offset(i_block_numf);

		int count = 0;
	        while (entry_offset < sb.block_size) {
		  pread(imageFD, inum, 4, block_offset + entry_offset);
		  in_num = (inum[0]<<0) | (inum[1]<<8) | (inum[2]<<16) | (inum[3]<<24);

		  if (in_num) {

		    /* PARENT INODE NUMBER - DEC FORMAT */
		    fprintf(directory, "%d,", inode_number);

		    /* ENTRY NUMBER - DEC FORMAT */
		    fprintf(directory, "%d,", count);

		    /* ENTRY LENGTH - DEC FORMAT */
		    pread(imageFD, recl, 2, block_offset + entry_offset + 4);
		    rec_ln = (recl[0]<<0) | (recl[1]<<8) | (recl[2]<<16) | (recl[3]<<24);
		    fprintf(directory, "%d,", rec_ln);

		    pread(imageFD, entry_namel, 1, block_offset + entry_offset + 6);
		    enamel = (entry_namel[0]<<0);
		    fprintf(directory, "%d,", enamel);

		    /* INODE NUMBER OF THE FILE ENTRY - DEC FORMAT */
		    fprintf(directory, "%d,", in_num);

		    
		    /* NAME - STRING FORMAT */
		    fprintf(directory, "\"");
		    pread(imageFD, name, 255, block_offset + entry_offset + 8);
		    for (int h = 0; h < 255; h++) {
		      if (name[h] == '\0') {
			break;
		      }
		      fprintf(directory, "%c", name[h]);
		    }
		    fprintf(directory, "\"");
		    fprintf(directory, "\n");
		    
		    count++;
		  }
		  else {
		    pread(imageFD, recl, 2, block_offset + entry_offset + 4);
		    rec_ln = (recl[0]<<0) | (recl[1]<<8) | (recl[2]<<16) | (recl[3]<<24);
		  }
		  
		  entry_offset += rec_ln;
		}
		entry_offset = 0;
	      }
	    }
	  }
	}
	counter++;
      }
      counter = 1;
    }
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  
  /* Lastly, close the file stream.  */
  if (fclose(directory) != 0) {
    perror("fclose"); exit(-1);
  }

  
  exit(0);
}
