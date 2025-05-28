#include "iec61850ClientManger.h"
#include <stdlib.h>
#include <stdio.h>
#include "icd_parse.h"


iec61850ClientManger::iec61850ClientManger(/* args */)
{
}

iec61850ClientManger::~iec61850ClientManger()
{
}

void iec61850ClientManger::init(const char* configFilePath)
{
    IcdParser parser;
    auto nodes = parser.parse(configFilePath);
    parser.printParseResult(nodes);
}
    
  