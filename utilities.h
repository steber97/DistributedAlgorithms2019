
#ifndef PROJECT_TEMPLATE_UTILITIES_H
#define PROJECT_TEMPLATE_UTILITIES_H

#include <iostream>
#include <fstream>
#include <vector>
#include "Link.h"

pair<int, unordered_map<int, pair<string, int>>*> parse_input_data(string &membership_file);
bool is_ack(string msg);
void ack_received(string msg);

#endif //PROJECT_TEMPLATE_UTILITIES_H

