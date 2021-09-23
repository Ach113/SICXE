#include "parser.h"

#include <unordered_map>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>

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
	/* reads the source file, produce vector of lines */
	ifstream source(filename);
	vector<string> lines;
	string line;
	while (source) {
		getline(source, line);
		if (!(line[0] == '.')) // skip comments
			lines.push_back(line);
	}
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
}

Parser::Parser(string filename) {
	populate_optab(); // insert values in OPTAB
	this->LOCCTR = 0;
	this->SYMTAB = unordered_map<string, uint32_t> ();
	// tokenize the source code
	vector<Instruction> instructions;
	for (string line : get_lines(filename)) {
		vector<string> tokens = split(line); // split line by whitespace
		string label, opcode, value = "";

		switch (tokens.size()) {
			case 3:
				label = tokens[0]; opcode = tokens[1]; value = tokens[2];
				break;
			case 2:
				opcode = tokens[0]; value = tokens[1];
				break;
			case 1:
				opcode = tokens[0];
				break;
			default:
				throw "invalid instruction";
		}
		instructions.push_back(Instruction{ label, opcode, value });
	}
	this->instructions = instructions;
}


void Parser::generate_object_code() {
	// first pass
	for (Instruction ins : this->instructions) {
		// initialize the symbol table, insert all labels with LOCCTR=0
		// after all labels are resolved, all corresponding values will be nonzero
		if (ins.label != "") {
			this->SYMTAB[ins.label] = 0;
		}
	}

}


