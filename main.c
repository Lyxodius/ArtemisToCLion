#include <windows.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>

#define INPUT_BUFFER_SIZE 512

int file_exists(const char *s) {
    return access(s, F_OK) == 0;
}

char *concatenate_strings(const char *s1, const char *s2) {
    char *result = (char *) malloc(strlen(s1) + strlen(s2) + 1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

void remove_trailing_newline(char *s) {
    if ((strlen(s) > 0) && (s[strlen(s) - 1] == '\n')) {
        s[strlen(s) - 1] = '\0';
    }
}

void remove_enclosing_quotes(char *s) {
    if (s[0] == '"') {
        char *substr = s + 1;
        strcpy(s, substr);
    }
    if (s[strlen(s) - 1] == '"') {
        s[strlen(s) - 1] = '\0';
    }
}

void remove_recursively(char *path) {
    if (file_exists(path)) {
        WIN32_FIND_DATA ffd;
        HANDLE hFind;
        hFind = FindFirstFile(concatenate_strings(path, "*"), &ffd);

        do {
            char *name = ffd.cFileName;
            if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;
            char *filePath = concatenate_strings(path, ffd.cFileName);
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                remove_recursively(concatenate_strings(filePath, "\\"));
                RemoveDirectory(filePath);
            } else {
                DeleteFile(filePath);
            }
        } while (FindNextFile(hFind, &ffd));

        RemoveDirectory(path);
    }
}

char *build_git_command(char *repository_path, char *path_to_assume_unchanged) {
    return concatenate_strings(concatenate_strings(concatenate_strings("git --git-dir \"", repository_path),
                                                   ".git\" update-index --assume-unchanged "),
                               path_to_assume_unchanged);
}

int main() {
    setbuf(stdout, 0);
    printf("**************************************************\n");
    printf("*                                                *\n");
    printf("* Willkommen beim Artemis zu CLion Konvertierer! *\n");
    printf("*               (c) 2021 Lyxodius                *\n");
    printf("*                                                *\n");
    printf("**************************************************\n\n");
    printf("Zum Schliessen des Programms 'q' eingeben.\n\n");

    int success = 0;
    while (success == 0) {
        char *path = malloc(INPUT_BUFFER_SIZE);
        int valid_path = 0;
        while (valid_path == 0) {
            printf("Bitte geben Sie den Pfad eines Artemis-Repository ein:\n");
            fgets(path, INPUT_BUFFER_SIZE, stdin);
            printf("\n");
            remove_trailing_newline(path);
            remove_enclosing_quotes(path);

            if (strcmp(path, "q") == 0) {
                return 0;
            } else if (!file_exists(path)) {
                printf("An dem gegebenen Pfad existiert kein Ordner. Bitte probieren Sie es erneut.\n");
            } else if (strlen(path) > 512) {
                printf("Pfad ist zu lang. Es sind maximal 512 Zeichen erlaubt.");
            } else {
                valid_path = 1;
            }
        }

        if (path[strlen(path) - 1] != '\\') {
            strcat(path, "\\");
        }

        printf("------------------------------------------------------------\n");
        int actions = 0;

        char *run_dir_path = concatenate_strings(path, ".run\\");
        if (file_exists(run_dir_path)) {
            remove_recursively(run_dir_path);
            printf(".run-Verzeichnis geloescht.\n");
            actions++;
        } else {
            printf(".run-Verzeichnis nicht gefunden. Wurde es bereits geloescht?\n");
        }

        char *makefile_path = concatenate_strings(path, "Makefile");
        if (file_exists(makefile_path)) {
            DeleteFile(makefile_path);
            printf("Makefile wurde geloescht.\n");
            actions++;
        } else {
            printf("Makefile nicht gefunden. Wurde sie bereits geloescht?\n");
        }

        WIN32_FIND_DATA ffd;
        HANDLE hFind;
        hFind = FindFirstFile(concatenate_strings(path, "*.c"), &ffd);
        char projectName[64];
        if (hFind != INVALID_HANDLE_VALUE) {
            FILE *f = fopen(concatenate_strings(path, "CMakeLists.txt"), "w");
            int firstProject = 1;
            do {
                strcpy(projectName, ffd.cFileName);
                projectName[strlen(projectName) - 2] = '\0';
                if (!firstProject) {
                    fprintf(f, "\n\n");
                } else {
                    firstProject = 0;
                }
                fprintf(f, "project(%s)\n", projectName);
                fprintf(f, "add_executable(%s %s)", projectName, ffd.cFileName);
                printf("Projekt %s wurde erstellt.\n", projectName);
                actions++;
            } while (FindNextFile(hFind, &ffd));
            fclose(f);
        } else {
            printf("Keine .c-Dateien gefunden.\n");
        }

        char *gitignore_path = concatenate_strings(path, ".gitignore");
        FILE *gitignore = fopen(gitignore_path, "w");
        fprintf(gitignore, "/.idea/\ncmake*");
        fclose(gitignore);
        if (file_exists(gitignore_path)) {
            printf(".gitignore aktualisiert.\n");
            actions++;
        } else {
            printf("Konnte .gitignore nicht aktualisieren.\n");
        }

        char *gitignore_command = build_git_command(path, ".gitignore");
        system(gitignore_command);
        char *all_run_command = build_git_command(path, ".run/all.run.xml");
        system(all_run_command);
        char *makefile_command = build_git_command(path, "Makefile");
        system(makefile_command);
        printf("Fuer Aufgaben irrelevante Dateien werden von git ignoriert.\n");

        printf("------------------------------------------------------------\n\n");

        if (actions > 0) {
            printf("Erfolgreich konvertiert!\nDas Projekt kann jetzt in CLion geoeffnet werden.\n\n");
            success = 1;
        } else {
            printf("Keine Aktionen durchgefuehrt.\n");
            printf("Wurde ein falscher Pfad eingegeben oder das Repository bereits konvertiert?\n\n");
        }
    }
    system("pause");

    return 0;
}
