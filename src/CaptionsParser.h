//
// Created by adebayo on 4/19/21.
//

#ifndef LIBMEDIAFYANDROID_CAPTIONSPARSER_H
#define LIBMEDIAFYANDROID_CAPTIONSPARSER_H
#include <string>
#include <vector>

namespace libmediafy {

struct Caption {
    std::string language;
    std::string text;
};

struct Cue {
    double from;
    double to;
    std::vector<Caption> captions;
};

struct MCPTContainer {
    std::string software;
    int version;
    std::string created;
    std::vector<std::string> languages;
    std::vector<Cue> cues;

    std::string to_string();
};

class CaptionsParser {
public:
    explicit CaptionsParser() = default;

    /// Parse the subtitle/caption contained in this string
    MCPTContainer* parse(std::string source);
private:

};
}

#endif //LIBMEDIAFYANDROID_CAPTIONSPARSER_H
