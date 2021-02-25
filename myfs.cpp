#include "myfs.h"
#include <string.h>
#include <iostream>
#include <math.h>
#include <sstream>

using namespace std;

const char *MyFs::MYFS_MAGIC = "MYFS";

MyFs::MyFs(BlockDeviceSimulator *blkdevsim_):blkdevsim(blkdevsim_)
{
	struct myfs_header header;
	blkdevsim->read(0, sizeof(header), (char *)&header);

	if (strncmp(header.magic, MYFS_MAGIC, sizeof(header.magic)) != 0 ||
	    (header.version != CURR_VERSION)) {
		cout << "Did not find myfs instance on blkdev" << endl;
		cout << "Creating..." << endl;
		format();
		cout << "Finished!" << endl;
	}
}

void MyFs::format()
{
	// put the header in place
	struct myfs_header header;
	strncpy(header.magic, MYFS_MAGIC, sizeof(header.magic));
	header.version = CURR_VERSION;
	blkdevsim->write(0, sizeof(header), (const char*)&header);

	// put the folder struct in place
	struct folder thisFolder;
	thisFolder.numOfFiles = 0;
	blkdevsim->write(sizeof(header), sizeof(thisFolder), (const char*)&thisFolder);

}

void MyFs::create_file(string path_str, bool directory)
{
	// read the folder info
	struct folder thisFolder;
	struct myfs_header header;
	blkdevsim->read(sizeof(header), sizeof(thisFolder), (char*)&thisFolder);

	//Search for file
	for (size_t i = 0; i < thisFolder.numOfFiles; i++)
	{
		if(thisFolder.arrOfFiles[i].filename == path_str)//if filename exists
		{
			throw runtime_error("File already exists");
		}
	}


	if(thisFolder.numOfFiles > FOLDER_CAPACITY)//Check if there is space for new files
	{
		throw runtime_error("Out of space, delete some files first");
	}

	//Create new file
	struct folder_node newFile;
	strcpy(newFile.filename, path_str.c_str());
	newFile.pos = thisFolder.numOfFiles + 1;
	newFile.type = 'F';//For now we allow only files
	thisFolder.arrOfFiles[thisFolder.numOfFiles] = newFile;
	thisFolder.numOfFiles++;

	//Write the folder info back to the block device
	blkdevsim->write(sizeof(header), sizeof(thisFolder), (const char*)&thisFolder);

}

string MyFs::get_content(string path_str)
{
	int pos;
	char content[FILE_CAPACITY];

	// read the folder info
	struct folder thisFolder;
	struct myfs_header header;
	blkdevsim->read(sizeof(header), sizeof(thisFolder), (char*)&thisFolder);

	for (size_t i = 0; i < thisFolder.numOfFiles; i++)//Search for file
	{
		if(thisFolder.arrOfFiles[i].filename == path_str)//if filename exists
		{
			pos = thisFolder.arrOfFiles[i].pos;
			blkdevsim->read(FILE_CAPACITY * pos, FILE_CAPACITY, content);
			return content;
		}

	}
	throw runtime_error("File not found");
	return "";
}

void MyFs::set_content(string path_str, string content)
{
	int pos;

	// read the folder info
	struct folder thisFolder;
	struct myfs_header header;
	blkdevsim->read(sizeof(header), sizeof(thisFolder), (char*)&thisFolder);

	for (size_t i = 0; i < thisFolder.numOfFiles; i++)//Search for file
	{
		if(thisFolder.arrOfFiles[i].filename == path_str)//if filename exists
		{
			pos = thisFolder.arrOfFiles[i].pos;
			blkdevsim->write(FILE_CAPACITY * pos, content.size(), content.c_str());
			return;
		}

	}
	throw runtime_error("File not found");
	return;
}

MyFs::dir_list MyFs::list_dir(string path_str)
{
	dir_list ans;
	struct dir_list_entry file;
	struct folder thisFolder;
	struct myfs_header header;

	// read the folder info
	blkdevsim->read(sizeof(header), sizeof(thisFolder), (char*)&thisFolder);

	//Go through each file and add it to the vector
	for (size_t i = 0; i < thisFolder.numOfFiles; i++)
	{
		file.name = thisFolder.arrOfFiles[i].filename;
		file.is_dir = false;//For now always false
		file.file_size = getFileSize(thisFolder.arrOfFiles[i].pos);
		ans.push_back(file);
	}

	return ans;
}

int MyFs::getFileSize(int pos)
{
	int counter = 0;
	char buffer[FILE_CAPACITY];
	blkdevsim->read(FILE_CAPACITY * pos, FILE_CAPACITY, buffer);

	//Go through the space of the file and count how much bytes are in use
	for (size_t i = 0; i < FILE_CAPACITY; i++)
	{
		if(buffer[i] != '\0')
		{
			counter++;
		}
	}

	return counter;
}
