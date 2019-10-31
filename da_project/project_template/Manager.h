//
// Created by stefano on 31/10/19.
//

#ifndef PROJECT_TEMPLATE_MANAGER_H
#define PROJECT_TEMPLATE_MANAGER_H

#include <vector>

//#include "Receiver.h"
#include "Sender.h"

using namespace std;

class Manager {
    int test;
    vector<Sender*> senders;
    //Received receiver;

public:
    Manager(vector<Sender*> senders);        // Constructor
};


#endif //PROJECT_TEMPLATE_MANAGER_H
