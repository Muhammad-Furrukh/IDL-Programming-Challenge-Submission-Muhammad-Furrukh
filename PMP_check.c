#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

void PMP_Check(char *filepath, char *address, char mode, char oper);
void Bin_convert(char pmp_cnfgn[][9], int num_reg);
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
    - Config file contains 129 lines
    - First 64 lines are the data of the corresponding 64 pmp configuration registers 
    - Next 64 lines are the data of the corresponding  64 pmp address registers
    - Last line is empty, so that the 64th address register data has a next line character at the end
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

    fclose(file);

    // Convert string of hexadecimal digits to binary string in pmpcnfgn registers
    Bin_convert(pmpcnfgn, 64);
    // Storing address modes of PMP entries in a separate array
    int addr_mode[64];
    mode_store(pmpcnfgn, addr_mode);
    int x = addr_search(pmpaddr, addr_mode, address); // Searches for the relevant pmp address and config register number
    
    // Correct configuration register has been found 
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

void Bin_convert(char pmp_cnfgn[][9], int num_reg){
    /*Function defined specifically for PMP configuration registers
    Converts the hexadecimal string in the set of pmp configuration registers and converts
    them into binary strings
    Takes two arguments:
    1. pmp_cnfgn[][9]: A 2D array containing the data of all the pmp configuration registers
    2. num_reg: An int which corresponds to the number of pmp configuration registers
    The function's return type is void
    Changes are made on default pmp configuration registers, not on a separate instance */
    int j, start;
    char *start_ptr;
    char temp[9]; // temporary variable to store binary string
    for (j = 0; j < num_reg; j++){
        // Finding the index of \n character in the address string
        start_ptr = strchr(pmp_cnfgn[j], '\n');
        start = start_ptr - pmp_cnfgn[j]; // LS hex digit is before this index
        int val;
        int repeat = 0;
        int pos = 0;

        while (repeat < 2){
            // Cases where hex string is of the form 0xn. In such cases MS hex digit is 0
            if ((pmp_cnfgn[j][start - 2] == 'x') && (pos != 0)){ 
                strncpy(temp + pos, bin[0], 4);
            }
            // Cases where hex string is of the form 0xmn. In such cases MS hex digit is m
            else if ((pmp_cnfgn[j][start - 2 + repeat] >= '0' ) && (pmp_cnfgn[j][start - 2 + repeat] <= '9')){
                val = pmp_cnfgn[j][start - 2 + repeat] - '0';
                strncpy(temp + pos, bin[val], 4);
            }
            else if ((pmp_cnfgn[j][start - 2 + repeat] >= 'A') && (pmp_cnfgn[j][start - 2 + repeat] <= 'F')){
                val = pmp_cnfgn[j][start - 2 + repeat] - 'A' + 10;
                strncpy(temp + pos, bin[val], 4);
            }
            else{
                val = pmp_cnfgn[j][start - 2 + repeat] - 'a' + 10;
                strncpy(temp + pos, bin[val], 4);
            }
            pos += 4; // After inserting the binary equivalent in upper half of array, insertion is to be done in lower half
            repeat++;
        }
        temp[8] = '\0';
        strcpy(pmp_cnfgn[j], temp); // binary string replacing hex string in configuration register
    }
}

void mode_store(char pmp_cnfgn[64][9], int addr_mode[64]){
    int i;
    for (i = 0; i < 64; i++){
        addr_mode[i] = MODE_check(pmp_cnfgn, i);
    }
}

int MODE_check(char pmp_cnfgn[64][9], int pointer){
    /* Function to check address mode of a given configuration register
    Takes 3 arguments:
    1. pmp_cnfgn[][9]: A 2D array containing the data of all the pmp configuration registers
    2. num_reg: An int corresponding to the total number of configuration registers
    3. pointer: register number for which the address mode is to be checked 
    Returns an int corresponding to address modes of the pmp configuration register
    */
    if ((pmp_cnfgn[pointer][3] == '0') && (pmp_cnfgn[pointer][4] == '0')){
        return 0;
    }
    else if ((pmp_cnfgn[pointer][3] == '0') && (pmp_cnfgn[pointer][4] == '1')){
        return 1;
    }
    else if ((pmp_cnfgn[pointer][3] == '1') && (pmp_cnfgn[pointer][4] == '0')){
        return 2;
    }
    else{
        return 3;
    }
}

int addr_search(char pmp_addr[64][13], int addr_mode[64], char *addr){
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
        if (bool == 1){
            return i;
        }
        i++;
    }
    return 100;
}

int TOR_addr_check(char pmp_addr[64][13], char *addr, int pointer){
    /* This function searches for the relevant PMP address register, given the relevant memory address
    mode is TOR
    Takes 4 arguments:
    1. pmp_addr[][13]: A 2D array containing the data of all 64 pmp address registers
    2. num_reg: An int corresponding to the number of pmp address registers
    3. addr: A hexadecimal string for the address of the memory location that is being searched for
    4. pointer: starting register number
    Returns an int which corresponds to the relevant address register number*/
    
    char *temp1 = pmp_addr[pointer];
    if (pointer != 0){
        char *temp2 = pmp_addr[pointer];
        while (*temp2){
            if ((*temp2 == '\n') && (*temp2 == '\r')){
                *temp2 = '\0';
            }
            temp2++;
        }
    }
    while (*temp1){
        if ((*temp1 == '\n') && (*temp1 == '\r')){
            *temp1 = '\0';
        }
        temp1++;
    }

    uint64_t pointer1_dec = strtol(pmp_addr[pointer], NULL, 16);
    uint64_t pointer2_dec = strtol(pmp_addr[pointer-1], NULL, 16);
    uint64_t input_dec = strtol(addr, NULL, 16);

    if ((input_dec >= pointer2_dec) && (input_dec < pointer1_dec)){
        return 1;
    }
    else{
        return 0;
    }
}

int NA4_addr_check(char pmp_addr_reg[13], char *addr){
    char *temp = pmp_addr_reg;
        while (*temp){
            if ((*temp == '\n') && (*temp == '\r')){
                *temp = '\0';
            }
            temp++;
        }
    uint64_t pointer_dec = strtol(pmp_addr_reg, NULL, 16);
    uint64_t input_dec = strtol(addr, NULL, 16);

    if ((input_dec >= pointer_dec) && (input_dec < pointer_dec + 3)){
        return 1;
    }
    else{
        return 0;
    }
}

int NAPOT_addr_check(char *pmp_addr_reg, char *addr){
    char bin_pmp_addr[35];
    addr_bin_conv(pmp_addr_reg, bin_pmp_addr);
    uint64_t offset = NAPOT_offset(bin_pmp_addr);
    uint64_t base = NAPOT_base(bin_pmp_addr);

    uint64_t search_addr = strtol(addr, NULL, 16);
    if ((search_addr >= base) && (search_addr < base + offset)){
        return 1;
    }
    else{
        return 0;
    }
}

void addr_bin_conv(char *pmp_addr_reg, char *bin_addr){
    char temp[35]; // temporary variable to store binary string
    int val;
    int i = 2;
    int pos = 0;

    // Inserting missing leading 0's
    char *end_ptr = strchr(pmp_addr_reg, '\n');
    int len = end_ptr - pmp_addr_reg - 2;
    char *two_zero = "00";
    strncpy(temp + pos, two_zero, 2);
    pos += 2;
    for (int j = 0; j < 9 - len - 1; j++){
        strncpy(temp + pos, bin[0], 4);
        pos += 4;
    }

    while (pmp_addr_reg[i] != '\n'){
        if ((pmp_addr_reg[i] >= '0' ) && (pmp_addr_reg[i] <= '9')){
            val = pmp_addr_reg[i] - '0';
            strncpy(temp + pos, bin[val], 4);
        }
        else if ((pmp_addr_reg[i] >= 'A') && (pmp_addr_reg[i] <= 'F')){
            val = pmp_addr_reg[i] - 'A' + 10;
            strncpy(temp + pos, bin[val], 4);
        }
        else{
            val = pmp_addr_reg[i] - 'a' + 10;
            strncpy(temp + pos, bin[val], 4);
        }
        pos += 4; 
        i++;
    }
    temp[34] = '\0';
    strcpy(bin_addr, temp);
}

uint64_t NAPOT_offset(char bin_addr[35]){
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
    uint64_t base = strtol(bin_addr, NULL, 2);
    return base;
}

int checkLock(char pmp_cnfgn[64][9]){
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