#!/usr/bin/python
import csv
import sys


# TODO: List sorting?
# TODO: Indirect blocks / read in from 'indirect.csv'?
# TODO: (possible): Possible adjustments for errors #1, 2, 3, 7
# TODO: Error checking?


#################################################################################
#################################################################################
##                                                                             ##
##                              CLASS DEFINITIONS                              ##
##                                                                             ##
#################################################################################
#################################################################################

# Contains basic file system parameters.
# 'fields' is a list of strings obtained by reading a row from the csv file.
class superBlock:
    def __init__(self, fields):
        self.magicNum          = int(fields[0], 16)  # Base 16 for converting hex
        self.inodeCount        = int(fields[1])     # Default base 10 for decimal
        self.blockCount        = int(fields[2])
        self.blockSize         = int(fields[3])
        self.fragmentSize      = int(fields[4])
        self.blocksPerGroup    = int(fields[5])
        self.inodesPerGroup    = int(fields[6])
        self.fragmentsPerGroup = int(fields[7])
        self.firstDataBlock    = int(fields[8])


# Describes an inode, its data blocks, and where / how it is referenced.
# 'fields' is a list of strings obtained by reading a row from the csv file.
class Inode:
    def __init__(self, fields):
        self.inodeNumber = int(fields[0])
        self.linkCount   = int(fields[5])
        self.referenceList = []
        self.blockPointers = []
        for column in range(11, 26):
            self.blockPointers.append(int(fields[column], 16))

    def add_ref(self, ref):
        self.referenceList.append(ref)

        
# Helps us keep track of which directory inodes / entry numbers refer to an inode
class inodeRef:
    def __init__(self, parentInode, entryNumber):
        self.parentInode = parentInode
        self.entryNumber = entryNumber


# Describes a block and where / how it is referenced.
class Block:
    def __init__(self, number):
        self.blockNumber   = number
        self.referenceList = []

    def add_ref(self, ref):
        self.referenceList.append(ref)


# Helps us keep track of which inodes / entry numbers refer to a data block
class blockRef:
    def __init__(self, inodeNumber, entryNumber, indirectNumber=None):
        self.inodeNumber = inodeNumber
        self.entryNumber = entryNumber
        if indirectNumber is None:
            self.indirectNumber = -1
        else:
            self.indirectNumber = indirectNumber


# Describes the parent and child inode numbers of a directory entry, along with
# the entry number and file (entry) name.
class Entry:
    def __init__(self, fields):
        self.parentNumber = int(fields[0])
        self.entryNumber  = int(fields[1])
        self.childNumber  = int(fields[4])
        self.entryName    = fields[5]


#################################################################################
#################################################################################
##                                                                             ##
##                                   SUBROUTINES                               ##
##                                                                             ##
#################################################################################
#################################################################################

# TODO: Possibly incomplete (b/c i'm not sure about indirect blocks)
def UNALLOCATED_BLOCK_ERROR(block):
    refList   = block.referenceList
    refCount  = len(refList)
    refString = ""
    
    for ref in range(0, refCount):
        refString = refString + " INODE < " + str(refList[ref].inodeNumber) + " > "
        if refList[ref].indirectNumber is not -1:
            refString = refString + "INDIRECT BLOCK < " + str(refList[ref].indirectNumber) + " > "
        refString = refString + "ENTRY < " + str(refList[ref].entryNumber) + " >"

    print 'UNALLOCATED BLOCK < {} > REFERENCED BY{}'.format(block.blockNumber, refString)


# TODO: Possibly incomplete (b/c i'm not sure about indirect blocks)
def MULTIPLY_REFERENCED_BLOCK_ERROR(block):
    refList   = block.referenceList
    refCount  = len(refList)
    refString = ""

    for ref in range(0, refCount):
        refString = refString + " INODE < " +  str(refList[ref].inodeNumber) + " > "
        if refList[ref].indirectNumber is not -1:
            refString = refString + "INDIRECT BLOCK < " + str(refList[ref].indirectNumber) + " > "
        refString = refString + "ENTRY < " + str(refList[ref].entryNumber) + " >"

    print 'MULTIPLY REFERENCED BLOCK < {} > BY{}'.format(block.blockNumber, refString)


# TODO: Possibly incomplete (b/c i think we might need a list of references somehow)
def UNALLOCATED_INODE_ERROR(entry):
    refString = ""

    for ref in range(0,1):       # TODO: Do we need list of references?  <---- Yes
        refString = refString + "DIRECTORY < " + str(entry.parentNumber) + " > "
        refString = refString + "ENTRY < " + str(entry.entryNumber) + " >"

    print 'UNALLOCATED INODE < {} > REFERENCED BY {}'.format(entry.childNumber, refString)


# ERROR 4 --- Report inodes that are absent from the free list and not referenced.
def MISSING_INODE_ERROR(inodeNumber, freeList):
    print 'MISSING INODE < {} > SHOULD BE IN FREE LIST < {} >'.format(inodeNumber, freeList)


# ERROR 5 --- Report inodes with link counts that are inconsistent with the actual
#             number of references made to the inode.
def INCORRECT_LINK_COUNT_ERROR(inodeNumber, badCount, goodCount):
    print 'LINKCOUNT < {} > IS < {} > SHOULD BE < {} >'.format(inodeNumber, badCount, goodCount)


# ERROR 6 --- For reporting when '.' and '..' entries don't link to correct inode.
def INCORRECT_ENTRY_ERROR(parentInode, entryName, badLink, correctLink):
    print 'INCORRECT ENTRY IN < {} > NAME < {} > LINK TO < {} > SHOULD BE < {} >'.format(parentInode, entryName, badLink, correctLink)


#################################################################################
#################################################################################
##                                                                             ##
##                                       MAIN                                  ##
##                                                                             ##
#################################################################################
#################################################################################

def main():

    numFreeBlocks    = []  # List of number of free inodes for each group
    numFreeInodes    = []  # List of number of free blocks for each group
    
    inodeBitmaps     = []  # List of inode numbers for inode bitmaps
    blockBitmaps     = []  # List of block numbers for block bitmaps 

    freeInodes       = []  # List of inode numbers for free inodes
    freeBlocks       = []  # List of block numbers for free blocks

    directoryEntries = []  # List of all directory entries read in from csv file

    allocatedInodes = {}   # Dictionary of all allocated inodes
    allocatedBlocks = {}   # Dictionary of all allocated blocks
    parentOf        = {}   # Associates a child inode (the key) to its parent

    
    # TODO: Put all output into 'lab3b_check.txt'. This means modifying the print
    #       functions above so that they dont just 'print' to the screen


    # Open 'super.csv' and read in the basic file system parameters
    with open('super.csv', 'r') as super:
        reader = csv.reader(super)
        row    = next(reader)
        sb     = superBlock(row)


    # Open 'group.csv' and use the data to construct a few of our lists
    with open('group.csv', 'r') as group:
        reader = csv.reader(group)
        for row in reader:
            numFreeBlocks.append(int(row[1]))
            numFreeInodes.append(int(row[2]))
            inodeBitmaps.append(int(row[4]))
            blockBitmaps.append(int(row[5]))


    # Open 'bitmap.csv' and use the data to create the inode / block free lists
    with open('bitmap.csv', 'r') as bitmap:
        reader = csv.reader(bitmap)
        for row in reader:
            if int(row[0]) in inodeBitmaps:
                freeInodes.append(int(row[1]))
            else:
                freeBlocks.append(int(row[1]))


    # Open 'inode.csv' and use the data to create a dictionary of all allocated inodes
    with open('inode.csv', 'r') as inode:
        reader = csv.reader(inode)
        for row in reader:
            allocatedInodes[int(row[0])] = Inode(row)


    # Open 'dictionary.csv' and use the data to create a list of all directory entries
    with open('directory.csv', 'r') as directory:
        reader = csv.reader(directory)
        for row in reader:
            directoryEntries.append(Entry(row))


            
    # TODO: Read 'indirect.csv' and do what with it?


    
    for inum in allocatedInodes:

        inode    = allocatedInodes[inum]
        entryNum = 0

        for pointer in range(0,12):
            blockPointer = inode.blockPointers[pointer]
            if blockPointer is 0: #TODO: or 'blockPointer > maxBlockNum'
                noop = 5 # TODO: INVALID_BLOCK_ERROR
            else:
                reference = blockRef(inum, entryNum)
                if blockPointer in allocatedBlocks:
                    allocatedBlocks[blockPointer].add_ref(reference)
                else:
                    block = Block(blockPointer)
                    block.add_ref(reference)
                    allocatedBlocks[blockPointer] = block
            entryNum = entryNum + 1

            
    # TODO: See #### 1 #### in my notes
    
    
    for entry in directoryEntries:

        parentNum = entry.parentNumber
        entryNum  = entry.entryNumber
        childNum  = entry.childNumber
        entryName = entry.entryName
        
        if childNum != parentNum or parentNum is 2:
            parentOf[childNum] = parentNum

        if childNum in allocatedInodes:
            allocatedInodes[childNum].add_ref(inodeRef(parentNum,entryNum))
        else:
            UNALLOCATED_INODE_ERROR(entry)   #TODO: possibly incomplete
            
        if entryNum is 0 and childNum != parentNum:
            INCORRECT_ENTRY_ERROR(parentNum, entryName, childNum, parentNum)

        if entryNum is 1 and childNum != parentOf[parentNum]:
            INCORRECT_ENTRY_ERROR(parentNum, entryName, childNum, parentOf[parentNum])


    for inum in allocatedInodes:

        inode     = allocatedInodes[inum]
        linkCount = len(inode.referenceList)
        
        if inum > 10 and linkCount is 0:
            inodes = 0
            for counter in range(0, len(numFreeInodes)):
                inodes = inodes + numFreeInodes[counter]
                if inum < inodes:
                    freeListNum = inodeBitmaps[counter]
                    MISSING_INODE_ERROR(inum, freeListNum)
                    break

        elif linkCount != inode.linkCount:
            INCORRECT_LINK_COUNT_ERROR(inum, inode.linkCount, linkCount)


    # TODO: See #### 2 #### in my notes

    # TODO: See #### 3 #### in my notes

    for block in allocatedBlocks:
        if len(allocatedBlocks[block].referenceList) > 1:
            MULTIPLY_REFERENCED_BLOCK_ERROR(allocatedBlocks[block])
    

    for block in freeBlocks:
        if block in allocatedBlocks:
            UNALLOCATED_BLOCK_ERROR(allocatedBlocks[block])
            
    
    sys.exit(0)





#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#

if __name__ == "__main__":
    main()
