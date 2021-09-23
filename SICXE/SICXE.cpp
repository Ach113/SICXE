#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "parser.h"

using namespace std;

int main()
{
	Parser p("code.asm");
	p.generate_object_code();
}

