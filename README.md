This project implements a key-value store using a hash table and linked lists. 


# File organization 

A database *db* is composed of four files, which are: *db.h*, *db.blk*, *db.kv* and *db.dkv*. 
Each of these files begins with a specific magic number, which allows to verify the correct type of file when opening it.

## File *.h*

The file with the extension *.h* contains the database's hash table. 
Its magic number is *1*. 

Its header is composed of the file's magic number and the index of the hash function used by the database: 
| MAGIC_NUMBER_H | hashFunctionIndex |
|----------------|-------------------|

To look up a key *K*, the database first computes the hash *H(K)* of *K*. 
Then, it looks up the hash table entry *i* by reading *sizeof(unsigned int)* in the hash table file at the position: 
> HEADER_H + i\* sizeof(unsigned int)

The hash table entry *i* contains the index of the first block which stores the offsets of keys whose hash value is equal to *i*. 

The read function returns one of the following values:
* Read returns -1 meaning that an error occurred. 
* Read returns *0* meaning that no block index is written at index *i*, i.e., the database contains no key whose hash value is equal to *i*. 
* Read returns *sizeof(unsigned int)* meaning that a block index is written at index *i*. This block index is either equal to *-1*, meaning that the entry contains no block index (it contained one but the keys corresponding to hash value *i* were deleted, so the block was released), or it is equal to a blockIndex > 0, meaning that the block blockIndex is the first block containing offsets of keys whose hash values is equal to *i*. 

The next step is to check the *.blk* file which contains the blocks. 

## File *.blk*

The file with the extension *.blk* contains the database's blocks. 
Its magic number is *2*. 

Its header is composed of the file's magic number and the number of blocks used by the database: 
| MAGIC_NUMBER_BLK | nbUsedBlocks |
|------------------|--------------|

To look up a block *i*, the database reads *BLOCK_SIZE* bytes at the file position:
> HEADER_BLK + i\*BLOCK_SIZE

A block *i* contains the offsets in the *KV* file of keys having the same hash value. 
A block can contain at most (*BLOCK_SIZE* - HEADER_BLK)/sizeof(unsigned int) offsets. 
To tolerate more keys of the same hash value, blocks are organized in a chained list. Each block contains in its header the index of the next block containing offsets of keys whose hash value is equal to *i*. 

Each block is composed of a boolean identifying whether the block has a link to a next block, followed by an unsigned int containing the next block index, followed by an unsigned int containing the number of occupied entries in the block:
| hasNextBlock | indexNextBlock | nbOccupiedSlots |
|--------------|----------------|-----------------|

To look up an entry *i* in a block, the database reads *sizeof(unsigned int)* bytes in the block at position:
> HEADER_BLOCK + i\*sizeof(unsigned int)

## File *.kv* 

The file with the extension *.kv* contains the database's data. 
Its magic number is *3*. 

Its header is composed of the file's magic number:
| MAGIC_NUMBER_KV |
|------------------|

The database contains tuples of keys and values. 
A key/value is composed of a length (unsigned int) and a content of length bytes. Thus, a kv entry looks like:
| keySize | key | valSize | val |
|---------|-----|---------|-----|

There is no easy way to look up directly entry *i* of kv, because keys and values can have different lengths. 
Therefore, we use another file, the *.dkv* file to store that information. 

## File *.dkv*

The file with the extension *.dkv* contains for each entry *i* of the database the offset and length of *i*. 
Its magic number is *3*. 

Its header is composed of the file's magic number and the number of the database's slots:
| MAGIC_NUMBER_DKV | nbSlots |
|------------------|---------|

Slot *i* of the database can be looked up by accessing entry:
> HEADER_DKV + i\*(2*sizeof(unsigned int))
It contains the slot's length and offset in *.kv*:
| size | offset |
|------|--------|
