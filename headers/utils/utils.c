#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils/utils.h"

struct kv {
    char process[30];
    char keys[30][50];
    float values[30];
};

int read_parameter_file(struct kv *params) {
    struct kv aux;
    FILE *f;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    f = fopen("conf/drone_parameters.conf", "r");
    if (f == NULL) {
        printf("Erorr during fopen inside utils read_parameter_file");
        getchar();
        exit(EXIT_FAILURE);
    }

    int values_index = 0;
    char *token;
    char *saveptr;
    int process_index = 0;

    char to_parse[100];
    while (1) {
        read = getline(&line, &len, f);
        // remove the newline
        if (read != -1)
            line[strlen(line) - 1] = 0;
        if (read == -1 || strlen(line) == 0) {
            params[process_index++] = aux;
            values_index = 0;
            memset(aux.process, 0, sizeof(aux.process));
            memset(aux.values, 0, sizeof(aux.values));
            memset(aux.keys, 0, sizeof(aux.keys));
            if(read == -1)
                break;
        } else if (line[0] == '#') {
            continue;
        } else if (line[0] == '[') {
            strncpy(aux.process, &line[1], strlen(line) - 2);
        } else {
            token = strtok_r(line, "=", &saveptr);
            strcpy(aux.keys[values_index], token);
            token = strtok_r(NULL, "=", &saveptr);
            float aux_value;
            sscanf(token, "%f", &aux_value);
            aux.values[values_index++] = aux_value;
        }
    }

    fclose(f);
    if (line)
        free(line);

    return process_index;
}

float get_param(char *process, char *param) {
    struct kv read_values[5];
    int processes = read_parameter_file(read_values);
    for (int i = 0; i < processes; i++) {
        if (!strcmp(read_values[i].process, process)) {
            for (int j = 0; strlen(read_values[i].keys[j]) != 0; j++) {
                if (!strcmp(read_values[i].keys[j], param)) {
                    return read_values[i].values[j];
                }
            }
        }
    }
    return -1;
}
