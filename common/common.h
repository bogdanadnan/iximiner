//
// Created by Haifa Bogdan Adnan on 04/08/2018.
//

#ifndef ARIOMINER_COMMON_H
#define ARIOMINER_COMMON_H

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <regex>

#include <thread>
#include <mutex>
#include <chrono>

#include <cmath>

#ifndef _MSC_VER
#include <unistd.h>
#include <sys/time.h>
#else
#include <win32_compatibility_layer.h>
#endif

#include <config.h>

using namespace std;

#define LOG(msg) cout<<msg<<endl

uint64_t microseconds();



#endif //ARIOMINER_COMMON_H