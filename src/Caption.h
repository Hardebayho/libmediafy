//
// Created by adebayo on 4/19/21.
//

#ifndef LIBMEDIAFYANDROID_CAPTION_H
#define LIBMEDIAFYANDROID_CAPTION_H

#include <string>
#include <map>

#include "json11.hpp"

namespace libmediafy {

    struct Cue {
        double from;
        double to;
        std::vector<std::string> languages;
        std::map<std::string, std::string> data;

        bool operator<(const Cue& rhs) const {
            return from < rhs.from && to < rhs.to;
        }
    };

    struct Captions {
        std::string software;
        std::vector<std::string> languages;
        std::string version;
        std::string created;
        std::vector<Cue> cues;

        /// Parse a caption data (MCPT style)
        static Captions* parse(std::string data) {
            std::string error;
            json11::Json json{json11::Json::parse(data, error)};
            if (!error.empty()) return nullptr;

            // Check relevant fields
            if (!json["languages"].is_array() || !json["software"].is_string()
                || !json["version"].is_string() || !json["created"].is_string()
                || !json["captions"].is_array()) return nullptr;

            Captions* captions = new(std::nothrow) Captions();
            if (!captions) return nullptr;

            auto language_items = json["languages"].array_items();
            bool good = true;
            std::for_each(language_items.begin(), language_items.end(), [&](json11::Json& json) {
                if (!good || !(good = json.is_string())) return;
                captions->languages.push_back({json.string_value()});
            });

            if (!good) {
                delete captions;
                return nullptr;
            }

            auto software = json["software"].string_value();
            if (software != "Mediafy") {
                delete captions;
                return nullptr;
            }

            captions->software = {software};

            auto version = json["version"].string_value();
            if (version.empty()) {
                delete captions;
                return nullptr;
            }

            captions->version = version;

            auto created = json["created"].string_value();
            if (created.empty()) {
                delete captions;
                return nullptr;
            }

            captions->created = created;

            auto captions_val = json["captions"].array_items();
            for (auto& caption : captions_val) {
                if (!caption.is_object()) {
                    delete captions;
                    return nullptr;
                }

                if (!caption["from"].is_number() || !caption["to"].is_number() || !caption["caption"].is_array()) {
                    delete captions;
                    return nullptr;
                }

                Cue cue;
                cue.from = caption["from"].number_value();
                cue.to = caption["to"].number_value();
                cue.languages = captions->languages;

                auto obj_items = caption["caption"].object_items();
                if (obj_items.size() != captions->languages.size()) {
                    delete captions;
                    return nullptr;
                }

                for (int i = 0; i < captions->languages.size(); i++) {
                    cue.data[captions->languages[i]] = obj_items[captions->languages[i]].string_value();
                }

                captions->cues.push_back(cue);
            }
        }
    };
}

#endif //LIBMEDIAFYANDROID_CAPTION_H
