/** Some utils function */

#include <stdlib.h>
#include <string.h>
#include "include/utils.h"

/** Clone of the standard itoa function with the exception of that this realizaion create string buffer
 *  by himself */
char* itoa2(int num) {
    char *buf = malloc(50);
    sprintf(buf, "%d", num);
    uint8_t len = (uint8_t) (strlen(buf) + 1);
    char *split = malloc(len);
    memcpy(split, buf, len);
    free(buf);

    return split;
}

/** Clone of the standard strcpy function with the exception of that this realizaion create string buffer
 *  by himself */
char* strcpy2(char* str) {
    size_t strsize = strlen(str) + 1;
    char* dup_str = malloc(strsize);
    memcpy(dup_str, str, strsize);

    return dup_str;
}

/** Create hex string from the binary blob */
char* sprintfhex(uint8_t *array, uint32_t size) {
	int sbf = size * 3 + 1;
	char *bf = (char*) malloc(sbf);
	char *s = (char*) malloc(4);
    for (int i = 0; i < size; i++) {
    	sprintf(s, "%02X ", array[i]);
    	memcpy(bf + i * 3, s, 3);

    }
    free(s);
    bf[sbf - 1] = 0;

    return bf;
}
