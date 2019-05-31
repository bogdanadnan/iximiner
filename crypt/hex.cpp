//
// Created by Haifa Bogdan Adnan on 30/05/2019.
//

#include "../common/dllexport.h"
#include "../common/common.h"
#include "hex.h"

void hex::encode(const unsigned char *input, int input_size, char *output) {
    for(int i=0; i<input_size; i++) {
        sprintf(output, "%02x", input[i]);
        output+=2;
    }
}

int hex::decode(const char *input, unsigned char *output, int output_size) {
    char buff[3];
    size_t input_size = strlen(input);
    if(output_size < input_size / 2)
        return -1;

    for(int i=0; i<input_size; i+=2) {
        strncpy(buff, &input[i], 2);
        output[i/2] = strtoul(buff, NULL, 16);
    }

    return input_size / 2;
}
