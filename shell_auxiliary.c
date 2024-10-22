#include "shell_auxiliary.h"

void getsUart0(USER_DATA *dataStruct)
{
    uint8_t count = 0;
    char c;
    do
    {
        c = getcUart0();

        // Blocking function
        /*
            ASCII values:
            127: Delete
            8: Backspace
            10: Line Feed
            13: Carriage Return
        */
        while (c == ASCII_DELETE && (count == 0 | count == 1))
        {
            if (count > 0)
                count--;
            c = getcUart0();
        }

        // Delete or Backspace
        count = ((c == ASCII_BACKSPACE) | (c == ASCII_DELETE)) ? (count > 0 ? --count : count) : ++count;

        // LF or CR i.e (Enter or Max char reached) add null terminator
        if ((c == 10 | c == 13) | count == MAX_CHARS)
        {
            dataStruct->buffer[count - 1] = c;
            dataStruct->buffer[count++] = 0;
            // Only need when reached max chars
            c = 0;
        }
        // Printable character
        if (c >= 32 && c < 127)
            dataStruct->buffer[count - 1] = c;
    } while (dataStruct->buffer[count - 1] != 0);
}

/**
 * @brief Detects wether it is alphabetic and returns a boolean
 * @param c Input character
 * @return bool
 */
bool isAlpha(char c)
{
    return (c >= ASCII_A && c <= ASCII_Z) | (c >= ASCII_a && c <= ASCII_z) ? true : false;
}

/**
 * @brief Detects wether char is numeric
 * @param c Input character
 * @return bool
 */
bool isNumeric(char c)
{
    return (c >= ASCII_0 && c <= ASCII_9) | (c >= ASCII_COMMA && c <= ASCII_PERIOD) ? true : false;
}

void parseFields(USER_DATA *dataStruct)
{
    /**
     * @brief
     * Alphabetic upper case: 65-90
     * Alphabetic lower case: 97-122
     * Numeric: 48-57
     * Comma, Hyphen, Period : 44, 45, 46
     */
    uint8_t count = 0;

    // Can only parse 5 fields
    while ((dataStruct->buffer[count] != 0) && (dataStruct->fieldCount < MAX_FIELDS))
    {
        // 97 = 'a'
        if (isAlpha(dataStruct->buffer[count]))
        {
            dataStruct->fieldType[dataStruct->fieldCount] = 97;
            dataStruct->fieldPosition[dataStruct->fieldCount++] = count;
        }
        // 110 = 'n'
        else if (isNumeric(dataStruct->buffer[count]))
        {
            dataStruct->fieldType[dataStruct->fieldCount] = 110;
            dataStruct->fieldPosition[dataStruct->fieldCount++] = count;
        }
        else
            dataStruct->buffer[count] = 0;

        // count will be at the delimeter position
        while (isAlpha(dataStruct->buffer[count]) | isNumeric(dataStruct->buffer[count++]))
            ;
        dataStruct->buffer[count - 1] = 0;
    }
}
/**
 * @brief Clear the struct members
 *
 * @return no return
 */
void clearStruct(USER_DATA *dataStruct)
{
    uint8_t i = 0;
    for (i = 0; i < MAX_CHARS; i++)
        dataStruct->buffer[i] = 0;
    for (i = 0; i < MAX_FIELDS; i++)
    {
        dataStruct->fieldPosition[i] = 0;
        dataStruct->fieldType[i] = 0;
    }
    dataStruct->fieldCount = 0;
}
/**
 * @brief Get the field value
 * @param dataStruct
 * @param fieldNumber
 *
 * @return The value of the field requested if it exists or NULL otherwise
 */
char *getFieldString(USER_DATA *dataStruct, uint8_t fieldNumber)
{
    return fieldNumber < dataStruct->fieldCount ? &dataStruct->buffer[dataStruct->fieldPosition[fieldNumber]] : NULL;
}

/**
 * @brief ASCII to integer conversion
 * @param str
 * @return int32_t
 */
int32_t atoi(char *str)
{
    /*
    ASCII values:
    45: '-'
    48: '0'
    57: '9'
    */

    // If it is a negative number
    // Start from the next character
    uint8_t i = str[0] == ASCII_MINUS ? 1 : 0;
    int32_t value = 0;

    while (str[i] != 0)
    {
        value = value * 10 + (str[i++] - 48);
    }
    return value;
}

/**
 * @brief Function to return the integer value of the field if it exists and is numeric
 * otherwise it returns 0
 * @param dataStruct
 * @param fieldNumber
 * @return int
 */
int32_t getFieldInteger(USER_DATA *dataStruct, uint8_t fieldNumber)
{
    /*
        ASCII values:
        110: 'n'
    */
    return (fieldNumber < dataStruct->fieldCount) && (dataStruct->fieldType[fieldNumber] == 110) ? atoi(getFieldString(dataStruct, fieldNumber)) : 0;
}
/**
 * @brief Determine if provide command with minimum number
 * of arguments satisfies contents in buffer
 *
 * @param dataStruct
 * @param command
 * @param minArgs
 * @return true If valid input from user
 * @return false
 */
bool isCommand(USER_DATA *dataStruct, const char command[], uint8_t minArgs)
{
    uint8_t i = 0;
    for (i = 0; dataStruct->buffer[i] != 0; i++)
    {
        // If command field does not match command parameter return false
        if (dataStruct->buffer[i] != command[i])
            return false;
    }
    return ((dataStruct->fieldCount - 1) >= minArgs) ? true : false;
}

bool strCmp(const char str1[], const char str2[])
{
    uint8_t i = 0;
    while (str1[i] != 0 || str2[i] != 0)
    {
        if (str1[i] != str2[i])
            return false;
        i++;
    }
    return true;
}

/**
 * @brief
 * Convert uin32_t to string i.e ASCII
 * Either base 10 or base 16
 */
void itoa(uint32_t value, char str[], uint8_t base)
{
    uint8_t i = 0;
    uint8_t j = 0;
    uint8_t remainder;
    char buffer[10];
    while (value != 0)
    {
        if (base == 16)
        {
            remainder = (value - (base * (value >> 4)));
            remainder = remainder < 10 ? remainder + ASCII_0 : remainder + 55;
            buffer[i++] = remainder;
            value >>= 4;
        }
        else
        {
            buffer[i++] = value % base + ASCII_0;
            value /= base;
        }
    }
    // Reverse the buffer
    while (i > 0)
    {
        str[j++] = buffer[--i];
    }
    str[j] = 0;
}