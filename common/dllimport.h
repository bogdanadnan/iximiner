//
// Created by Haifa Bogdan Adnan on 04.11.2018.
//

#ifndef IXIMINER_DLLIMPORT_H
#define IXIMINER_DLLIMPORT_H

#ifndef DLLEXPORT
    #ifndef _WIN64
        #define DLLEXPORT
    #else
        #define DLLEXPORT __declspec(dllimport)
    #endif
#endif

#endif //IXIMINER_DLLIMPORT_H
