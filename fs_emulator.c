#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct{
	uint32_t inode;
	uint32_t parentInode;
	char type;
	char name[32];
} Inode;

Inode inodeList[1024];
size_t inodeCount = 0;
uint32_t currentInode = 0;

void loadInodeList(const char *path) {
	// read the inodeList from the file path to your inodeList array
	// update the inodeCount
	// remember that your directory will have only one inodeList with a given number of inodes in the list
	
	FILE *fp = fopen(path, "rb");
	if(fp == NULL){
		printf("Cannot open inodeList\n");
		exit(1);
	}

	Inode temp;
	//read one Inode at a time and put into inodeList if it is a valid Inode
	while(fread(&temp, sizeof(Inode), 1, fp) == 1){
		//check if valid Inode
		if(temp.inode < 1024 && (temp.type == 'd' || temp.type == 'f')){
			inodeList[inodeCount] = temp;
			inodeCount ++;
		}
	}

	fclose(fp);
}

void saveInodeList(const char *path) {
	// write the content of the inodeList to the file path
	FILE *fp = fopen(path, "wb");
        if(fp == NULL){
                printf("Cannot open inodeList\n");
                exit(1);
        }

	fwrite(inodeList, sizeof(Inode), inodeCount, fp);

	fclose(fp);
}


void changeDirectory(const char *name) {
	// iterate through the inodeList to search whether a directory with the name already exists
	// if yes, then set the currentInode value to the index of the list with that name
	// otherwise print an error message to indicate that the directory is not found
	
	uint32_t current = currentInode;
	
	for(int i = 0; i < inodeCount; i++){
		if(inodeList[i].type == 'd'){
			if(strcmp(inodeList[i].name, name) == 0){
				currentInode = i;
			}
		}
	}

	if(current == currentInode){
		printf("Directory not found\n");
	}
}

void listContents() {
	// iterate through inodeList and print the members of those elements in the list for which the 
	// parentInode is the currentInode
	for(int i = 0; i < inodeCount; i++){
		Inode inode = inodeList[i];
		if(inode.parentInode == currentInode){
			printf("inode: %u, type: %c, name: %s\n", inode.inode, inode.type, inode.name);
		}
	}
}
void createDirectory(const char *name) {
	// iterate through the inodeList to see if the name of the directory exists; if yes, exit
	// check to see if the inodeCount is 1024; if yes, exit
	// create a new inode with new inodeCount, set its parentInode and type, and also its name
	// create a file with its name as the inode number
	// write into this file its . and .. inode values
	
	if(inodeCount == 1024){
		printf("There is no space for additional files\n");
                exit(1);
        }

	for(int i = 0; i < inodeCount; i++){
                Inode inode = inodeList[i];
                if((inode.type == 'd' || inode.type == 'f') && strcmp(inode.name, name) == 0){
                        printf("File with that name already exists\n");
                        exit(1);
                }
        }
	
	//create new Inode 
	Inode *newInode = &inodeList[inodeCount];
	newInode->inode = inodeCount;
	newInode->parentInode = currentInode;
	newInode->type = 'd';
	strncpy(newInode->name, name, 32);

	//create new file
	char filename[15];
	snprintf(filename, sizeof(filename), "%u", newInode->inode);

	//write into the new file
	FILE *fp = fopen(filename, "w");
	if(fp == NULL){
		printf("Couldn't open file\n");
		exit(1);
	}
	fprintf(fp, "%u .\n%u ..\n", newInode->inode, newInode->parentInode);
	inodeCount++;
	fclose(fp);
}

void createFile(const char *name) {
	// iterate through inodeList to see if filename exists; if yes, exit
	// check if the inodeCount is 1024; if yes, exit
	// create a new entry for an inode in inodeList
	// create a file with its name as the inode number
	// write into this file its filename
	
	if(inodeCount == 1024){
		printf("There is no space for additional files\n");
		exit(1);
	}

	for(int i = 0; i < inodeCount; i++){
		Inode inode = inodeList[i];
		if((inode.type == 'd' || inode.type == 'f') && strcmp(inode.name, name) == 0){
			printf("File with that name already exists\n");
			exit(1);
		}
	}
	
	//create new Inode
        Inode *newInode = &inodeList[inodeCount];
        newInode->inode = inodeCount;
        newInode->parentInode = currentInode;
        newInode->type = 'f';
        strncpy(newInode->name, name, 32);

        //create new file
        char filename[15];
        snprintf(filename, sizeof(filename), "%u", newInode->inode);

        //write into the new file
        FILE *fp = fopen(filename, "w");
        if(fp == NULL){
                printf("Couldn't open file\n");
                exit(1);
        }
        fprintf(fp, "%s", newInode->name);
	inodeCount++;
        fclose(fp);
}

int main(int argc, char *argv[]){
	//must provide exactly one command line argument
	if(argc != 2){
		printf("Must provide directory name\n");
		return 1;
	}
	
	//change the working directory
	if(chdir(argv[1]) != 0){
		perror("chdir failed");
		return 1;
	}

	//load the inodeList
	loadInodeList("inodes_list");

	//read each line from stdin
	char line[100];
	while(fgets(line, sizeof(line), stdin) != NULL){
		//strip newline from input
		char *newline = strchr(line, '\n');
		if(line != NULL){
			*newline = '\0';
		}
		
		//split line into command and argument
		char cmd[32];
		char arg[33];
		int argc = 0; //argument count

		char *space = strchr(line, ' ');
		if(space == NULL){
			//copy whole line into cmd leaving space for \0
			strncpy(cmd, line, 31);
		}
		else{
			size_t cmdLength = space - line;
			if(cmdLength > 31){
				cmdLength = 31;
			}
			//copy the command from line without the argument
			strncpy(cmd, line, cmdLength);

			//initialize a pointer right after the space
			char *rest = space + 1;
			if(*rest != '\0'){
				strncpy(arg, rest, 32);
				argc = 1;

				//check for more args
				char *moreArgs = strchr(rest, ' ');
				if(moreArgs != NULL && *moreArgs != '\0'){
					argc = 2;
				}

			}
		}

		//if cmd is NULL, wait for user to type something
		if(cmd == NULL){
			continue;
		}

		//if cmd is exit, then break the loop
		if(strcmp(cmd, "exit") == 0){
			break;
		}
		else if(strcmp(cmd, "ls") == 0 && argc == 0){
			listContents();
		}
		else if(strcmp(cmd, "cd") == 0 && argc == 1){
			changeDirectory(arg);
		}
		else if(strcmp(cmd, "mkdir") == 0 && argc == 1){
			createDirectory(arg);
		}
		else if(strcmp(cmd, "touch") == 0 && argc == 1){
			createFile(arg);
		}
		else if(strcmp(cmd, "cd") == 0 || strcmp(cmd, "mkdir") == 0 || strcmp(cmd, "touch") == 0){
			printf("Wrong number of arguments\n");
		}
		else{
			printf("Unknown command\n");
		}
	}

	saveInodeList("inodes_list");
	return 0;
}
