#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <getopt.h>
#include <taglib/tag_c.h>
#include <dirent.h>

enum errornumbers
{
    success,
    misc_error,
    no_arguments,
    invalid_argument,
    getextension_failure,
    organize_failure,
    extension_compare_failure
};

int input(char *text, char *str, int max_len)
{
    strcpy(str, "");
    printf("%s", text);
    printf("");
    if (fgets(str, max_len, stdin) != NULL)
    {
        for (int i = 0; i < max_len; i++)
        {
            if (str[i] == '\n')
            {
                str[i] == '\0';
                break;
            }
        }
        // Retorna 1 porque eu uso como comparador em ifs
        return 1;
    }
    else
    {
        return 0;
    }
}

int create_directory(const char *path, int simulated)
{
#ifdef _WIN32
    _mkdir(path);
#else
    mkdir(path, 0777);
#endif
    return success;
}

int extensionCompare(char *extension, char *compare_ext)
{
    if (strcmp(extension, compare_ext) == 0)
    {
        return success;
    }
    return extension_compare_failure;
}

int allExtensionsCompare(char *extension)
{
    int fail = extension_compare_failure;
    if (extensionCompare(extension, ".mp3") == fail && extensionCompare(extension, ".ogg") == fail && extensionCompare(extension, ".flac") == fail && extensionCompare(extension, ".wav") == fail)
    {
        return fail;
    }
    return success;
}

int getExtension(char *file, char *extension)
{
    int extensionIndex = -1;

    int i;
    for (i = strlen(file) - 1; i >= 0; i--)
    {
        if (file[i] == '.')
        {
            extensionIndex = i;
            break;
        }
    }

    if (extensionIndex < 0)
    {
        return getextension_failure;
    }

    int j = 0;
    for (i = extensionIndex; i < strlen(file); i++)
    {
        extension[j] = file[i];
        j++;
    }

    extension[j] = '\0';

    if (allExtensionsCompare(extension) == extension_compare_failure)
    {
        return extension_compare_failure;
    }
    return success;
}

int organize_audio_file(char *file_path, char *output_dir, char *extension, char *artist, char *album, char *title, int debug, int verbose, int simulated)
{
    int exitcode = success;
    char c = '/';
    char and = '&';
    char comma = ',';
    char semicolon = ';';
    int i;

    for (i = strlen(artist) - 1; i >= 0; i--)
    {
        if (artist[i] == c || artist[i] == and || artist[i] == comma || artist[i] == semicolon)
        {
            char *new_artist = malloc(strlen(artist) * sizeof(char));
            strncpy(new_artist, artist, i);
            strcpy(artist, new_artist);
            free(new_artist);
        }
    }
    if (artist[strlen(artist) - 1] == ' ')
    {
        artist[strlen(artist) - 1] = '\0';
    }

    for (i = strlen(album) - 1; i >= 0; i--)
    {
        char c = '/';
        if (album[i] == c || album[i] == and || album[i] == comma || album[i] == semicolon)
        {
            album[i] = '-';
        }
    }
    for (i = strlen(title) - 1; i >= 0; i--)
    {
        char c = '/';
        if (title[i] == c || title[i] == and || title[i] == comma || title[i] == semicolon)
        {
            title[i] = '-';
        }
    }

    char *output_dir_path = malloc(sizeof(char) * (strlen(output_dir) + 3));

    sprintf(output_dir_path, "./%s", output_dir);

    create_directory(output_dir_path, simulated);

    char *artist_path = malloc(sizeof(char) * (strlen(output_dir_path) + strlen(artist) + 2));

    sprintf(artist_path, "%s/%s", output_dir_path, artist);

    create_directory(artist_path, simulated);

    char *album_path = malloc(sizeof(char) * (strlen(artist_path) + strlen(album) + 2));

    sprintf(album_path, "%s/%s", artist_path, album);

    create_directory(album_path, simulated);

    char *new_file_path = malloc(sizeof(char) * (strlen(artist) + strlen(album) + strlen(title) + strlen(extension) + 10));

    sprintf(new_file_path, "./%s/%s/%s/%s%s", output_dir, artist, album, title, extension);

    if (debug)
    {
        printf("Debug: artist_path: %s\n", artist_path);
        printf("Debug: album_path: %s\n", album_path);
        printf("Debug: file_path: %s\n", file_path);
        printf("Debug: new_file_path: %s\n", new_file_path);
    }

    if (simulated)
    {
        printf("File would be moved and renamed to: %s\n", new_file_path);
    }
    else
    {
        if (rename(file_path, new_file_path) == success)
        {
            if (verbose)
            {
                printf("File moved and renamed to: %s\n", new_file_path);
            }
        }
        else
        {
            printf("Error moving or renaming the file %s.\n", file_path);
            exitcode = organize_failure;
        }
    }

    free(output_dir_path);
    free(artist_path);
    free(album_path);
    free(new_file_path);

    return exitcode;
}

int print_help(const char *program_name)
{
    printf("Usage: %s [options] <file1> [file2] [file3] ...\n", program_name);
    printf("Options:\n");
    printf("  --debug          Enable debug mode\n");
    printf("  -h, --help       Display this help message\n");
    printf("  -v, --verbose    Display any change\n");
    printf("  -o, --output     Choose output directory name\n");
    printf("  -s, --simulated  Simulate output\n");
    return success;
}

int process_file(char *name, char *output_dir, int debug, int verbose, int simulated)
{
    if (verbose)
    {
        printf("%s is a file.\n", name);
    }

    char *extension = malloc(10);
    if (getExtension(name, extension) == getextension_failure)
    {
        printf("Error getting extension of file %s.\n", name);
        return misc_error;
    }

    if (allExtensionsCompare(extension) == extension_compare_failure)
    {
        if(verbose)
            printf("Error: File %s has an invalid extension.\n", name);
        return invalid_argument;
    }

    TagLib_File *file = taglib_file_new(name);
    if (file == NULL)
    {
        printf("Error opening file %s.\n", name);
        return misc_error;
    }

    TagLib_Tag *tag = taglib_file_tag(file);
    if (tag == NULL)
    {
        printf("Error getting tag of file %s.\n", name);
        return misc_error;
    }

    char *artist = malloc(1024 * sizeof(char));
    char *album = malloc(1024 * sizeof(char));
    char *title = malloc(1024 * sizeof(char));

    strcpy(artist, taglib_tag_artist(tag));
    strcpy(album, taglib_tag_album(tag));
    strcpy(title, taglib_tag_title(tag));

    if (artist == NULL || album == NULL || title == NULL)
    {
        printf("Error getting artist, album or title of file %s.\n", name);
        return misc_error;
    }

    if (organize_audio_file(name, output_dir, extension, artist, album, title, debug, verbose, simulated) != success)
    {
        return misc_error;
    }

    taglib_tag_free_strings();
    taglib_file_free(file);
    free(extension);
    free(artist);
    free(album);
    free(title);
    return success;
}

int process_directory(const char *dir_path, char *output_dir, int debug, int verbose, int simulated, int recursive)
{
    DIR *dir = opendir(dir_path);
    if (!dir)
    {
        fprintf(stderr, "Error opening directory: %s\n", dir_path);
        return misc_error;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_DIR)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            char sub_dir_path[PATH_MAX];
            snprintf(sub_dir_path, sizeof(sub_dir_path), "%s/%s", dir_path, entry->d_name);

            if (process_directory(sub_dir_path, output_dir, debug, verbose, simulated, recursive) != success)
            {
                closedir(dir);
                return misc_error;
            }
        }
        else if (entry->d_type == DT_REG)
        {
            char file_path[PATH_MAX];
            snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, entry->d_name);

            int file_result = process_file(file_path, output_dir, debug, verbose, simulated);
            if (file_result != success && file_result != invalid_argument)
            {
                closedir(dir);
                return misc_error;
            }
        }
    }

    closedir(dir);
    return success;
}

int main(int argc, char *argv[])
{
    int debug = 0;
    int verbose = 0;
    int simulated = 0;
    int recursive = 0;
    char *output_dir;
    strcpy(output_dir, "out");

    static struct option long_options[] = {
        {"debug", no_argument, 0, 0},
        {"verbose", no_argument, 0, 'v'},
        {"simulated", no_argument, 0, 's'},
        {"recursive", no_argument, 0, 'r'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}};

    int option_index = 0;
    int opt;
    int opt_index = 1;

    while ((opt = getopt_long(argc, argv, "vshr", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
        case 0:
            if (strcmp(long_options[option_index].name, "debug") == 0)
            {
                debug = 1;
            }
            break;
        case 'v':
            verbose = 1;
            break;
        case 'h':
            print_help(argv[0]);
            return success;
        case 's':
            simulated = 1;
            break;
        case 'r':
            recursive = 1;
            break;
        case '?':
            fprintf(stderr, "Unknown option -%c\n", optopt);
            return invalid_argument;
        }
        opt_index++;
    }

    char choice;
    while (1)
    {
        if (input("This operation is irreversible. Are you sure? (y/N): ", &choice, 128))
        {
            if (choice == 'y' || choice == 'Y')
            {
                break;
            }
            else if (choice == 'n' || choice == 'N' || choice == '\0')
            {
                return success;
            }
        }
    }

    if (argc == optind)
    {
        print_help(argv[0]);
        return no_arguments;
    }

    for (int i = optind; i < argc; ++i)
    {
        if (!(strcmp(argv[i], output_dir)))
        {
            continue;
        }
        struct stat file_stat;
        if (stat(argv[i], &file_stat) == 0 && S_ISDIR(file_stat.st_mode))
        {
            if (verbose)
            {
                printf("%s is a directory.\n", argv[i]);
            }

            process_directory(argv[i], output_dir, debug, verbose, simulated, recursive);
        }
        else
        {
            process_file(argv[i], output_dir, debug, verbose, simulated);
        }
    }
    printf("Done.\nSongs organized in ./%s.\n", output_dir);
    return success;
}