#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <getopt.h>
#include <taglib/tag_c.h>

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

int create_directory(const char *path)
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

int organize_audio_file(char *file_path, char *extension, char *artist, char *album, char *title, int debug, int verbose, int simulated)
{
    int exitcode = success;
    char c = '/';
    int i;
    for (i = strlen(artist) - 1; i >= 0; i--)
    {
        if (artist[i] == c)
        {
            artist[i] = '-';
        }
    }
    for (i = strlen(album) - 1; i >= 0; i--)
    {
        char c = '/';
        if (album[i] == c)
        {
            album[i] = '-';
        }
    }
    for (i = strlen(title) - 1; i >= 0; i--)
    {
        char c = '/';
        if (title[i] == c)
        {
            title[i] = '-';
        }
    }

    char *artist_path = malloc(strlen(artist) + 3);
    char *album_path = malloc(strlen(artist) + strlen(album) + 4);

    sprintf(artist_path, "./%s", artist);
    sprintf(album_path, "./%s/%s", artist, album);

    create_directory(artist_path);
    create_directory(album_path);

    char *new_file_path = malloc(strlen(artist) + strlen(album) + strlen(title) + strlen(extension) + 8);

    sprintf(new_file_path, "./%s/%s/%s%s", artist, album, title, extension);

    if (debug)
    {
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
    printf("  -v, --verbose    Display this help message\n");
    printf("  -s, --simulated  Display this help message\n");
    return success;
}

int main(int argc, char *argv[])
{
    int debug = 0;
    int verbose = 0;
    int simulated = 0;

    static struct option long_options[] = {
        {"debug", no_argument, 0, 0},
        {"verbose", no_argument, 0, 'v'},
        {"simulated", no_argument, 0, 's'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}};

    int option_index = 0;
    int opt;

    while ((opt = getopt_long(argc, argv, "vsh", long_options, &option_index)) != -1)
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
            return 0;
        case 's':
            simulated = 1;
            break;
        case '?':
            fprintf(stderr, "Unknown option -%c\n", optopt);
            return invalid_argument;
        }
    }

    if (argc == optind)
    {
        print_help(argv[0]);
        return no_arguments;
    }

    char extension[128];
    for (int i = optind; i < argc; ++i)
    {
        TagLib_File *file = taglib_file_new(argv[i]);
        TagLib_Tag *tag = taglib_file_tag(file);
        char *artist = taglib_tag_artist(tag);
        char *album = taglib_tag_album(tag);
        char *title = taglib_tag_title(tag);

        int ext_check = getExtension(argv[i], extension);

        if (ext_check == success)
        {
            if (organize_audio_file(argv[i], extension, artist, album, title, debug, verbose, simulated) == organize_failure)
            {
                return organize_failure;
            }
        }
        else
        {
            if (ext_check == getextension_failure)
            {
                return getextension_failure;
            }
            else if (ext_check == extension_compare_failure)
            {
                continue;
            }
            else
            {
                return misc_error;
            }
        }

        taglib_tag_free_strings();
        taglib_file_free(file);
    }

    return success;
}
