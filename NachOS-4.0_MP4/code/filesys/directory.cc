// directory.cc
//	Routines to manage a directory of file names.
//
//	The directory is a table of fixed length entries; each
//	entry represents a single file, and contains the file name,
//	and the location of the file header on disk.  The fixed size
//	of each directory entry means that we have the restriction
//	of a fixed maximum size for file names.
//
//	The constructor initializes an empty directory of a certain size;
//	we use ReadFrom/WriteBack to fetch the contents of the directory
//	from disk, and to write back any modifications back to disk.
//
//	Also, this implementation has the restriction that the size
//	of the directory cannot expand.  In other words, once all the
//	entries in the directory are used, no more files can be created.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "filehdr.h"
#include "directory.h"

//----------------------------------------------------------------------
// Directory::Directory
// 	Initialize a directory; initially, the directory is completely
//	empty.  If the disk is being formatted, an empty directory
//	is all we need, but otherwise, we need to call FetchFrom in order
//	to initialize it from disk.
//
//	"size" is the number of entries in the directory
//----------------------------------------------------------------------

Directory::Directory(int size)
{
    table = new DirectoryEntry[size];

    // MP4 mod tag
    memset(table, 0, sizeof(DirectoryEntry) * size); // dummy operation to keep valgrind happy

    tableSize = size;
    for (int i = 0; i < tableSize; i++){
        table[i].inUse = FALSE;
    }
}

//----------------------------------------------------------------------
// Directory::~Directory
// 	De-allocate directory data structure.
//----------------------------------------------------------------------

Directory::~Directory()
{
    delete[] table;
}

//----------------------------------------------------------------------
// Directory::FetchFrom
// 	Read the contents of the directory from disk.
//
//	"file" -- file containing the directory contents
//----------------------------------------------------------------------

void Directory::FetchFrom(OpenFile *file)
{
    (void)file->ReadAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::WriteBack
// 	Write any modifications to the directory back to disk
//
//	"file" -- file to contain the new directory contents
//----------------------------------------------------------------------

void Directory::WriteBack(OpenFile *file)
{
    (void)file->WriteAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::FindIndex
// 	Look up file name in directory, and return its location in the table of
//	directory entries.  Return -1 if the name isn't in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int Directory::FindIndex(char *name)
{
    for (int i = 0; i < tableSize; i++){
        if (table[i].inUse && !strncmp(table[i].name, name, FileNameMaxLen)){
            return i;
        }
    }
        
    return -1; // name not in directory
}

// mp4
// Find Recursive in the directory

int Directory::FindRecursive(char *name){
    char head[FileNameMaxLen + 1];
    int headSize;
    headSize = PathHead(name, head); //head的長度

    // check if head is in directory
    int index = FindIndex(head);
    // cout<<"find index: "<<index<<endl;
    if(index == -1) return -1; // not in directory

    // Last one in path
    if(!strcmp(head, name+1)) return table[index].sector;

    // in directory
    int sector = table[index].sector; //to file header sector 
    
    // There still exist directory in path
    OpenFile *openFile = new OpenFile(sector); //to/t1/t2，open to file
    Directory *dir = new Directory(NumDirEntries); 
    dir->FetchFrom(openFile); //把openfile load進dir，以directory形式呈現
    int rt = dir->FindRecursive(name+headSize+1);
    delete openFile;
    delete dir;
    return rt;
}

//----------------------------------------------------------------------
// Directory::Find
// 	Look up file name in directory, and return the disk sector number
//	where the file's header is stored. Return -1 if the name isn't
//	in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int Directory::Find(char *name){
    // int i = FindIndex(name);

    // if (i != -1)
    //     return table[i].sector;
    // return -1;
    // cout<<"Find: "<<name<<endl;
    // int sector = FindRecursive(name);
    // if(sector != -1) cout<<"Find Seccuess\n";
    // else cout<<"Find Fail\n";
    // return sector;
    if(!strcmp(name, "/"))
        return RootDirectorySector;
    return FindRecursive(name);
}

//----------------------------------------------------------------------
// Directory::Add
// 	Add a file into the directory.  Return TRUE if successful;
//	return FALSE if the file name is already in the directory, or if
//	the directory is completely full, and has no more space for
//	additional file names.
//
//	"name" -- the name of the file being added
//	"newSector" -- the disk sector containing the added file's header
//----------------------------------------------------------------------

bool Directory::Add(char *name, int newSector, int type)
{
    // name = name + 1; // ignore the first slash
    // if (FindIndex(name) != -1)
    //     return FALSE;

    // for (int i = 0; i < tableSize; i++)
    //     if (!table[i].inUse)
    //     {
    //         table[i].inUse = TRUE;
    //         strncpy(table[i].name, name, FileNameMaxLen);
    //         table[i].sector = newSector;
    //         return TRUE;
    //     }
    // return FALSE; // no space.  Fix when we have extensible files.
    // cout<<"---AddRecursive---\n";
    bool pass =  AddRecursive(name, newSector, type); //newsector 是fileheader的sector
    // cout<<"---End AddRecursive---\n";
    return pass;
}

bool Directory::AddRecursive(char *name, int newSector, int type){
    char head[FileNameMaxLen + 1];
    int headSize = PathHead(name, head);

    if(!strcmp(name + 1, head)){//head is the last one in path
        for(int i=0;i<tableSize;i++){
            if (!table[i].inUse){
                table[i].inUse = TRUE;
                table[i].type = type;
                table[i].sector = newSector; //to file headersector
                strncpy(table[i].name, head, FileNameMaxLen);
                // cout<<"----------\n";
                // cout<<"Add File:"<<name<<"\n";
                // cout<<"Sector: "<<newSector<<endl;
                // cout<<"Type: "<<(type==1?"[D]":"[F]")<<endl;
                // cout<<"----------\n";
                return true;
            }
        }
        return false;
    }else{
        for(int i=0;i<tableSize;i++){
            if (!strcmp(head, table[i].name) && table[i].inUse){
                OpenFile *openFile = new OpenFile(table[i].sector);
                Directory *dir = new Directory(NumDirEntries);
                dir->FetchFrom(openFile);
                bool pass = dir->AddRecursive(name+headSize+1, newSector, type);//mp4why
                dir->WriteBack(openFile);
                delete dir;
                delete openFile;
                return pass;
            }
        }
        return false;
    }
    return false;
}

//----------------------------------------------------------------------
// Directory::Remove
// 	Remove a file name from the directory.  Return TRUE if successful;
//	return FALSE if the file isn't in the directory.
//
//	"name" -- the file name to be removed
//----------------------------------------------------------------------

bool Directory::Remove(char *name)
{
    // int i = FindIndex(name);

    // if (i == -1)
    //     return FALSE; // name not in directory
    // table[i]. = FALSE;
    // return TRUE;

    return RemoveRecursive(name);
}

bool Directory::RemoveRecursive(char *name){
    char head[FileNameMaxLen + 1];
    int headSize;
    headSize = PathHead(name, head);

    // check if head is in directory
    int index = FindIndex(head);
    if(index == -1) return false; // not in directory


    // Last in path
    if(!strcmp(head, name+1)){
        table[index].inUse = false;
        return true;
    }
    
    // in directory
    int sector = table[index].sector;
    // There still exist directory in path
    OpenFile *openFile = new OpenFile(sector);
    Directory *dir = new Directory(NumDirEntries);
    dir->FetchFrom(openFile);
    bool pass = dir->RemoveRecursive(name+headSize+1);
    dir->WriteBack(openFile);
    delete dir;
    delete openFile;
    return pass;
}

//----------------------------------------------------------------------
// Directory::List
// 	List all the file names in the directory.
//----------------------------------------------------------------------

void Directory::List(bool recursiveFlag)
{
    // for (int i = 0; i < tableSize; i++)
    //     if (table[i].inUse)
    //         printf("%s\n", table[i].name);
    if(recursiveFlag == false){
        for(int i = 0; i < tableSize; i++){
            if(table[i].inUse){
                // if(table[i].type == 0) cout<<"[F] ";
                // else cout<<"[D] ";
                printf("%s\n", table[i].name);
            }
        }
    }else{
        ListRecursive(0);
    }
}

void Directory::ListRecursive(int level){
    for(int i = 0; i < tableSize; i++){
        if (table[i].inUse){
            if(table[i].type == 0){
                for(int j = 0;j < level;j++) printf("    ");
                cout<<"[F] ";
                printf("%s\n", table[i].name);
            }else{
                for(int j = 0;j < level;j++) printf("    ");
                cout<<"[D] ";
                printf("%s\n", table[i].name);

                OpenFile *openFile = new OpenFile(table[i].sector);
                Directory *dir = new Directory(NumDirEntries);
                dir->FetchFrom(openFile);
                dir->ListRecursive(level+1);
                delete dir;
                delete openFile;
            }
        }
    }
}

//----------------------------------------------------------------------
// Directory::Print
// 	List all the file names in the directory, their FileHeader locations,
//	and the contents of each file.  For debugging.
//----------------------------------------------------------------------

void Directory::Print()
{
    FileHeader *hdr = new FileHeader;

    printf("Directory contents:\n");
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse)
        {
            printf("Name: %s, Sector: %d\n", table[i].name, table[i].sector);
            hdr->FetchFrom(table[i].sector);
            hdr->Print();
        }
    printf("\n");
    delete hdr;
}

int Directory::PathHead(char *name, char*head){
    int startIndex = 1;
    int endIndex;
    int headSize = 0;

    for(int i=startIndex;i<(FileNameMaxLen+1);i++){
        if(name[i] == '\0' || name[i] == '/'){
            endIndex = i;
            break;
        }
        headSize++;
        head[i-startIndex] = name[i];
    }
    head[endIndex-startIndex] = '\0';
    // cout<<"head: "<<head<<endl;
    // cout<<"name: "<<name<<endl;

    return headSize;
}