//
// Created by Haifa Bogdan Adnan on 17/08/2018.
//

#ifndef IXIMINER_BASE64_H
#define IXIMINER_BASE64_H

class DLLEXPORT base64 {
public:
    static void encode(const char *input, int input_size, char *output);
    static int decode(const char *input, char *output, int output_size);
};

#endif //IXIMINER_BASE64_H
