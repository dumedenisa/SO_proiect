#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>
#include <stdint.h>

#define PATH_MAX_LEN 256
char file[256];

// functie pentru numararea liniilor scrise intr un fisier
int countLines(const char *file)
{ 

    int fIn = open(file, O_RDONLY);
    if (fIn == -1)
    {
        perror("Eroare la deschiderea fisierului!");
        exit(-1);
    }

    int count = 0;
    char ch;
    int bytesRead;

    while ((bytesRead = read(fIn, &ch, 1)) > 0)
    { 
        if (ch == '\n')
        {
            count++;
        }
    }

    if (bytesRead == -1)
    {
        perror("Eroare la citirea fisierului!");
        exit(-2);
    }

    close(fIn);
    return count;
}

void convertToGray(const char *inputPath)
{
    int inputFile = open(inputPath, O_RDWR);
    if (inputFile == -1)
    {
        perror("Eroare la deschiderea fisierului BMP pentru citire si scriere!");
        exit(EXIT_FAILURE);
    }

    unsigned char header[54];
    if (read(inputFile, header, sizeof(header)) == -1)
    {
        perror("Eroare la citirea header-ului!");
        exit(EXIT_FAILURE);
    }

    int width = *(int *)&header[18];
    int height = *(int *)&header[22];
    int pixels = width * height;

    unsigned char *image = malloc(sizeof(unsigned char) * pixels * 3);
    if (image == NULL)
    {
        perror("Eroare la alocarea memoriei pentru imagine!");
        exit(EXIT_FAILURE);
    }

    if (read(inputFile, image, sizeof(unsigned char) * pixels * 3) == -1)
    {
        perror("Eroare la citirea imaginii!");
        free(image);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < pixels * 3; i += 3)
    {
        uint8_t red = image[i];
        uint8_t green = image[i + 1];
        uint8_t blue = image[i + 2];

        uint8_t gray = (uint8_t)(0.299 * red + 0.587 * green + 0.114 * blue);

        image[i] = gray;
        image[i + 1] = gray;
        image[i + 2] = gray;
    }

    if (lseek(inputFile, 54, SEEK_SET) == -1)
    {
        perror("Eroare la mutarea cursorului la începutul imaginii!");
        free(image);
        exit(EXIT_FAILURE);
    }

    if (write(inputFile, image, sizeof(unsigned char) * pixels * 3) == -1)
    {
        perror("Eroare la scrierea imaginii!");
        free(image);
        exit(EXIT_FAILURE);
    }

    close(inputFile);
    free(image);
}

int process_file(const char *intFile, const char *outFile)
{
    int fd = open(intFile, O_RDONLY);
    if (fd == -1)
    {
        perror("Eroare la deschiderea fisierului");
        return -1;
    }

    struct stat file_info;
    if (fstat(fd, &file_info) == -1)
    {
        perror("Eroare la obtinerea informatiilor despre fisier");
        close(fd);
        return -1;
    }

    char nume_fisier[256];
    strcpy(nume_fisier, intFile);

    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%d.%m.%Y", localtime(&file_info.st_mtime));

    int statFile = open(outFile, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR); 
    if (statFile == -1)
    {
        perror("Eroare la deschiderea fisierului output!");
        exit(-4);
    }

    mode_t file_mode = file_info.st_mode;
    char stringUser[4], stringGroup[4], stringOther[4];

    stringUser[0] = (file_mode & S_IRUSR) ? 'R' : '-';
    stringUser[1] = (file_mode & S_IWUSR) ? 'W' : '-';
    stringUser[2] = (file_mode & S_IXUSR) ? 'X' : '-';
    stringUser[3] = '\0';

    stringGroup[0] = (file_mode & S_IRGRP) ? 'R' : '-';
    stringGroup[1] = (file_mode & S_IWGRP) ? 'W' : '-';
    stringGroup[2] = (file_mode & S_IXGRP) ? 'X' : '-';
    stringGroup[3] = '\0';

    stringOther[0] = (file_mode & S_IROTH) ? 'R' : '-';
    stringOther[1] = (file_mode & S_IWOTH) ? 'W' : '-';
    stringOther[2] = (file_mode & S_IXOTH) ? 'X' : '-';
    stringOther[3] = '\0';

    int fisier;

    if (S_ISREG(file_info.st_mode))
    { // verific daca e fisier obisnuit cu extensia .bmp
        if (strstr(intFile, ".bmp") != NULL)
        {
            if ((fisier = open(intFile, O_RDONLY)) < 0)
            { // deschid fisierul bmp
                perror("Eroare la deschiderea fisierului !");
                close(statFile);
                exit(-5);
            }

            int offset = 0;

            if ((offset = lseek(fisier, 18, SEEK_SET)) == -1)
            { // mut cu functia lseek cursorul la byte-ul 18
                perror("Eroare la mutarea cursorului la width!");
                close(fisier);
                exit(-6);
            }
            int width;
            if (read(fisier, &width, sizeof(int)) == -1)
            { // citesc de la byte-ul 18 width-ul pozei
                perror("Eroare la citirea latimii pozei!");
                close(fisier);
                exit(-7);
            }

            if ((offset = lseek(fisier, 22, SEEK_SET)) == -1)
            { // mut cu functia lseek cursorul la byte-ul 22
                perror("Eroare la mutarea cursorului la height!");
                close(fisier);
                exit(-8);
            }
            int height;
            if (read(fisier, &height, sizeof(int)) == -1)
            { // citesc de la byte-ul 22 height-ul pozei
                perror("Eroare la citirea inaltimii pozei!");
                close(fisier);
                exit(-9);
            }
            close(fisier);

            int pidConvert = fork(); // creez pocesul pentru converie in gri

            if (pidConvert < 0)
            {
                perror("Eroare la creearea procesului!");
                exit(-11);
            }

            if (pidConvert == 0)
            { // Proces fiu
                convertToGray(intFile);
                exit(-10);
            }
            else
            { // Proces parinte
                int status;
                wait(&status);
            }

            // Statisticile se scriu doar pentru fișierele care nu sunt .bmp
            dprintf(statFile, "Nume fisier: %s\nLatime:%d \nInaltime: %d\nDimensiune: %ld bytes\nIdentificatorul user-ului: %d\nTimpul ultimei modificari: %s\nContorul de legturi: %ld\nDrepturi de acces user: %s\nDrepturi de acces grup: %s\nDrepturi de acces alții: %s\n \n",
                    intFile, width, height, (long)file_info.st_size, file_info.st_uid, timestamp, file_info.st_nlink, stringUser, stringGroup, stringOther); // afisez in fisierul statistica datele despre fisierul .bmp
        }
        else
        { // este un fisier obisnuit fara .bmp
            dprintf(statFile, "Nume fisier: %s\nDimensiune: %ld bytes\nIdentificatorul user-ului: %d\nTimpul ultimei modificari: %s\nContorul de legturi: %ld\nDrepturi de acces user: %s\nDrepturi de acces grup: %s\nDrepturi de acces alții: %s\n \n",
                    intFile, (long)file_info.st_size, file_info.st_uid, timestamp, (long)file_info.st_nlink, stringUser, stringGroup, stringOther); // afisez in fisierul statistica datele despre fisier
        }
    }
    else if (S_ISDIR(file_info.st_mode))
    { // verific daca este un alt director
        dprintf(statFile, "Nume director: %s\nIdentificatorul utilizatorului: %d\nDrepturi de acces user: %s\nDrepturi de acces grup: %s\nDrepturi de acces alții: %s\n \n",
                intFile, file_info.st_uid, stringUser, stringGroup, stringOther); // afisez in fisierul statistica datele despre director
    }
    else if (S_ISLNK(file_info.st_mode))
    { // verific daca este legatura simbolica
        dprintf(statFile, "Nume legatura: %s\nDimensiune fisier dimensiunea fisierului target: %ld \nDrepturi de acces user legatura: %s\nDrepturi de acces grup legatura: %s\nDrepturi de acces alții legatura: %s\n \n",
                intFile, (long)file_info.st_size, stringUser, stringGroup, stringOther); // afisez in fisierul statistica datele despre legatura simbolica
    }

    close(statFile);

    int numLines;
    numLines = countLines(outFile);
    return numLines; // functia returneaza un intreg ce reprezinta numarul de linii scrise in fisierul statisitica a fisierului in care ma aflu
}

void process_directory(const char *intDir, const char *outDir)
{
    DIR *dir = opendir(intDir);
    if (!dir)
    {
        perror("Eroare la deschiderea directorului");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            char path[PATH_MAX_LEN];
            int path_len = snprintf(path, PATH_MAX_LEN, "%s/%s", intDir, entry->d_name);
            if (path_len >= PATH_MAX_LEN)
            {
                fprintf(stderr, "Calea %s/%s este prea lunga.\n", intDir, entry->d_name);
                continue;
            }

            struct stat entry_info;
            if (lstat(path, &entry_info) == -1)
            {
                perror("Eroare la obtinerea informatiilor despre intrare");
                closedir(dir);
                exit(EXIT_FAILURE);
            }

            char statistica[512];
            //char outputFilePath[PATH_MAX_LEN];
            snprintf(statistica, 256, "%s/%s_statistica.txt", outDir, entry->d_name);

            int pid = fork(); // creez procesul pentru scrierea de informații în fișierul de statistică

            if (pid < 0)
            {
                perror("Eroare la creearea procesului de scriere!");
                exit(EXIT_FAILURE);
            }

            if (pid == 0)
            {                                                 // cod fiu
                int numLines = process_file(file, statistica); // apelam functia care se ocupa de procesarea unui fisier
                exit(numLines);                               // returnam numarul de linii
            }
            else
            { // cod parinte
                int status;
                wait(&status);
                printf("S-a încheiat procesul cu PID-ul %d și statusul %d. Numărul de linii scrise: %d.\n", pid, WEXITSTATUS(status), countLines(statistica));
            }
        }

        closedir(dir);
    }
}

    int main(int argc, char *argv[])
    {
        if (argc != 3)
        {
            fprintf(stderr, "Usage: %s <director_intrare> <director_iesire>\n", argv[0]);
            exit(1);
        }

        const char *director_intrare = argv[1];
        const char *director_iesire = argv[2];

        int stat_fd = open("statistica.txt", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
        if (stat_fd == -1)
        {
            perror("Eroare la crearea fisierului de statistica");
            exit(1);
        }

        process_directory(director_intrare, director_iesire);

        close(stat_fd);

        return 0;
    }
