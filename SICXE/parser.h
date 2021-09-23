#ifndef PARSER_H
#define PARSER_H

/* SOURCE CODE -> (PARSER) -> OBJECT CODE */

#include <vector>
#include <unordered_map>
#include <string>

using namespace std;

struct Instruction {
	string label, opcode, value;
};

class Parser {
	private:
		vector<Instruction> instructions;
		unordered_map<string, uint32_t> SYMTAB;
		uint32_t LOCCTR;
	public:
		Parser(string filename);
		void generate_object_code();
};

#endif