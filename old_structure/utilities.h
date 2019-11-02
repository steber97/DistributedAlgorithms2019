
#ifndef PROJECT_TEMPLATE_UTILITIES_H
#define PROJECT_TEMPLATE_UTILITIES_H

#include "Manager.h"

/**
 * this function returns a string (const char*) out of an int
 * @param n: any integer value (I guess only positive ones).
 * @return
 */
const char* int_to_char_pointer(int n);

Link* parse_input_data(int argc, char** argv);

#endif //PROJECT_TEMPLATE_UTILITIES_H

