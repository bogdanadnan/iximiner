//
// Created by Haifa Bogdan Adnan on 29/08/2018.
//

#ifndef IXIMINER_AUTOTUNE_H
#define IXIMINER_AUTOTUNE_H

#include "../app/runner.h"

class autotune : public runner {
public:
    autotune(arguments &args);
    ~autotune();

    virtual void run();
    virtual void stop();
private:
    arguments &__args;
    bool __running;
};


#endif //IXIMINER_AUTOTUNE_H
