//
// Created by adebayo on 4/19/21.
//

#include <sstream>
#include "CaptionsParser.h"

std::string libmediafy::MCPTContainer::to_string() {
    std::stringstream ss{};

    std::string langs;
    for (int i = 0; i < languages.size(); i++) {
        langs += languages[i] + ";";
    }

    return ss.str();
}

libmediafy::MCPTContainer *libmediafy::CaptionsParser::parse(std::string source) {
    return nullptr;
}
