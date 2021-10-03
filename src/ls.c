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
#define COLOR_CODE_LENGTH 30
#define INIT_BUFF_LEN 100
#define MAX_LIST_SIZE 250

// Catgories are:  dir, link, socket, pipe, exec, block, char
#define CATEGORIES 7

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <linux/limits.h>
#include <sys/stat.h>

#include "colors.h"

struct prog_options
{
  char directory[PATH_MAX];
  int order_rev, show_hidden, print_color;
};

typedef struct
{
  char name[NAME_MAX];
  unsigned size;
  unsigned char type;
} file_info;

struct prog_options* options;

int entry_cmp(void const* p1, void const* p2)
{
  file_info left = *((file_info*) p1);
  file_info right = *((file_info*) p2);

  return strncmp(left.name, right.name, NAME_MAX);
}
int entry_cmp_r(void const* p1, void const* p2)
{
  file_info left = *((file_info*) p1);
  file_info right = *((file_info*) p2);

  return -strncmp(left.name, right.name, NAME_MAX);
}

void usage(FILE* fp)  // Print usage/help for the programm
{
  fprintf(fp, "usage: ls [OPTIONS] or ls <directory>\n");
  fprintf(fp, "\t-a\tShow all files (including 'hidden' ones)\n");
  fprintf(fp, "\t-h\tPrints this help\n");
  fprintf(fp, "\t-r\tReverse the sorting of the list\n");
}

// TODO: Find out how to get all the colors (like gray,brown etc.)
char* ls_to_ansi_color(char ls, int fg)
{
  switch (ls)
  {
    case 'a':
      if (fg)
        return BLK;
      else
        return BLKB;
    case 'b':
      if (fg)
        return RED;
      else
        return REDB;
    case 'c':
      if (fg)
        return GRN;
      else
        return GRNB;
    case 'd':
      if (fg)
        return HGRN;
      else
        return GRNHB;
    case 'e':
      if (fg)
        return BLU;
      else
        return BLUB;
    case 'f':
      if (fg)
        return MAG;
      else
        return MAGB;
    case 'g':
      if (fg)
        return CYN;
      else
        return CYNB;
    case 'h':
      if (fg)
        return HGRN;
      else
        return GRNHB;
    case 'A':
      if (fg)
        return BBLK;
      else
        return BLKHB;
    case 'B':
      if (fg)
        return BRED;
      else
        return REDHB;
    case 'C':
      if (fg)
        return BGRN;
      else
        return GRNHB;
    case 'D':
      if (fg)
        return BHGRN;
      else
        return GRNHB;
    case 'E':
      if (fg)
        return BBLU;
      else
        return BLUHB;
    case 'F':
      if (fg)
        return BMAG;
      else
        return MAGB;
    case 'G':
      if (fg)
        return BCYN;
      else
        return CYNB;
    case 'H':
      if (fg)
        return BHGRN;
      else
        return GRNHB;
    case 'x':
      return "";
    default:
      return "";
  }
}

int main(const int argc, char* const argv[])
{
  // parse options
  options = malloc(sizeof(*options));
  options->order_rev = 0;
  options->show_hidden = 0;
  options->print_color = 0;

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

    while ((c = getopt(argc, argv, "rahc")) != -1)
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

  file_info* entries = calloc(MAX_LIST_SIZE, sizeof(*entries));
  int entry_idx = 0;

  dir_p = opendir(options->directory);
  if (dir_p != NULL)
  {
    while ((entry_p = readdir(dir_p)))
    {
      // entry_p->d_name should always be shorter then (linux limits)
      entries[entry_idx].type = entry_p->d_type;
      entries[entry_idx].size = entry_p->d_reclen;

      strncpy(entries[entry_idx].name, entry_p->d_name, NAME_MAX);
      ++entry_idx;
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
    qsort(entries, entry_idx, sizeof(*entries), entry_cmp_r);
  else
    qsort(entries, entry_idx, sizeof(*entries), entry_cmp);

  // check for LSCOLORS
  int print_colors = 1;

  char** colors = malloc(CATEGORIES * sizeof(*colors));
  for (int i = 0; i < CATEGORIES; i++)
    colors[i] = malloc(COLOR_CODE_LENGTH * sizeof(**colors));

  const char* colors_config = getenv("LSCOLORS");
  if (!colors_config)
  {
    print_colors = 0;
  } else
  {
    if (strnlen(colors_config, 12) != 12)
    {
      fprintf(stderr, "ERROR: Failed trying to parse $LSCOLORS\n");
      closedir(dir_p);
      free(entries);
      free(options);
      free(buff);
      return EXIT_FAILURE;
    }

    // Parsing all the color codes, order stems from the man pages for readdir()
    //
    // Color for block devices
    for (int i = 0; i < CATEGORIES; i++)
    {
      char fg = colors_config[i * 2];
      char bg = colors_config[i * 2 + 1];

      strncat(colors[i], ls_to_ansi_color(fg, 1), 20);
      strncat(colors[i], ls_to_ansi_color(bg, 0), 20);
    }
  }

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

      if (print_colors)
      {
        char* color_code = malloc(20 * sizeof(*color_code));

        // TODO: make this proper, and add all the colors
        switch (entries[i].type)
        {
          case DT_REG: {
            struct stat sb;
            if (stat(entries[i].name, &sb) == 0 &&
                sb.st_mode & S_IXUSR)  // is executable
              strncpy(color_code, colors[4], 20);
            else
              strncpy(color_code, "", 20);
            break;
          }
          case DT_DIR:
            strncpy(color_code, colors[0], 20);
            break;
          case DT_LNK:
            strncpy(color_code, colors[1], 20);
            break;
          case DT_SOCK:
            strncpy(color_code, colors[2], 20);
            break;
          case DT_FIFO:
            strncpy(color_code, colors[3], 20);
            break;
          case DT_BLK:
            strncpy(color_code, colors[5], 20);
            break;
          case DT_CHR:
            strncpy(color_code, colors[6], 20);
            break;
        }

        int cl_l = strnlen(color_code, 20);

        if (cl_l + idx >= b_len)
        {
          buff = realloc(buff, b_len * 2);
          b_len *= 2;
        }

        strncpy(&buff[idx], color_code, cl_l);
        idx += cl_l;
      }

      strncpy(&buff[idx], entries[i].name, l);

      idx += l;
      buff[idx] = '\t';
      ++idx;

      // put in reset code
      int rs_l = strnlen(reset, 10);

      if (rs_l + idx >= b_len)
      {
        buff = realloc(buff, b_len * 2);
        b_len *= 2;
      }

      strncpy(&buff[idx],reset,rs_l);
    }
  }
  puts(buff);

  closedir(dir_p);
  free(entries);

  free(options);
  free(buff);

  return EXIT_SUCCESS;
}
