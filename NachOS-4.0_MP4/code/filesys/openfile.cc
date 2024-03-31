// openfile.cc
//	Routines to manage an open Nachos file.  As in UNIX, a
//	file must be open before we can read or write to it.
//	Once we're all done, we can close it (in Nachos, by deleting
//	the OpenFile data structure).
//
//	Also as in UNIX, for convenience, we keep the file header in
//	memory while the file is open.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.
#ifndef FILESYS_STUB

#include "copyright.h"
#include "main.h"
#include "filehdr.h"
#include "openfile.h"
#include "synchdisk.h"

//----------------------------------------------------------------------
// OpenFile::OpenFile
// 	Open a Nachos file for reading and writing.  Bring the file header
//	into memory while the file is open.
//
//	"sector" -- the location on disk of the file header for this file
//----------------------------------------------------------------------

OpenFile::OpenFile(int sector) //the location on disk of the file header for this file
{
    hdr = new FileHeader;
    hdr->FetchFrom(sector);
    // hdr->Print();
    seekPosition = 0;
}

//----------------------------------------------------------------------
// OpenFile::~OpenFile
// 	Close a Nachos file, de-allocating any in-memory data structures.
//----------------------------------------------------------------------

OpenFile::~OpenFile()
{
    delete hdr;
}

//----------------------------------------------------------------------
// OpenFile::Seek
// 	Change the current location within the open file -- the point at
//	which the next Read or Write will start from.
//
//	"position" -- the location within the file for the next Read/Write
//----------------------------------------------------------------------

void OpenFile::Seek(int position)
{
    seekPosition = position;
}

//----------------------------------------------------------------------
// OpenFile::Read/Write
// 	Read/write a portion of a file, starting from seekPosition.
//	Return the number of bytes actually written or read, and as a
//	side effect, increment the current position within the file.
//
//	Implemented using the more primitive ReadAt/WriteAt.
//
//	"into" -- the buffer to contain the data to be read from disk
//	"from" -- the buffer containing the data to be written to disk
//	"numBytes" -- the number of bytes to transfer
//----------------------------------------------------------------------

int OpenFile::Read(char *into, int numBytes)
{
    int result = ReadAt(into, numBytes, seekPosition);
    seekPosition += result;
    return result;
}

int OpenFile::Write(char *into, int numBytes)
{
    int result = WriteAt(into, numBytes, seekPosition);
    seekPosition += result;
    return result;
}

//----------------------------------------------------------------------
// OpenFile::ReadAt/WriteAt
// 	Read/write a portion of a file, starting at "position".
//	Return the number of bytes actually written or read, but has
//	no side effects (except that Write modifies the file, of course).
//
//	There is no guarantee the request starts or ends on an even disk sector
//	boundary; however the disk only knows how to read/write a whole disk
//	sector at a time.  Thus:
//
//	For ReadAt:
//	   We read in all of the full or partial sectors that are part of the
//	   request, but we only copy the part we are interested in.
//	For WriteAt:
//	   We must first read in any sectors that will be partially written,
//	   so that we don't overwrite the unmodified portion.  We then copy
//	   in the data that will be modified, and write back all the full
//	   or partial sectors that are part of the request.
//
//	"into" -- the buffer to contain the data to be read from disk
//	"from" -- the buffer containing the data to be written to disk
//	"numBytes" -- the number of bytes to transfer
//	"position" -- the offset within the file of the first byte to be
//			read/written
//----------------------------------------------------------------------

// mp4
int OpenFile::ReadAt(char *into, int numBytes, int position)
{
    // cout<<"---Reading At---\n";
    int fileLength = hdr->FileLength();
    int firstSectorIndex, lastSectorIndex, numSectors;
    char *buf;

    if ((numBytes <= 0) || (position >= fileLength))
        return 0; // check request
    if ((position + numBytes) > fileLength)
        numBytes = fileLength - position;
    DEBUG(dbgFile, "Reading " << numBytes << " bytes at " << position << " from file of length " << fileLength);

    firstSectorIndex = divRoundDown(position, BlockDataSize);
    lastSectorIndex = divRoundDown(position + numBytes - 1, BlockDataSize);
    numSectors = 1 + lastSectorIndex - firstSectorIndex;

    // read in all the full and partial sectors that we need
    // modification
    buf = new char[numSectors * BlockDataSize];
    int nowSector, newSector;
    for (int i = firstSectorIndex; i <= lastSectorIndex; i++){
        hdr->ByteToSectorAndNextSector(i * BlockDataSize, &nowSector, &newSector);
        Block *block = new Block;
        block->FetchFrom(nowSector);
        // cout<<"Block:\n NowSector: "<<block->GetBlockSector()<<"\n NextSector: "<<block->GetNextSector()<<endl;
        char *data = block->GetData();
        bcopy(data, &buf[(i - firstSectorIndex) * BlockDataSize], BlockDataSize);
        // cout<<"Compare: "<<strncmp(data, &buf[(i - firstSectorIndex) * BlockDataSize], BlockDataSize)<<endl;
    }
        // kernel->synchDisk->ReadSector(hdr->ByteToSector(i * BlockDataSize),
        //                               &buf[(i - firstSectorIndex) * BlockDataSize]);
    // copy the part we want
    bcopy(&buf[position - (firstSectorIndex * BlockDataSize)], into, numBytes);

    // cout<<"---Reading At End---\n";

    delete[] buf;
    
    return numBytes;
}

// mp4
int OpenFile::WriteAt(char *from, int numBytes, int position)
{
    // cout<<"---Writing At---\n";
    // cout<<"First Block "<<hdr->GetFirstBlockSector()<<endl;
    int fileLength = hdr->FileLength();
    int i, firstSector, lastSector, numSectors;
    bool firstAligned, lastAligned;
    char *buf;

    if ((numBytes <= 0) || (position >= fileLength)) //mp4why?
        return 0; // check requeston + numBytes) > fileLength)
    if ((position + numBytes) > fileLength)
        numBytes = fileLength - position;
    DEBUG(dbgFile, "Writing " << numBytes << " bytes at " << position << " from file of length " << fileLength);

    firstSector = divRoundDown(position, BlockDataSize);
    lastSector = divRoundDown(position + numBytes - 1, BlockDataSize);
    numSectors = 1 + lastSector - firstSector;

    buf = new char[numSectors * BlockDataSize];

    memset(buf, 0, sizeof(char) * numSectors * BlockDataSize); //要將char buf字串陣列全部初始化為0,要設定的bytes數

    firstAligned = (position == (firstSector * BlockDataSize)); //firstAligned is true if....
    lastAligned = ((position + numBytes) == ((lastSector + 1) * BlockDataSize));

    // read in first and last sector, if they are to be partially modified
    //mp4why
    if (!firstAligned)
        ReadAt(buf, BlockDataSize, firstSector * BlockDataSize);
    if (!lastAligned && ((firstSector != lastSector) || firstAligned))
        ReadAt(&buf[(lastSector - firstSector) * BlockDataSize],
               BlockDataSize, lastSector * BlockDataSize);
    //mp4why
    // copy in the bytes we want to change
    bcopy(from, &buf[position - (firstSector * BlockDataSize)], numBytes);
    //copy numBytes of SRC(from) to DEST

    // modification
    // write modified sectors back
    int nowSector, newSector;
    for (i = firstSector; i <= lastSector; i++){
        hdr->ByteToSectorAndNextSector(i * BlockDataSize, &nowSector, &newSector);
        //Return which disk sector is storing a particular byte within the file
        
        Block *block = new Block;
        block->SetBlockSector(nowSector);
        block->SetNextSector(newSector);
        char *data = block->GetData();
        bcopy(&buf[(i - firstSector) * BlockDataSize], data, BlockDataSize);//mp4why?
        //Copy BlockDataSize bytes of &buf[(i - firstSector) * BlockDataSize] to data
        // cout<<"Compare: "<<strncmp(data, &buf[(i - firstSector) * BlockDataSize], BlockDataSize)<<endl;
        //cout<<"Block:\n NowSector: "<<block->GetBlockSector()<<"\n NextSector: "<<block->GetNextSector()<<endl;
        block->WriteBack();
        //Write the contents of a buffer into a disk sector.  Return only after the data has been written.
        //("sectorNumber" -- the disk sector to be written,"data" -- the new contents of the disk sector)
    }

    delete[] buf;
    // cout<<"---Writing At End---\n";
    return numBytes;
}

//----------------------------------------------------------------------
// OpenFile::Length
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int OpenFile::Length()
{
    return hdr->FileLength();
}

#endif //FILESYS_STUB
