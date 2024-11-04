#ifndef SHELL_AUXILIARY
#define SHELL_AUXILIARY

#include <stdint.h>
#include <stdbool.h>
#include "uart0.h"
#include "tm4c123gh6pm.h"
#include "clock.h"

// Maximum number of chars that can be accepted from the user
// and the structure for holding UI info
#define MAX_CHARS 80
#define MAX_FIELDS 5

#define ASCII_BACKSPACE 8
#define ASCII_COMMA 44
#define ASCII_MINUS 45
#define ASCII_PERIOD 46
#define ASCII_0 48
#define ASCII_9 57
#define ASCII_A 65
#define ASCII_Z 90
#define ASCII_a 97
#define ASCII_z 122
#define ASCII_DELETE 127

#define NULL 0

typedef struct _USER_DATA
{
    char buffer[MAX_CHARS + 1];
    uint8_t fieldCount;
    uint8_t fieldPosition[MAX_FIELDS];
    uint8_t fieldType[MAX_FIELDS];
} USER_DATA;

bool isAlpha(char c);
bool isNumeric(char c);
bool isCommand(USER_DATA *dataStruct, const char command[], uint8_t minArgs);
bool strCmp(const char str1[], const char str2[]);

void itoa(uint32_t value, char str[], uint8_t base);
void parseFields(USER_DATA *dataStruct);
void clearStruct(USER_DATA *dataStruct);
void getsUart0(USER_DATA *dataStruct);

char *getFieldString(USER_DATA *dataStruct, uint8_t fieldNumber);
// int32_t atoi(char *str);
int32_t getFieldInteger(USER_DATA *dataStruct, uint8_t fieldNumber);

#endif
