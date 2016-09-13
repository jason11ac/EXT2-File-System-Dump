/*
 *  super_block
 *
 *  A basic set of file system parameters
 *  the file system's super block. These
 *  are the nine fields to be reported in
 *  file 'super.csv'.
 */
struct super_block {

  unsigned int magic_number;
  unsigned int inode_total;
  unsigned int block_total;
  unsigned int block_size;
  int fragment_size;
  unsigned int blocks_per_group;
  unsigned int inodes_per_group;
  unsigned int fragments_per_group;
  unsigned int first_data_block;
  
};

/*
 *  group_descr
 *
 *  Information for each group descriptor
 *  in the file system. These are the six
 *  fields to be reported in file 'group.csv'.
 */
struct group_descr {

  unsigned int contained_blocks;
  unsigned int free_blocks_per_group;
  unsigned int free_inodes_per_group;
  unsigned int directories_per_group;
  unsigned int inode_bitmap_block;
  unsigned int block_bitmap_block;
  unsigned int inode_table_start_block;
  
};
