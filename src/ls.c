/******************************************************************************
 * FILE:
 *				ls.c
 *
 * DESCRIPTION:
 *				A small program to list directiories and files
 *on the command line
 *
 * AUTHOR:
 *				Jakob Sachs (jakobsachs1999@gmail.com)
 *
 * DATE:
 *				03.10.2021
 *
 ******************************************************************************/
#define INIT_BUFF_LEN 100
#define MAX_LIST_SIZE 250

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef struct
{
  char* name;
  size_t size;
  time_t time;
} file_record;

struct prog_options
{
  char directory[PATH_MAX];
  int order_rev, show_hidden;
};

struct prog_options* options;

int entry_cmp(void const* p1, void const* p2)
{
  file_record left = *((file_record*) p1);
  file_record right = *((file_record*) p2);
	
  return strncasecmp(left.name, right.name, NAME_MAX);
}
int entry_cmp_r(void const* p1, void const* p2)
{
  file_record left = *((file_record*) p1);
  file_record right = *((file_record*) p2);

  return -strncasecmp(left.name, right.name, NAME_MAX);
}

void usage(FILE* fp)  // Print usage/help for the programm
{
  fprintf(fp, "usage: ls [OPTIONS] or ls <directory>\n");
  fprintf(fp, "\t-a\tShow all files (including 'hidden' ones)\n");
  fprintf(fp, "\t-h\tPrints this help\n");
  fprintf(fp, "\t-r\tReverse the sorting of the list\n");
}

int main(const int argc, char* const argv[])
{
  // parse options
  options = malloc(sizeof(*options));
  options->order_rev = 0;
  options->show_hidden = 0;

  if (argc == 1)
  {
    if (!getcwd(options->directory, PATH_MAX))
    {
      free(options);
      fprintf(stderr, "ERROR: Failed to get current working directory!\n");
      return EXIT_FAILURE;
    }
  } else
  {
    int c;
    opterr = 0;

    while ((c = getopt(argc, argv, "rah")) != -1)
    {
      switch (c)
      {
        case 'r':
          options->order_rev = 1;
          break;

        case 'a':
          options->show_hidden = 1;
          break;

        case 'h':
          usage(stdout);
          free(options);
          return EXIT_SUCCESS;
          break;

        case '?':
          fprintf(stderr, "ERROR: unrecognized argument %s\n",
                  argv[optind - 1]);

          usage(stderr);
          free(options);
          return EXIT_FAILURE;
          break;
      }
    }

    if (optind + 1 == argc)
    {
      if (argv[optind][0] == '/')  // check if path is absolute
      {
        strncpy(options->directory, argv[optind], PATH_MAX);
      } else  // if relative
      {
        if (!getcwd(options->directory, PATH_MAX))
        {
          free(options);
          fprintf(stderr, "ERROR: Failed to get current working directory!\n");
          return EXIT_FAILURE;
        }

        int l = strnlen(options->directory, PATH_MAX);
        options->directory[l] = '/';
        ++l;

        if (l + strnlen(argv[optind], PATH_MAX) >= PATH_MAX)
        {
          fprintf(stderr, "ERROR: Path to long");
        }

        strncpy(&options->directory[l], argv[optind], PATH_MAX - l);
      }
    } else
    {
      if (!getcwd(options->directory, PATH_MAX))
      {
        free(options);
        fprintf(stderr, "ERROR: Failed to get current working directory!\n");
        return EXIT_FAILURE;
      }
    }
  }

  size_t b_len = INIT_BUFF_LEN;
  char* buff = malloc(sizeof(*buff) * b_len);
  size_t idx = 0;

  // Loop over directory
  DIR* dir_p;
  struct dirent* entry_p;

  file_record* entries = malloc(MAX_LIST_SIZE * sizeof(file_record));
  for (int i = 0; i < MAX_LIST_SIZE; i++)
    entries[i].name = malloc(NAME_MAX * sizeof(char*));

  int entry_idx = 0;

  dir_p = opendir(options->directory);
  if (dir_p != NULL)
  {
    while ((entry_p = readdir(dir_p)))
    {
      if (strncmp(entry_p->d_name, ".", NAME_MAX) != 0)
      {
        strncpy(entries[entry_idx].name, entry_p->d_name, NAME_MAX);

        struct stat st_bf;
        stat(entry_p->d_name, &stat);

				entries[entry_idx].size = st_bf.st_size;
				entries[entry_idx].time = st_bf.st_mtime;
				
        ++entry_idx;
      }
    }
  } else
  {
    // TODO: add additional information to the error report
    // like 'is not a directory' , etc.
    fprintf(stderr, "ERROR: Failed to open directory: %s\n",
            options->directory);
    free(options);
    return EXIT_FAILURE;
  }

  // sort entries
  if (options->order_rev)
    qsort(entries, entry_idx, sizeof(file_record), entry_cmp_r);
  else
    qsort(entries, entry_idx, sizeof(file_record), entry_cmp);

  for (int i = 0; i < entry_idx; i++)
  {
    // assemble buff
    if (entries[i].name[0] != '.' || options->show_hidden)
    {
      int l = strnlen(entries[i].name, NAME_MAX);
      if (l + idx >= b_len)  // check for buff size
      {
        buff = realloc(buff, b_len * 2);
        b_len *= 2;
      }
      strncpy(&buff[idx], entries[i].name, l);
      idx += l;
      buff[idx] = '\t';
      ++idx;
    }
  }
  puts(buff);

  closedir(dir_p);

  for (int i = 0; i < MAX_LIST_SIZE; i++) free(entries[i].name);
  free(entries);

  free(options);
  free(buff);

  return EXIT_SUCCESS;
}
