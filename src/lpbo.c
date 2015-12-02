/* lpbo.c - (C) 2015, Emir Marincic
 * lpbo - A tool to work with Arma pbo files.
 * See README for contact-, COPYING for license information. */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#include <getopt.h>

#ifdef HAVE_DIRECT_H
    #include <direct.h>
#endif

#include <libpbo/pbo.h>

enum mode
{
	UNKNOWN = 0,	//None of the following
	LIST,			// -l
	EXTRACT,		// -x
	CREATE			// -c
};

enum mode g_mode;
const char *g_program_name;
const char *g_file;
const char *g_dir;

#define USAGE_E(STR)\
				fputs(STR "\n", stderr); \
				usage(EXIT_FAILURE);

void usage(int ec);

void makedir(const char *path)
{
	#ifdef HAVE_DIRECT_H
	    mkdir(path);
	#else
	    mkdir(path, 0755);
	#endif
}

void create_directories(char *path)
{    
    for(int i = 0; path[i] != '\0'; i++) {
        if(path[i] == '/') {
            path[i] = '\0';
            makedir(path);
            path[i] = '/';
        }
    }
}

void set_mode(enum mode mode)
{
	if(g_mode != UNKNOWN){
		USAGE_E("You may not specify more then one '-xcl' option.")
	}
	g_mode = mode;
}

void set_file(const char *file)
{
	if(g_file){
		USAGE_E("You may not specify multiple files.")
	}
	g_file = file;
}

void usage(int ec)
{
	FILE *str = ec == EXIT_FAILURE ? stderr : stdout;
	#define O(STR) fputs(STR "\n", str)
	#define F(STR, ...) fprintf(str, STR "\n", __VA_ARGS__)
	
	if(ec == EXIT_FAILURE){
		F("Try '%s -h' for more information.", g_program_name);
		exit(ec);
	}

	O("===============================");
	O("== lpbo - An Arma Pbo editor ==");
	O("===============================");
	F("usage: %s [-lxcfCh] [FILE]...", g_program_name);
	O("");
	O("\t-l : List contents of file.");
	O("\t-x : Extract contents of file.");
	O("\t-c : Create a new pbo.");
	O("\t-f <str>: Use pbo file.");
	O("\t-C <str>: Change to directory.");
	O("\t-h : Display this.");
	O("");
	O("(C) 2015 Emir Marincic");

	#undef O
	#undef F
}

void process_args(int *argc, char ***argv)
{
	int c = 0;
	while((c = getopt(*argc, *argv, "lxcf:h")) != -1)
	{
		switch(c) {
		case 'l':
			set_mode(LIST);
			break;
		case 'x':
			set_mode(EXTRACT);
			break;
		case 'c':
			set_mode(CREATE);
			break;
		case 'f':
			set_file(optarg);
			break;
		case 'h':
			usage(EXIT_SUCCESS);
			break;
		case '?':
		default:
			USAGE_E("Error")
		}
	}
	*argc -= optind;
	*argv += optind;
}

void list_files_cb(const char *file, void *user)
{
	if(file[0] == '\0')
		return;
	printf("%s\n", file);
}

void list_files()
{
	pbo_t d = pbo_init(g_file);
	pbo_read_header(d);
	pbo_get_file_list(d, list_files_cb, NULL);
	pbo_dispose(d);
}

void extract_files_cb(const char *filename, void *user)
{
	if(filename[0] == '\0')
		return;

	pbo_t d = (pbo_t)user;

	char buf[512];
	strcpy(buf, filename);

	for(int i = 0; buf[i] != '\0'; i++)
		if(buf[i] == '\\')
			buf[i] = '/';

	create_directories(buf);

	FILE *file = fopen(buf, "w");
	if(!file)
		return;
	pbo_write_to_file(d, filename, file);
	fclose(file);
}

void extract_files()
{
	pbo_t d = pbo_init(g_file);
	pbo_read_header(d);
	pbo_get_file_list(d, extract_files_cb, d);
	pbo_dispose(d);
}

void create_pbo(int fcount, char** files)
{
	pbo_t d = pbo_init(g_file);
	pbo_init_new(d);

	for(int i = 0; i < fcount; i++) {
		if(files[i][0] == '$') { //Header extension
			return; //DO SMTH HERE
		}
		char buf[512];
		strcpy(buf, files[i]);

		for(int j = 0; buf[j] != '\0'; j++)
			if(buf[j] == '/')
				buf[j] = '\\';

		pbo_add_file_p(d, buf, files[i]);
	}
	pbo_write(d);
	pbo_dispose(d);
}

int main(int argc, char **argv)
{
	g_mode = UNKNOWN;
	g_program_name = argv[0];

	process_args(&argc, &argv);

	switch(g_mode) {
	case UNKNOWN:
		USAGE_E("You must specify one of the '-xcl' options")
		break;
	case LIST:
		list_files();
		break;
	case EXTRACT:
		extract_files();
		break;
	case CREATE:
		create_pbo(argc, argv);
		break;
	}

	exit(EXIT_SUCCESS);
}