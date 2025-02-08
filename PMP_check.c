#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

/************************ Functions ***********************************/
void PMP_Check(char *filepath, char *address, char mode, char oper);
void cnfgn_bin_convert(char pmp_cnfgn[64][9]);
void mode_store(char pmp_cnfgn[64][9], int addr_mode[64]);
int MODE_check(char pmp_cnfgn[64][9], int pointer);
int addr_search(char pmp_addr[64][13], int addr_mode[64], char *addr);
int TOR_addr_check(char pmp_addr[64][13], char *addr, int pointer);
int NA4_addr_check(char pmp_addr_reg[13], char *addr);
int NAPOT_addr_check(char *pmp_addr_reg, char *addr);
void addr_bin_conv(char *pmp_addr_reg, char *bin_addr);
uint64_t NAPOT_offset(char bin_addr[35]);
uint64_t NAPOT_base(char bin_addr[35]);
int checkLock(char pmp_cnfgn[64][9]);
void Perm_check(char pmp_cnfgn[][9], char oper, int pointer);
/***********************************************************************/

char *bin[] = {"0000", "0001", "0010", "0011", "0100", "0101", "0110", "0111", 
               "1000", "1001", "1010", "1011", "1100", "1101", "1110", "1111"};

int main(int argc, char *argv[]){
    // Checking any errors in number of arguments entered
    if (argc != 5){
        printf("Error: Expected 4 arguments, but %d were entered\n", &argc - 1);
        return 1;
    }

    char *filepath = argv[1];
    char *addr = argv[2];
    char mode = argv[3][0];
    char oper = argv[4][0];

    // Checking validity of input arguments
    if (fopen(filepath, "r") == NULL){
        printf("Error: No such file exists.\n");
        return 1;
    }
    else if ((addr[0] != '0') || (addr[1] != 'x')){
        printf("Error: Leading \'0x\' missing in hexadecimal address\n");
        return 1;
    }
    else if ((mode != 'M') && (mode != 'S') && (mode != 'U')){
        printf("Error: Invalid mode \'%c\'.\n", mode);
        return 1;
    }
    else if ((oper != 'R') && (oper != 'W') && (oper != 'X')){
        printf("Error: Invalid operation \'%c\'.\n", oper);
        return 1;
    }


    PMP_Check(filepath, addr, mode, oper);

    return 0;
}

void PMP_Check(char *filepath, char *address, char mode, char oper){
    /* Function implementing a Physical Memory Protection
    Takes 4 arguments:
    1. filepath: A string containing the path to a PMP Configuration file
    2. address: Address of the memory location for which the permission is to be checked
    3. mode: Privilege mode, which maybe M, S or U 
    4. oper: The operation that is to be performed, which includes R, W and X
    The functions return type is void
    The following assumptions are made regarding the configuration file:
    - Config file contains 128 lines
    - First 64 lines are the data of the corresponding 64 pmp configuration registers 
    - Next 64 lines are the data of the corresponding  64 pmp address registers
    - There are no semi-colons at the end of each line and each line starts with 0x
    - Data is in hexadecimal format*/

    FILE *file = fopen(filepath, "r");
    
    if (file == NULL){
        printf("Failed to open file.\n");
    }

    char pmpcnfgn[64][9];
    char pmpaddr[64][13];
    int i = 0;
    
    // Loads cofiguration register data in the 2D array pmpcnfgn
    while ((i < 64) && fgets(pmpcnfgn[i], sizeof(pmpcnfgn[i]), file)){
        printf("pmpcnfgn_%d = %s", i, pmpcnfgn[i]);
        i++;
    }
    
    i = 0;
    // Loads address register data in the 2D array pmpaddr
    while ((i < 64) && fgets(pmpaddr[i], sizeof(pmpaddr[i]), file)){
        printf("pmpaddr_%d = %s", i, pmpaddr[i]);
        i++;
    }
    printf("\n");

    fclose(file);

    // Convert string of hexadecimal digits to binary string in pmpcnfgn registers
    cnfgn_bin_convert(pmpcnfgn);
    // Storing address modes of PMP entries in a separate array
    int addr_mode[64];
    mode_store(pmpcnfgn, addr_mode);
    // Searches for the relevant pmp address and config register number
    int x = addr_search(pmpaddr, addr_mode, address); 
    // Prints data in binary format for the selected config register
    printf("pmpcnfgn_%d = %s", x, pmpcnfgn[x]);
    for (int j = 0; j < 8; j++){
        printf("%c", pmpcnfgn[x][j]);
    }
    printf("\n"); 
    // Checking permissions for the memory block
    if ((x == 100) && ((mode == 'U') || (mode == 'S'))){
        printf("Access fault.\n");
    }
    else if ((x == 100) && (mode = 'M')){
        if (checkLock(pmpcnfgn)){
            printf("Access fault.\n");
        }
        else{
            printf("No access fault.\n");
        }
    }
    else if ((pmpcnfgn[x][0] == '0') && mode == 'M'){
        printf("No access fault.\n");
    }
    else{
        Perm_check(pmpcnfgn, oper, x);
    }
    
} 

void cnfgn_bin_convert(char pmp_cnfgn[64][9]){
    /* Function defined specifically for PMP configuration registers
    Converts the hexadecimal string in the set of pmp configuration registers and converts
    them into binary strings
    Takes a single argument:
    pmp_cnfgn[64][9]: A 2D array containing the data of all the pmp configuration registers
    The function's return type is void
    Changes are made on default pmp configuration registers, not on a separate instance */
    int j, len, pos, i, val;
    char *end_ptr;
    char temp[9]; // temporary variable to store binary string
    for (j = 0; j < 64; j++){
        // Inserting missing leading 0's
        end_ptr = strchr(pmp_cnfgn[j], '\n'); // Points to the '\n' character
        len = end_ptr - pmp_cnfgn[j] - 2; // Length of hex string, excluding "0x" and '\n'
        pos = 0;
        // Insering missing leading 0's in binary string
        if (len < 2){
            strncpy(temp + pos, bin[0], 4);
            pos += 4;
        }

        i = 2;
        while (pmp_cnfgn[j][i] != '\n'){
            // Conversion of hex character to a binary string of length 4 
            if ((pmp_cnfgn[j][i] >= '0' ) && (pmp_cnfgn[j][i] <= '9')){
                val = pmp_cnfgn[j][i] - '0';
                strncpy(temp + pos, bin[val], 4);
            }
            else if ((pmp_cnfgn[j][i] >= 'A') && (pmp_cnfgn[j][i] <= 'F')){
                val = pmp_cnfgn[j][i] - 'A' + 10;
                strncpy(temp + pos, bin[val], 4);
            }
            else{
                val = pmp_cnfgn[j][i] - 'a' + 10;
                strncpy(temp + pos, bin[val], 4);
            }
            pos += 4; 
            i++;
        }
        temp[8] = '\0';
        // Binary string replaces the hex string in default pmp_cnfgn array
        strcpy(pmp_cnfgn[j], temp);
    }
}

void mode_store(char pmp_cnfgn[64][9], int addr_mode[64]){
    /* Function stores address mode of each pmp entry in a separate 1D array for easy access
    Takes two arguments:
    1. pmp_cnfgn[64][9]: A 2D array containg the data of pmp configuration registers in binary format
    2. addr_mode[64]: A 1D array to store the address mode of each pmp entry at the corresponding index
    Return type is void*/
    int i;
    for (i = 0; i < 64; i++){
        addr_mode[i] = MODE_check(pmp_cnfgn, i);
    }
}

int MODE_check(char pmp_cnfgn[64][9], int pointer){
    /* Function to check address mode of a given configuration register
    Takes 2 arguments:
    1. pmp_cnfgn[64][9]: A 2D array containing the data of all the pmp configuration registers in 
    binary format
    2. pointer: register number for which the address mode is to be checked 
    Returns an int corresponding to address modes of the pmp configuration register
    */
    if ((pmp_cnfgn[pointer][3] == '0') && (pmp_cnfgn[pointer][4] == '0')){ // OFF mode
        return 0;
    }
    else if ((pmp_cnfgn[pointer][3] == '0') && (pmp_cnfgn[pointer][4] == '1')){ // TOR mode
        return 1;
    }
    else if ((pmp_cnfgn[pointer][3] == '1') && (pmp_cnfgn[pointer][4] == '0')){ // NA4 mode
        return 2;
    }
    else if ((pmp_cnfgn[pointer][3] == '1') && (pmp_cnfgn[pointer][4] == '1')){ // NAPOT mode
        return 3;
    }
}

int addr_search(char pmp_addr[64][13], int addr_mode[64], char *addr){
    /* Function that searches for the correct pmp entry based on the given memory address
    Takes 3 arguments:
    1. pmp_addr[64][13]: A 2D array that contains the data of all the pmp address registers in hex format
    2. addr_mode[64]: A 1D array that contains the address mode of the corresponding pmp entries
    3. addr: Hex string, which is the address of memory location to be looked for
    Return type is an int, which is the pmp entry number corresponing the relevant memory block*/
    int i = 0;
    int bool;
    while (i < 64){
        if (addr_mode[i] == 0){
            // OFF mode, so PMP entry is disabled
        }
        else if (addr_mode[i] == 1){
            // TOR Mode
            bool = TOR_addr_check(pmp_addr, addr, i);
        }
        else if (addr_mode[i] == 2){
            // NA4 Mode
            bool = NA4_addr_check(pmp_addr[i], addr);
        }
        else{
            // NAPOT Mode
            bool = NAPOT_addr_check(pmp_addr[i], addr);
        }
        if (bool == 1){ // If addr lies in the memory block protected by the ith pmp entry, than entry number is returned
            return i;
        }
        i++;
    }
    return 100; // No relevant entry found
}

int TOR_addr_check(char pmp_addr[64][13], char *addr, int pointer){
    /* This function checks if the searched address lies in the memory block protected by the given
    pmp entry, when PMP entry is in TOR mode 
    Takes 3 arguments:
    1. pmp_addr[64][13]: A 2D array containing the data of all 64 pmp address registers in hex format
    2. addr: A hexadecimal string for the address of the memory location that is being searched for
    3. pointer: pmp entry which needs to be checked
    Returns an int which verifies whether the searched address is protected by the given pmp entry
    A return value of 1 implies, yes it is protected by the given entry
    Otherwise, it returns 0*/
    
    char *temp1 = pmp_addr[pointer];  // Stores address data of current pmp entry
    if (pointer != 0){
        char *temp2 = pmp_addr[pointer-1]; // Stores address of previous pmp entry if current entry number is not 0
        while (*temp2){ // Repaces any '\n' or '\r' character with '\0'
            if ((*temp2 == '\n') || (*temp2 == '\r')){
                *temp2 = '\0';
            }
            temp2++;
        }
    }
    while (*temp1){ // Repaces any '\n' or '\r' character with '\0'
        if ((*temp1 == '\n') || (*temp1 == '\r')){
            *temp1 = '\0';
        }
        temp1++;
    }

    uint64_t pointer1_dec = strtoull(pmp_addr[pointer], NULL, 16); // Converts hex string to a 64-bit int
    uint64_t pointer2_dec;

    if (pointer != 0){
        pointer2_dec = strtoull(pmp_addr[pointer-1], NULL, 16); // Converts hex string to a 64-bit int if current pmp entry number is not 0
    }
    else{
        pointer2_dec = 0;
    }
    
    uint64_t input_dec = strtoull(addr, NULL, 16); 

    if ((input_dec >= pointer2_dec) && (input_dec < pointer1_dec)){
        return 1;
    }
    else{
        return 0;
    }
}

int NA4_addr_check(char pmp_addr_reg[13], char *addr){
    /* This function checks if the searched address lies in the memory block protected by the given
    pmp entry, when PMP entry is in NA4 mode 
    Takes 2 arguments:
    1. pmp_addr_reg[13]: A 1D array containing the data of the given pmp address register in hex format
    2. addr: A hexadecimal string for the address of the memory location that is being searched for
    Returns an int which verifies whether the searched address is protected by the given pmp entry
    A return value of 1 implies, yes it is protected by the given entry
    Otherwise, it returns 0*/
    char *temp = pmp_addr_reg; // Stores address data of current pmp entry
        while (*temp){ // Repaces any '\n' or '\r' character with '\0'
            if ((*temp == '\n') || (*temp == '\r')){
                *temp = '\0';
            }
            temp++;
        }
    uint64_t pointer_dec = strtoull(pmp_addr_reg, NULL, 16); // Converts hex string to a 64-bit int
    uint64_t input_dec = strtoull(addr, NULL, 16); // Converts hex string to a 64-bit int

    if ((input_dec >= pointer_dec) && (input_dec < pointer_dec + 3)){
        return 1;
    }
    else{
        return 0;
    }
}

int NAPOT_addr_check(char *pmp_addr_reg, char *addr){
    /* This function checks if the searched address lies in the memory block protected by the given
    pmp entry, when PMP entry is in NAPOT mode 
    Takes 2 arguments:
    1. pmp_addr_reg[13]: A 1D array containing the data of the given pmp address register in hex format
    2. addr: A hexadecimal string for the address of the memory location that is being searched for
    Returns an int which verifies whether the searched address is protected by the given pmp entry
    A return value of 1 implies, yes it is protected by the given entry
    Otherwise, it returns 0*/
    char bin_pmp_addr[35]; // Array to store address data of given pmp entry in binary format
    addr_bin_conv(pmp_addr_reg, bin_pmp_addr); // Converts hex address to binary format
    uint64_t offset = NAPOT_offset(bin_pmp_addr); // Finding encoded offset for the given pmp entry in NAPOT mode
    uint64_t base = NAPOT_base(bin_pmp_addr); // Finding encoded base address for the given pmp entry in NAPOT mode

    uint64_t search_addr = strtoull(addr, NULL, 16); // Converts hex string to a 64 bit int
    if ((search_addr >= base) && (search_addr < base + offset)){
        return 1;
    }
    else{
        return 0;
    }
}

void addr_bin_conv(char *pmp_addr_reg, char *bin_addr){
    /* This function converts a address data of at max 9 hex digits to a string of 34 binary digits
    It takes two arguments:
    1. pmp_addr_reg: String containing address data of a given pmp entry in hex format
    2. Empty string, which will store the binary form of address data
    Return type of the function is void*/
    int val;
    int i = 2;
    int pos = 0;

    // Inserting 2 MSB bits
    char *end_ptr = strchr(pmp_addr_reg, '\n'); // Index at which '\n' character is present
    int len = end_ptr - pmp_addr_reg - 2; // Length of hex string excluding "0x" and '\n'
    // Case where 9th hex character is missing. Implying it is 0
    if (len < 9){
        strncpy(bin_addr + pos, "00", 2);
        pos += 2;
    }
    // Case where 9th hex character is present. It will be a 2 bit character, since maximum bits are 34
    else if (len == 9){
        val = pmp_addr_reg[i] - '0';
        if (val == 0){
            strncpy(bin_addr + pos, "00", 2);
        }
        else if (val == 1){
            strncpy(bin_addr + pos, "01", 2);
        }
        else if (val == 2){
            strncpy(bin_addr + pos, "10", 2);
        }
        else if (val == 3){
            strncpy(bin_addr + pos, "11", 2);
        }
        pos += 2;
    }
    // Rest of the hex characters are of 4 bits, so if they are missing, this implies that they are 0
    for (int j = 0; j < 9 - len - 1; j++){
        strncpy(bin_addr + pos, bin[0], 4);
        pos += 4;
    }

    // Converting the available hex characters to binary
    while (pmp_addr_reg[i] != '\n'){
        if ((pmp_addr_reg[i] >= '0' ) && (pmp_addr_reg[i] <= '9')){
            val = pmp_addr_reg[i] - '0';
            strncpy(bin_addr + pos, bin[val], 4);
        }
        else if ((pmp_addr_reg[i] >= 'A') && (pmp_addr_reg[i] <= 'F')){
            val = pmp_addr_reg[i] - 'A' + 10;
            strncpy(bin_addr + pos, bin[val], 4);
        }
        else{
            val = pmp_addr_reg[i] - 'a' + 10;
            strncpy(bin_addr + pos, bin[val], 4);
        }
        pos += 4; 
        i++;
    }
    bin_addr[34] = '\0';
}

uint64_t NAPOT_offset(char bin_addr[35]){
    /* This function decodes the offset from pmp address register in NAPOT mode
    It takes a single argument:
    bin_addr[35]: A 1D array of characters, containing address for a given pmp entry in hex format*/
    int i = 31;
    int power = 2; 
    while (i > 1){
        power++;
        if (bin_addr[i] == '0'){

            break;
        }
        i--;
    }
    uint64_t offset = 1;
    for (int j = 0; j < power; j++){
        offset *= 2;
    }
    return offset;
}

uint64_t NAPOT_base(char bin_addr[35]){
    /* This function decodes the base address from pmp address register in NAPOT mode
    It takes a single argument:
    bin_addr[35]: A 1D array of characters, containing address for a given pmp entry in hex format*/
    int i = 31; 
    while (i > 1){
        if (bin_addr[i] == '0'){
            break;
        }
        else{
            bin_addr[i] = '0';
        }
        i--;
    }

    uint64_t base = strtoull(bin_addr, NULL, 2);
    return base;
}

int checkLock(char pmp_cnfgn[64][9]){
    /* This function checks if Lock bit is enabled in any pmp configuration register
    It takes a single argument:
    pmp_cnfgn[64][9]: A 2D array containing the data of all the pmp configuration registers in binary format
    Returns a 1 if any pmp config register has Lock bit enabled
    Otherwise it returns a 0*/
    for (int i = 0; i < 64; i++){
        if (pmp_cnfgn[i][0] == '1'){
            return 1;
        }
    }
    return 0;
}


void Perm_check(char pmp_cnfgn[][9], char oper, int pointer){
    /* Function to check relevant permissions of a given configuration register
    Takes 3 arguments:
    1. pmp_cnfgn[][9]: A 2D array containing the data of all the pmp configuration registers
    2. oper: R, W, X operation that is to be performed for relevant memory block
    3. pointer: Relevant configuration register number
    Return type is void
    Function prints whether there is an access fault or not*/

    switch (oper)
    {
    case 'R':
        if (pmp_cnfgn[pointer][7] == '1'){
            printf("No access fault.\n");
        }
        else{
            printf("Access fault. \n");
        }
        break;
    case 'W':
        if (pmp_cnfgn[pointer][6] == '1'){
            printf("No access fault.\n");
        }
        else{
            printf("Access fault. \n");
        }
        break;
    case 'X':
        if (pmp_cnfgn[pointer][5] == '1'){
            printf("No access fault.\n");
        }
        else{
            printf("Access fault. \n");
        }
        break;
    }
}