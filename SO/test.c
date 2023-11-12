#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>
#include <sys/time.h>
#include <time.h>

void writeStats(int output_file, const char *filename, int width, int height)
{
    struct stat st;
    if (stat(filename, &st) == -1)
    {
        perror("Error at stat");
        exit(EXIT_FAILURE);
    }

    char modifDate[40];
    strftime(modifDate, sizeof(modifDate), "%d.%m.%Y", localtime(&st.st_mtime));

    char buffer[256];
    int dim = snprintf(buffer, sizeof(buffer), "nume fisier: %s\ninaltime: %d\nlungime: %d\ndimensiune: %ld\n", filename, height, width, st.st_size);
    if (write(output_file, buffer, dim) == -1)
    {
        perror("Error at writing the name on the output file");
        exit(EXIT_FAILURE);
    }

    dim = snprintf(buffer, sizeof(buffer), "identificatorul utilizatorului: %d\ntimpul ultimei modificari: %s\ncontorul de legaturi: %ld\n", getuid(), modifDate, st.st_nlink);
    if (write(output_file, buffer, dim) == -1)
    {
        perror("Error at writing to the output file");
        exit(EXIT_FAILURE);
    }

    dim = snprintf(buffer, sizeof(buffer), "drepturi de acces user: %c%c%c\ndrepturi de acces grup: %c%c%c\ndrepturi de acces altii: %c%c%c\n",
                   (st.st_mode & S_IRUSR) ? 'R' : '-',
                   (st.st_mode & S_IWUSR) ? 'W' : '-',
                   (st.st_mode & S_IXUSR) ? 'X' : '-',
                   (st.st_mode & S_IRGRP) ? 'R' : '-',
                   (st.st_mode & S_IWGRP) ? 'W' : '-',
                   (st.st_mode & S_IXGRP) ? 'X' : '-',
                   (st.st_mode & S_IROTH) ? 'R' : '-',
                   (st.st_mode & S_IWOTH) ? 'W' : '-',
                   (st.st_mode & S_IXOTH) ? 'X' : '-');
    if (write(output_file, buffer, dim) == -1)
    {
        perror("Error at writing to the output file");
        exit(EXIT_FAILURE);
    }
}

void processEntry(int output_file, const char *entry_name, const char *parent_dir) {
    if (parent_dir == NULL) {
        fprintf(stderr, "Parent directory is NULL. Entry name: %s\n", entry_name);
        exit(EXIT_FAILURE);
    }

    char full_path[PATH_MAX];
    if (snprintf(full_path, sizeof(full_path), "%s/%s", parent_dir, entry_name) >= sizeof(full_path)) {
        fprintf(stderr, "Path too long. Entry name: %s\n", entry_name);
        exit(EXIT_FAILURE);
    }

    struct stat st;
    if (lstat(full_path, &st) == -1) {
        perror("Error at lstat");
        fprintf(stderr, "Error processing entry: %s\n", full_path);
        exit(EXIT_FAILURE);
    }
    if (S_ISDIR(st.st_mode))
    {
        char buffer[256];
        int dim = snprintf(buffer, sizeof(buffer), "nume director: %s\nidentificatorul utilizatorului: %d\ndrepturi de acces user: RWX\ndrepturi de acces grup: R--\ndrepturi de acces altii: ---\n", entry_name, getuid());
        if (write(output_file, buffer, dim) == -1)
        {
            perror("Error at writing to the output file");
            exit(EXIT_FAILURE);
        }

        DIR *dir = opendir(full_path);
        if (dir == NULL)
        {
            perror("Error at opendir");
            exit(EXIT_FAILURE);
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL)
        {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
            {
                processEntry(output_file, entry->d_name, full_path);
            }
        }

        closedir(dir);
    }
    else if (S_ISREG(st.st_mode))
    {
        if (strstr(entry_name, ".bmp") != NULL)
        {
            int input_file = open(full_path, O_RDONLY);
            if (input_file == -1)
            {
                perror("Error at opening the input file");
                exit(EXIT_FAILURE);
            }

            char buffer[256];
            int dim = snprintf(buffer, sizeof(buffer), "Nume fisier: %s\n", full_path);
            if (write(output_file, buffer, dim) == -1)
            {
                perror("Error at writing the name on the output file");
                exit(EXIT_FAILURE);
            }

            int width, height;
            if (read(input_file, buffer, 18) != -1)
            {
                read(input_file, &width, 4);
                read(input_file, &height, 4);
            }
            else
            {
                perror("Cannot read the first 18 bytes");
                exit(EXIT_FAILURE);
            }

            writeStats(output_file, full_path, width, height);

            close(input_file);
        }
        else
        {
            char buffer[256];
            int dim = snprintf(buffer, sizeof(buffer), "Nume fisier: %s\n", full_path);
            if (write(output_file, buffer, dim) == -1)
            {
                perror("Error at writing the name on the output file");
                exit(EXIT_FAILURE);
            }

            writeStats(output_file, full_path, -1, -1);
        }
    }
    else if (S_ISLNK(st.st_mode))
    {
        char target_path[PATH_MAX];
        ssize_t target_size = readlink(full_path, target_path, sizeof(target_path) - 1);
        if (target_size == -1)
        {
            perror("Error at readlink");
            exit(EXIT_FAILURE);
        }

        target_path[target_size] = '\0';

        char buffer[256];
        int dim = snprintf(buffer, sizeof(buffer), "nume legatura: %s\ndimensiune legatura: %ld\ndimensiune fisier dimensiunea fisierului target: %ld\n", entry_name, st.st_size, strlen(target_path));
        if (write(output_file, buffer, dim) == -1)
        {
            perror("Error at writing to the output file");
            exit(EXIT_FAILURE);
        }

        dim = snprintf(buffer, sizeof(buffer), "drepturi de acces user legatura: RWX\ndrepturi de acces grup legatura: R--\ndrepturi de acces altii legatura: ---\n");
        if (write(output_file, buffer, dim) == -1)
        {
            perror("Error at writing to the output file");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        fprintf(stderr, "Entry is neither a file, directory, nor a symbolic link: %s\n", full_path);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <director_intrare>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *input_directory = argv[1];
    int output_file = open("statistica.txt", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (output_file == -1) {
        perror("The output file cannot be opened");
        exit(EXIT_FAILURE);
    }

    processEntry(output_file, ".", input_directory);

    close(output_file);

    return 0;
}

