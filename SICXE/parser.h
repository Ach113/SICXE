#ifndef PARSER_H
#define PARSER_H

/* SOURCE CODE -> (PARSER) -> OBJECT CODE */

#include <vector>
#include <unordered_map>
#include <string>

using namespace std;

struct Instruction {
	string label, opcode, operand;
};

class Parser {
	private:
		vector<Instruction> instructions;
		unordered_map<string, int> SYMTAB;
		uint32_t LOCCTR;
	public:
		Parser(string filename);
		void run();
};

#endif