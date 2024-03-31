// filehdr.cc
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector,
//
//      Unlike in a real system, we do not keep track of file permissions,
//	ownership, last modification date, etc., in the file header.
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and nextBlockSectordisclaimer of warranty provisions.

#include "copyright.h"

#include "filehdr.h"
#include "debug.h"
#include "synchdisk.h"
#include "main.h"

Block::Block(){};
Block::Block(int nowSector, int newSector){
	this->blockSector = nowSector;
	this->nextBlockSector = newSector;
}
Block::~Block(){};

void Block::FetchFrom(int sectorNumber){
	kernel->synchDisk->ReadSector(sectorNumber, (char *)this);
	//Read the contents of a disk sector into a buffer.  Return only after the data has been read.
	//("sectorNumber" -- the disk sector to read,"data" -- the buffer to hold the contents of the disk sector)
}

void Block::WriteBack(){
	kernel->synchDisk->WriteSector(blockSector, (char *)this);
	//Write the contents of a buffer into a disk sector.  Return only after the data has been written.
        //("sectorNumber" -- the disk sector to be written,"data" -- the new contents of the disk sector)
}

int Block::GetBlockSector(){
	return blockSector;
}

void Block::SetBlockSector(int nowSector){
	blockSector = nowSector;
}

int Block::GetNextSector(){
	return nextBlockSector;
}

void Block::SetNextSector(int newSector){
	nextBlockSector = newSector;
}

char* Block::GetData(){
	return (char*)data;
}

bool Block::SetNext(int newSector){
	nextBlockSector = newSector;
	return true;
}

//----------------------------------------------------------------------
// MP4 mod tag
// FileHeader::FileHeader
//	There is no need to initialize a fileheader,
//	since all the information should be initialized by Allocate or FetchFrom.
//	The purpose of this function is to keep valgrind happy.
//----------------------------------------------------------------------
FileHeader::FileHeader()
{
	numBytes = -1;
	numSectors = -1;
	firstBlockSector = -1;

	// memset(dataSectors, -1, sizeof(dataSectors));
}

//----------------------------------------------------------------------
// MP4 mod tag
// FileHeader::~FileHeader
//	Currently, there is not need to do anything in destructor function.
//	However, if you decide to add some "in-core" data in header
//	Always remember to deallocate their space or you will leak memory
//----------------------------------------------------------------------
FileHeader::~FileHeader()
{
	// nothing to do now
}

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

// mp4
bool FileHeader::Allocate(PersistentBitmap *freeMap, int fileSize)
{
	// cout<<"Allocate "<<fileSize<<endl;
	numBytes = fileSize;
	numSectors = divRoundUp(fileSize, BlockDataSize);
	// cout<<"numSector: "<<numSectors<<endl;
	if (freeMap->NumClear() < numSectors)
		return FALSE; // not enough space

	int nowSector=-1, newSector = -1;
	for (int i = 0; i < numSectors; i++){
		newSector = freeMap->FindAndSet(); // find a free sector //

		// cout<<"Index: "<<i<<endl;
		if(nowSector == -1){ // header
			firstBlockSector = newSector;
		}else{ // blocks
			Block *nowBlock = new Block();
			nowBlock->FetchFrom(nowSector);
			nowBlock->SetNext(newSector);
			nowBlock->WriteBack();
			delete nowBlock;
		}
		Block *newBlock = new Block(newSector, -1);
		newBlock->WriteBack();
		delete newBlock;

		ASSERT(newSector >= 0);
		nowSector = newSector;
		// ASSERT(newSector >= 0);
		// dataSectors[i] = freeMap->FindAndSet();
		// since we checked that there was enough free space,
		// we expect this to succeed
		// ASSERT(dataSectors[i] >= 0);
	}
	return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void FileHeader::Deallocate(PersistentBitmap *freeMap)
{
	int nowSector = firstBlockSector;
	while(nowSector != -1){
		Block *block = new Block;
		block->FetchFrom(nowSector);

		ASSERT(freeMap->Test(nowSector)); // ought to be marked!
		freeMap->Clear(nowSector);

		nowSector = block->GetNextSector();
	}
	// for (int i = 0; i < numSectors; i++)
	// {
	// 	ASSERT(freeMap->Test((int)dataSectors[i])); // ought to be marked!
	// 	freeMap->Clear((int)dataSectors[i]);
	// }
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk.
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

// mp4
void FileHeader::FetchFrom(int sector)
{	
	// cout<<"Fetch from sector\n";
	char *buf = new char[SectorSize];

	kernel->synchDisk->ReadSector(sector, buf);
	bcopy(buf, (char*)this, sizeof(FileHeader));

	//Reminder: filehdr FrtchFrom and WirteBack work correctly...
	// cout<<"-------\n";
	// cout<<"firstBlockSector: "<<firstBlockSector<<endl;
	// cout<<"numBytes: "<<numBytes<<endl;
	// cout<<"numSectors: "<<numSectors<<endl;
	// cout<<"-------\n";

	// Print();

	delete []buf;

	// kernel->synchDisk->ReadSector(sector, (char *)this);
	/*
		MP4 Hint:
		After you add some in-core informations, you will need to rebuild the header's structure
	*/
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk.
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------
// mp4
void FileHeader::WriteBack(int sector)
{
	char *buf = new char[SectorSize];

	kernel->synchDisk->ReadSector(sector, buf);

	bcopy((char *)this, buf, sizeof(FileHeader));

	kernel->synchDisk->WriteSector(sector, buf);
	delete []buf;

	// kernel->synchDisk->WriteSector(sector, (char *)this);
	/*
		MP4 Hint:
		After you add some in-core informations, you may not want to write all fields into disk.
		Use this instead:
		char buf[SectorSize];
		memcpy(buf + offset, &dataToBeWritten, sizeof(dataToBeWritten));
		...
	*/
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

void FileHeader::ByteToSectorAndNextSector(int offset, int *a, int *b)
{
	int remainingBytes = offset;
	int nowSector = -1;
	int newSector = firstBlockSector;
	while(remainingBytes >= 0){
		Block *block = new Block;
		nowSector = newSector;
		block->FetchFrom(nowSector);
		//Read the contents of a disk sector into a buffer.  Return only after the data has been read.
		newSector = block->GetNextSector();//return nextblock
		delete block;
		remainingBytes -= BlockDataSize;
		// cout<<"----------------\n";
		// cout<<"Now Sector: "<<nowSector<<endl;
		// cout<<"New Sector: "<<newSector<<endl;
		// cout<<"----------------\n";
	}
	ASSERT(nowSector != -1);
	(*a) = nowSector;
	(*b) = newSector;
	// return (dataSectors[offset / SectorSize]);
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int FileHeader::FileLength()
{
	return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void FileHeader::Print()
{
	char *data = new char[SectorSize];
	int nowSector;

	printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
	nowSector = firstBlockSector;
	while(nowSector != -1){
		cout<<nowSector<<" ";
		Block *block = new Block;
		block->FetchFrom(nowSector);
		nowSector = block->GetNextSector();
		delete block;
	}
	// cout<<nowSector<<" ";

	// for (i = 0; i < numSectors; i++)
	// 	printf("%d ", dataSectors[i]);

	printf("\nFile contents:\n");
	nowSector = firstBlockSector;
	int nowByte = 0;
	while(nowSector != -1 && (nowByte < numBytes)){
		Block *block = new Block;
		block->FetchFrom(nowSector);
		char *data = block->GetData();
		for (int i = 0; (i < BlockDataSize) && (nowByte < numBytes); i++, nowByte++)
		{
			// printf("%c", block->GetData(i));
			if ('\040' <= data[i] && data[i] <= '\176') // isprint(data[j])
				printf("%c", data[i]);
			else
				printf("\\%x", (unsigned char)data[i]);
		}
		nowSector = block->GetNextSector();
		printf("\n");
	}
	

	// for (i = k = 0; i < numSectors; i++)
	// {
	// 	kernel->synchDisk->ReadSector(dataSectors[i], data);
	// 	for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++)
	// 	{
	// 		if ('\040' <= data[j] && data[j] <= '\176') // isprint(data[j])
	// 			printf("%c", data[j]);
	// 		else
	// 			printf("\\%x", (unsigned char)data[j]);
	// 	}
	// 	printf("\n");
	// }
	delete[] data;
}

int FileHeader::GetFirstBlockSector(){
	return firstBlockSector;
}