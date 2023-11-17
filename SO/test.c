#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>

#define PATH_MAX_LEN 256

int process_file(const char *file_path, int stat_fd);

void process_directory(const char *dir_path, int stat_fd);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <director_intrare>\n", argv[0]);
        exit(1);
    }

    const char *director_intrare = argv[1];

    int stat_fd = open("statistica.txt", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if (stat_fd == -1) {
        perror("Eroare la crearea fisierului de statistica");
        exit(1);
    }

    process_directory(director_intrare, stat_fd);

    close(stat_fd);

    return 0;
}

int process_file(const char *file_path, int stat_fd) {
    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("Eroare la deschiderea fisierului");
        return -1;
    }

    struct stat file_info;
    if (fstat(fd, &file_info) == -1) {
        perror("Eroare la obtinerea informatiilor despre fisier");
        close(fd);
        return -1;
    }

    char nume_fisier[256];
    strcpy(nume_fisier, file_path);

    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%d.%m.%Y", localtime(&file_info.st_mtime));

    mode_t file_mode = file_info.st_mode;
    char user_permissions[4], group_permissions[4], others_permissions[4];

    user_permissions[0] = (file_mode & S_IRUSR) ? 'R' : '-';
    user_permissions[1] = (file_mode & S_IWUSR) ? 'W' : '-';
    user_permissions[2] = (file_mode & S_IXUSR) ? 'X' : '-';
    user_permissions[3] = '\0';

    group_permissions[0] = (file_mode & S_IRGRP) ? 'R' : '-';
    group_permissions[1] = (file_mode & S_IWGRP) ? 'W' : '-';
    group_permissions[2] = (file_mode & S_IXGRP) ? 'X' : '-';
    group_permissions[3] = '\0';

    others_permissions[0] = (file_mode & S_IROTH) ? 'R' : '-';
    others_permissions[1] = (file_mode & S_IWOTH) ? 'W' : '-';
    others_permissions[2] = (file_mode & S_IXOTH) ? 'X' : '-';
    others_permissions[3] = '\0';

    char statistica[512];
    if (S_ISLNK(file_info.st_mode)) {
        char target_path[256];
        ssize_t target_len = readlink(file_path, target_path, sizeof(target_path) - 1);
        if (target_len == -1) {
            perror("Eroare la citirea legaturii simbolice");
            close(fd);
            return -1;
        }
        target_path[target_len] = '\0';

        sprintf(statistica, "nume legatura: %s\ndimensiune legatura: %lu\ndimensiune fisier target: %lu\ndrepturi de acces user legatura: %s\ndrepturi de acces grup legatura: %s\ndrepturi de acces altii legatura: %s\n\n",
                nume_fisier, file_info.st_size, file_info.st_size, user_permissions, group_permissions, others_permissions);

        if (write(stat_fd, statistica, strlen(statistica)) == -1) {
            perror("Eroare la scriere");
            close(fd);
            return -1;
        }
    } else {
        sprintf(statistica, "nume fisier: %s\ndimensiune: %lu\nidentificatorul utilizatorului: %d\ntimpul ultimei modificari: %s\ncontorul de legaturi: %lu\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s\n\n",
                nume_fisier, file_info.st_size, file_info.st_uid, timestamp, file_info.st_nlink, user_permissions, group_permissions, others_permissions);

        if (write(stat_fd, statistica, strlen(statistica)) == -1) {
            perror("Eroare la scriere");
            close(fd);
            return -1;
        }
    }

    close(fd);
    return 0;  // Returnăm 0 pentru a indica succesul operației
}


void process_directory(const char *dir_path, int stat_fd) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror("Eroare la deschiderea directorului");
        exit(1);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char path[PATH_MAX_LEN];
            int path_len = snprintf(path, PATH_MAX_LEN, "%s/%s", dir_path, entry->d_name);
            if (path_len >= PATH_MAX_LEN) {
                fprintf(stderr, "Calea %s/%s este prea lunga.\n", dir_path, entry->d_name);
                continue;
            }

            struct stat entry_info;
            if (lstat(path, &entry_info) == -1) {
                perror("Eroare la obtinerea informatiilor despre intrare");
                closedir(dir);
                exit(1);
            }

            char statistica[512];
            if (S_ISDIR(entry_info.st_mode)) {
                // Procesare director
                char user_permissions[4], group_permissions[4], others_permissions[4];
                
                user_permissions[0] = (entry_info.st_mode & S_IRUSR) ? 'R' : '-';
                user_permissions[1] = (entry_info.st_mode & S_IWUSR) ? 'W' : '-';
                user_permissions[2] = (entry_info.st_mode & S_IXUSR) ? 'X' : '-';
                user_permissions[3] = '\0';

                group_permissions[0] = (entry_info.st_mode & S_IRGRP) ? 'R' : '-';
                group_permissions[1] = (entry_info.st_mode & S_IWGRP) ? 'W' : '-';
                group_permissions[2] = (entry_info.st_mode & S_IXGRP) ? 'X' : '-';
                group_permissions[3] = '\0';

                others_permissions[0] = (entry_info.st_mode & S_IROTH) ? 'R' : '-';
                others_permissions[1] = (entry_info.st_mode & S_IWOTH) ? 'W' : '-';
                others_permissions[2] = (entry_info.st_mode & S_IXOTH) ? 'X' : '-';
                others_permissions[3] = '\0';

                sprintf(statistica, "nume director: %s\nidentificatorul utilizatorului: %d\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s\n\n",
                        entry->d_name, entry_info.st_uid, user_permissions, group_permissions, others_permissions);

                if (write(stat_fd, statistica, strlen(statistica)) == -1) {
                    perror("Eroare la scriere");
                    closedir(dir);
                    exit(1);
                }

                process_directory(path, stat_fd);
            } else {
                // Procesare fisier sau legatura simbolica
                if (process_file(path, stat_fd) == -1) {
                    // Tratarea erorilor în funcția process_file
                    closedir(dir);
                    exit(1);
                }
            }
        }
    }

    closedir(dir);
}
