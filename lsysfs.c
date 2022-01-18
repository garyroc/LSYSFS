/**
 * Less Simple, Yet Stupid Filesystem.
 * 
 * Mohammed Q. Hussain - http://www.maastaar.net
 *
 * This is an example of using FUSE to build a simple filesystem. It is a part of a tutorial in MQH Blog with the title "Writing Less Simple, Yet Stupid Filesystem Using FUSE in C": http://maastaar.net/fuse/linux/filesystem/c/2019/09/28/writing-less-simple-yet-stupid-filesystem-using-FUSE-in-C/
 *
 * License: GNU GPL
 */
 
#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

char dir_list[ 256 ][ 256 ];
int curr_dir_idx = -1;

char files_list[ 256 ][ 256 ];
int curr_file_idx = -1;

char files_content[ 256 ][ 256 ];
int curr_file_content_idx = -1;

//Store stat->time explicitly
//[0]: access time, [1]: modified time, [2]: change time
int dir_time[256][3] = {{0}};
int files_time[256][3] = {{0}};

void add_dir( const char *dir_name )
{
	printf("add_dir\n");
	curr_dir_idx++;
	strcpy( dir_list[ curr_dir_idx ], dir_name );
}

int is_dir( const char *path )
{
	printf("is_dir\n");
	path++; // Eliminating "/" in the path
	
	for ( int curr_idx = 0; curr_idx <= curr_dir_idx; curr_idx++ )
		if ( strcmp( path, dir_list[ curr_idx ] ) == 0 )
			return 1;
	
	return 0;
}

void add_file( const char *filename )
{
	printf("add_file\n");
	curr_file_idx++;
	strcpy( files_list[ curr_file_idx ], filename );
	
	curr_file_content_idx++;
	strcpy( files_content[ curr_file_content_idx ], "" );
}

int is_file( const char *path )
{
	printf("is_file\n");
	path++; // Eliminating "/" in the path
	
	for ( int curr_idx = 0; curr_idx <= curr_file_idx; curr_idx++ )
		if ( strcmp( path, files_list[ curr_idx ] ) == 0 )
			return 1;
	
	return 0;
}

int get_file_index( const char *path )
{
	printf("get_file_index\n");
	path++; // Eliminating "/" in the path
	
	for ( int curr_idx = 0; curr_idx <= curr_file_idx; curr_idx++ )
		if ( strcmp( path, files_list[ curr_idx ] ) == 0 )
			return curr_idx;
	
	return -1;
}

int get_dir_index( const char *path )
{
	path++; // Eliminating "/" in the path
	
	for ( int curr_idx = 0; curr_idx <= curr_dir_idx; curr_idx++ )
		if ( strcmp( path, dir_list[ curr_idx ] ) == 0 )
			return curr_idx;
	return -1;
}

void write_to_file( const char *path, const char *new_content )
{
	printf("write_to_file\n");
	int file_idx = get_file_index( path );
	
	if ( file_idx == -1 ) // No such file
		return;

	//minor bug of LSYSFS
	if(strlen(files_content[file_idx]) != 0)
		strcat(files_content[file_idx], new_content);
	else
		strcpy(files_content[file_idx], new_content ); 
}

// ... //


static int do_getattr( const char *path, struct stat *st )
{
	printf("do_getattr\n");
	st->st_uid = getuid(); // The owner of the file/directory is the user who mounted the filesystem
	st->st_gid = getgid(); // The group of the file/directory is the same as the group of the user who mounted the filesystem
	
	int cur_index = 0;
	
	if ( strcmp( path, "/" ) == 0)
	{
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2; // Why "two" hardlinks instead of "one"? The answer is here: http://unix.stackexchange.com/a/101536
		st->st_atime = time(NULL);
		st->st_mtime = time(NULL); 
		st->st_ctime = time(NULL);
	}
	else if ( is_file( path ) == 1 )
	{
		st->st_mode = S_IFREG | 0644;
		st->st_nlink = 1;
		st->st_size = 1024;
		cur_index = get_file_index(path);
		if(files_time[cur_index][0] == 0 && files_time[cur_index][1] == 0 && files_time[cur_index][2] == 0)
		{
			//maintain the old time
			st->st_atime = time(NULL);
			st->st_mtime = time(NULL);
			st->st_ctime = time(NULL); 
			files_time[cur_index][0] = st->st_atime;
			files_time[cur_index][1] = st->st_mtime;
			files_time[cur_index][2] = st->st_ctime;
		}
		else
		{
			//get the old time
			st->st_atime = files_time[cur_index][0];
			st->st_mtime = files_time[cur_index][1];
			st->st_ctime = files_time[cur_index][2];
		}
	}
	else if( is_dir( path ) == 1) 
	{
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2;
		cur_index = get_dir_index(path);
		if(dir_time[cur_index][0] == 0 && dir_time[cur_index][1] == 0 && dir_time[cur_index][2] == 0)
		{
			st->st_atime = time(NULL);
			st->st_mtime = time(NULL); 
			dir_time[cur_index][0] = st->st_atime;
			dir_time[cur_index][1] = st->st_mtime;
			dir_time[cur_index][2] = st->st_ctime;
		}
		else
		{
			st->st_atime = dir_time[cur_index][0];
			st->st_mtime = dir_time[cur_index][1];
			st->st_ctime = dir_time[cur_index][2];
		}
	}
	else
	{
		printf("-ENOENT\n");
		return -ENOENT;
	}
	
	return 0;
}

static int do_readdir( const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi )
{
	printf("do_readdir\n");
	printf("Current address:%s\n",path);

	
	path++;// Eliminating "/" in the path

	filler( buffer, ".", NULL, 0 ); // Current Directory
	filler( buffer, "..", NULL, 0 ); // Parent Directory
	

	for ( int curr_idx = 0; curr_idx <= curr_dir_idx; curr_idx++ )
	{
		printf("Count:%d\n",curr_idx);
		int path_flag = 1;
		
		if (strlen(dir_list[curr_idx]) <= strlen(path))
			continue;

		// Matching path prefix
		if(strlen(path) != 0)
		{
			for(int i = 0; i < strlen(path); i++)
			{	
				if (dir_list[curr_idx][i] != path[i] )
				{
					path_flag = 0;
					break;
				}
			}
		}

		// Checking current dir
		if (path_flag)
		{
			printf("prefix match!!\n");
			int surfix_start = (strlen(path) == 0) ? strlen(path) : strlen(path) + 1;
			for(int i = surfix_start; i < strlen(dir_list[curr_idx]); i++)
			{
				// Not in current dir
				if (dir_list[curr_idx][i] == '/')
				{
					printf("Not in current dir level.\n");
					path_flag = 0;
					break;
					
				}
			}
			//display dir 
			if (path_flag)
			{
				char *dirname = dir_list[curr_idx] + surfix_start;
				//strcpy(dirname, *(dir_list + curr_idx * 256 + strlen(path)));
				printf("%s\n", dirname);
				filler( buffer, dirname, NULL, 0 );
			}
		}
	}


	for ( int curr_idx = 0; curr_idx <= curr_file_idx; curr_idx++ )
	{
		int path_flag = 1;
		if (strlen(files_list[curr_idx]) < strlen(path))
			continue;
		// Matching path prefix
		if(strlen(path) != 0)
		{
			for(int i = 0; i < strlen(path); i++)
			{
				if (files_list[curr_idx][i] != path[i] )
				{
					path_flag = 0;
					break;
				}
			}
		}
		// Checking current dir
		if (path_flag)
		{
			int surfix_start = (strlen(path) == 0) ? strlen(path) : strlen(path) + 1;
			for(int i = surfix_start; i < strlen(files_list[curr_idx]); i++)
			{
				// Not in current dir
				if (files_list[curr_idx][i] == '/')
				{
					path_flag = 0;
					break;
					
				}
			}
			//display dir 
			if (path_flag)
			{
				char *dirname = files_list[curr_idx] + surfix_start;
				//strcpy(dirname, *(files_list + curr_idx * 256 + strlen(path)));
				printf("%s\n", dirname);
				filler( buffer, dirname, NULL, 0 );
			}
		}
	}
	
	return 0;
}

static int do_read( const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi )
{
	printf("do_read\n");
	int file_idx = get_file_index( path );
	
	if ( file_idx == -1 )
		return -1;
	
	char *content = files_content[ file_idx ];
	
	memcpy( buffer, content + offset, size );
		
	return strlen( content ) - offset;
}

static int do_mkdir( const char *path, mode_t mode )
{
	printf("do_mkdir\n");
	path++;// Eliminating "/" in the path
	add_dir( path );
	
	return 0;
}

static int do_mknod( const char *path, mode_t mode, dev_t rdev )
{
	printf("do_mknod\n");
	path++;// Eliminating "/" in the path
	add_file( path );
	
	return 0;
}

static int do_write( const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *info )
{
	printf("do_write\n");
	write_to_file( path, buffer );
	
	return size;
}

static int do_unlink (const char *path)/** Remove a file */
{
	int file_idx = get_file_index(path), count = 0, found = -1, delete;
	
	if(file_idx == -1) return -1;//file not found

	path++;// Eliminating "/" in the path

	for(int i = 0 ; i <= curr_file_idx; i++)
		if(strcmp(path, files_list[i]) == 0)
			found = 0;

	if(found == 0)
	{
		for(int i = file_idx; i < curr_file_idx && curr_file_idx == curr_file_content_idx; i++)//copy over the deleted index
		{
			strcpy(files_list[i], files_list[i+1]);//file list
			strcpy(files_content[i], files_content[i+1]);//file content list
			files_time[i][0] = files_time[i+1][0];
			files_time[i][1] = files_time[i+1][1];
		}
		curr_file_idx--;//shrink currnet size
		curr_file_content_idx--;//shrink current size
		return 0;
		
	}
	else return -1; //0: succeed, -1: false
}

	
static int do_rmdir (const char *path)/** Remove a directory */
{
	int dir_idx = get_dir_index(path), count = 0, found = -1;
	
	if(dir_idx == -1) return -1;//dir not found

	path++; // remove '/'

	for (int curr_idx = 0; curr_idx <= curr_dir_idx; curr_idx++) //case: dirs under a dir
	{
		// Matching path prefix
		for(int i = 0; i < strlen(path); i++)
		{
			if (dir_list[curr_idx][i] != path[i] )
			{
				count++;
				break;
			}
		}
			
		if(strcmp(path, dir_list[curr_idx]) == 0)//removable if cur_dir == delete target
			found = 0;
	}

	for(int curr_idx = 0 ; curr_idx <= curr_file_idx; curr_idx++)//not removable if files under a dir 
		// Matching path prefix
		for(int i = 0; i < strlen(path); i++)
		{
			if (files_list[curr_idx][i] != path[i] )
			{
				found = -1;
				break;
			}
		}
			

	if(count == 1 && found == 0)
	{
		strcpy(dir_list[dir_idx], "");//clear this string
		for(int i = dir_idx; i < curr_dir_idx; i++)//copy forward
		{
			strcpy(dir_list[i], dir_list[i+1]);
			dir_time[i][0] = dir_time[i+1][0];
			dir_time[i][1] = dir_time[i+1][1];
			dir_time[i][2] = dir_time[i+1][2];
		}
		//shrink
		curr_dir_idx--;
		return 0;
	}
	//0: succeed, -1: false
	else return -1; 
}

static int do_utime(const char *path, struct utimbuf *buf)
{
	printf("do_utime\n");
	int cur_index;
	buf->actime = time(NULL);
	buf->modtime = time(NULL);
	if(is_dir(path))
	{
		cur_index = get_dir_index(path);
		dir_time[cur_index][0] = buf->actime;
		dir_time[cur_index][1] = buf->modtime;
		return 0;
	}
	else if (is_file(path))
	{
		cur_index = get_file_index(path);
		files_time[cur_index][0] = buf->actime;
		files_time[cur_index][1] = buf->modtime;
		return 0;
	}
	else return -1;
}

static struct fuse_operations operations = {
    .getattr	= do_getattr,
    .readdir	= do_readdir,
    .read		= do_read,
    .mkdir		= do_mkdir,
    .mknod		= do_mknod,
    .write		= do_write,
	.unlink		= do_unlink,
    .rmdir		= do_rmdir,
    .utime		= do_utime,
};

int main( int argc, char *argv[] )
{
	return fuse_main( argc, argv, &operations, NULL );
}
