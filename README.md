### Building the Program:
The submitted C code can be built into an executable using gcc compiler, where the command `gcc PMP_check.c -o PMP_check` will be sufficient.

### PMP Configuration file:
While writing the program, the following assumptions were made regarding the configuration file:
- It consists of 128 lines of text only
- The first 64 lines contain pmp configuration register data
- Next 64 lines contain pmp address register data
- All the data is in hexadecimal format
- The hexadecimal digits can be either in upper or lower case
- The hexadecimal strings begin with '0x'
- PMP configuration register data can range from 1 to 2 hex characters
- PMP address register data can range from 1 to 9 hex characters
- There are no semi-colons or full stops at the end of each line and no additional text apart from the data in the registers
- **The pmp address register data in the file already contains the two least significant bits of the address, making it a 34 bit data**

### Running the Program:
The program can be run using the command `./PMP_check`, where 'PMP_check' is the executable name in this case, followed by the inputs to the program
The submitted program takes 4 inputs:
1. filepath: This is the path of the pmp configuration file
2. address: Address of the memory location for which the permissions are to be checked. This will be hex string of maximum 9 characters, excluding '0x'
3. mode: Privelege mode which can either 'M', 'S' or 'U'. No other input will be accepted
4. oper: The operation which is to be performed on the data in the desired memory location, which can be either 'R' (read), 'W' (write) or 'X' (execute). No other input will be accepted.

**No additional inputs will be accepted.**

Below is an example when running the program:
```
./PMP_check .\pmp_configuration.txt 0x560000 M R
```
### Output:
The program prints the hexadecimal data of all the pmp configuration and address registers like this:

![Output-ss](https://github.com/Muhammad-Furrukh/IDL-Programming-Challenge-Submission-Muhammad-Furrukh/blob/main/Screenshot%202025-02-08%20210705.png)

This is for the aid in debugging, where the relevant pmp register number and its data is printed out on the terminal.

At the end, the program prints whether an access fault occured or not, when accessing the given memory section.


