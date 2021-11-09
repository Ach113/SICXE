#include "parser.h"

#include <unordered_map>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>

using namespace std;

enum instruction_length {format1=8, format2=16, format3=24, format4=32};

enum literal_type {
	ascii, // C'_'
	hex,  // X'_'
	immediate, // #_
	indirect, // @_
	simple // _
};

struct Literal {
	vector<uint8_t> bytes;
	literal_type type;
};

// KEY -> mnemonic operand, VALUE -> (machine opcode, instruction format/length)
unordered_map<string, tuple<uint8_t, instruction_length>> OPTAB;

vector<string> get_lines(string filename) {
	/* reads the source file, produces vector of lines */
	ifstream source(filename);
	vector<string> lines;
	string line;
	while (source) {
		getline(source, line);
		if (line[0] != '.') // skip comments
			lines.push_back(line);
	}
	lines.pop_back();
	return lines;
}

// code by Bjarne Stoustrup, taken from: https://stackoverflow.blog/2019/10/11/c-creator-bjarne-stroustrup-answers-our-top-five-c-questions/
vector<string> split(const string& s) {
	stringstream ss(s);
	vector<string> words;
	for (string w; ss >> w; )
		words.push_back(w);
	return words;
}

void populate_optab() {
	OPTAB["ADD"] = tuple<uint8_t, instruction_length> (0x18, format3);
	OPTAB["STL"] = tuple<uint8_t, instruction_length>(0x14, format3);
	OPTAB["JSUB"] = tuple<uint8_t, instruction_length>(0x48, format3);
	OPTAB["COMP"] = tuple<uint8_t, instruction_length>(0x28, format3);
	OPTAB["J"] = tuple<uint8_t, instruction_length>(0x3C, format3);
	OPTAB["STA"] = tuple<uint8_t, instruction_length>(0x0C, format3);
	OPTAB["LDL"] = tuple<uint8_t, instruction_length>(0x08, format3);
	OPTAB["RSUB"] = tuple<uint8_t, instruction_length>(0x4C, format3);
	OPTAB["STCH"] = tuple<uint8_t, instruction_length>(0x54, format3);
	OPTAB["LDA"] = tuple<uint8_t, instruction_length>(0x00, format3);
	OPTAB["LDA"] = tuple<uint8_t, instruction_length>(0x00, format3);
	OPTAB["LDT"] = tuple<uint8_t, instruction_length>(0x74, format3);
	OPTAB["TD"] = tuple<uint8_t, instruction_length>(0xE0, format3);
	OPTAB["JEQ"] = tuple<uint8_t, instruction_length>(0x30, format3);
	OPTAB["LDCH"] = tuple<uint8_t, instruction_length>(0x50, format3);
	OPTAB["TIXR"] = tuple<uint8_t, instruction_length>(0xB8, format2);
	OPTAB["JLT"] = tuple<uint8_t, instruction_length>(0x38, format3);
	OPTAB["WD"] = tuple<uint8_t, instruction_length>(0xDC, format3);
	OPTAB["CLEAR"] = tuple<uint8_t, instruction_length>(0x4, format2);
}

Parser::Parser(string filename) {
	populate_optab(); // insert values in OPTAB
	this->LOCCTR = 0;
	this->SYMTAB = unordered_map<string, int> ();
	// populate SYMTAB
	this->SYMTAB["A"] = 0;
	this->SYMTAB["X"] = 1;
	this->SYMTAB["L"] = 2;
	this->SYMTAB["B"] = 3;
	this->SYMTAB["S"] = 4;
	this->SYMTAB["T"] = 5;
	this->SYMTAB["F"] = 6;
	// tokenize the source code
	vector<Instruction> instructions;
	for (string line : get_lines(filename)) {
		vector<string> tokens = split(line); // split line by whitespace
		string label, opcode, operand = "";

		switch (tokens.size()) {
			case 3:
				label = tokens[0]; opcode = tokens[1]; operand = tokens[2];
				break;
			case 2:
				opcode = tokens[0]; operand = tokens[1];
				break;
			case 1:
				opcode = tokens[0];
				break;
			default:
				throw "invalid instruction";
		}
		instructions.push_back(Instruction{ label, opcode, operand });
	}
	this->instructions = instructions;
}

bool is_label(string s) {

	if ((s[0] == 'X' || s[0] == 'C') && s[1] == '\'') {
		return false;
	}

	if (all_of(s.begin(), s.end(), [](char c) {return isdigit(c); })) {
		return false;
	}

	return true;
}

bool is_base_relative(int disp) {
	if (disp >= -2047 && disp <= 2047)
		return false;
	return true;
}

// converts hex representation of string to binary string
string convert_opcode(string opcode) {
	string binary[16] = {
		"0000",
		"0001",
		"0010",
		"0011",
		"0100",
		"0101",
		"0110",
		"0111",
		"1000",
		"1001",
		"1010",
		"1011",
		"1100",
		"1101",
		"1110",
		"1111"
	};
	uint8_t machine_code = get<0>(OPTAB[opcode]);
	return binary[machine_code >> 4] + binary[machine_code & 0x0f];
}

// returns number of bytes associated with operrand (supports hex, char and decimal)
int get_nbytes(string operand) {
	unsigned int x;
	stringstream ss;
	switch (operand[0]) {
		case 'X':
			ss << std::hex << operand.substr(2, operand.length() - 2);
			ss >> x;
			return x;
		case 'C':
			return operand.length() - 2;
		default:
			return stoi(operand);
	}
}


void Parser::run() {
	// first pass
	vector<string> labels;
	for (Instruction ins : this->instructions) {
		// populate labels vector
		if (ins.label != "") {
			this->SYMTAB[ins.label] = -1;
			labels.push_back(ins.label);
		}
	}

	// used to store LOCCTR per instruction, helps avoid recalculating LOCCTR
	vector<int> PCvector(this->instructions.size());

	// resolve all the labels
	for (int i = 0; i < this->SYMTAB.size(); i++) {
		this->LOCCTR = 0x405D;
		int index = 0;
		for (Instruction ins : this->instructions) {
			PCvector[index] = this->LOCCTR;
			// if current instruction has a label
			if (ins.label != "") {
				// if it is a label, check if its been resolved
				if (is_label(ins.operand) && ins.opcode == "EQU") {
					this->SYMTAB[ins.label] = this->SYMTAB[ins.operand];
					PCvector[index] = this->SYMTAB[ins.operand];
					index++;
					continue;
				}
				else {
					this->SYMTAB[ins.label] = this->LOCCTR;
				}
			}
			// increment LOCCTR
			if (ins.opcode == "BYTE") {
				this->LOCCTR += get_nbytes(ins.operand);
			} 
			else if (ins.opcode == "RESW") {
				this->LOCCTR += 3;
			}
			else if (ins.opcode == "RESB") {
				this->LOCCTR += 1;
			}
			else {
				string opcode = (ins.opcode[0] == '+') ? ins.opcode.substr(1) : ins.opcode;
				this->LOCCTR += get<1>(OPTAB[ins.opcode]) / 8;
			}
			index++;
		}
	}

	// check if all labels have been resolved
	for (string label : labels) {
		if (this->SYMTAB[label] == -1) {
			throw "Not all labels have been resoved";
		}
	}

	// final pass
	this->LOCCTR = 0x405D;
	int index = 0;
	for (Instruction ins : this->instructions) {
		int PC = PCvector[index];
		index++;
		/// calculate the object code for given instruction
		// get `e` flag
		bool e = (ins.opcode[0] == '+') ? true : false;
		// remove `+` from the opcode to allow OPTAB addressing
		string opcode = (ins.opcode[0] == '+') ? ins.opcode.substr(1) : ins.opcode;

		// handle assembly directives
		if (ins.opcode == "BYTE") {

		}
		else if (ins.opcode == "WORD") {

		}
		else if (ins.opcode == "RESB") {

		}
		else if (ins.opcode == "RESW") {

		}
		else if (ins.opcode == "EQU") {

		}
		else {
			string machine_opcode = convert_opcode(ins.opcode);
			bool n, i = true;
			bool x, b, p = false;
			/*
				MECHANISM FOR SETTING X IS NEEDED HERE
			*/
			// determine n, i flags
			if (ins.operand[0] == '@') {
				ins.operand = ins.operand.substr(1); // remove '@'
				n = true;
				i = false;
			} 
			else if (ins.operand[0] == '#') {
				ins.operand = ins.operand.substr(1); // remove '#'
				n = false;
				i = true;
			}
			// determine b, p flags

		}

		/// display the instruction
		cout << std::hex << PC << " " << ins.label << " " << ins.opcode << " " << ins.operand << endl;
	}
	// display SYMTAB
	cout << "\nSYMTAB:" << endl;
	for (string label : labels) {
		cout << "    " << label << " > " << std::hex << SYMTAB[label] << endl;
	}
}


