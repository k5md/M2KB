#ifndef CONFIG_H
#define CONFIG_H
#include <fstream>
#include <inicpp.h>
#include <string.h>
#include <map>
struct Config {
    std::string configPath = "config.ini";
    std::string keymapPath = "keymap.cfg";
    int altcmkp = 0;
    char keymap[256];

    Config() {
        for (size_t i = 0; i < 255; i++) {
            keymap[i] = 0; // idx is midi key code (0-127, typically), value is vk code (1-254)
        }
    }

    void save() {
        ini::IniFile storage;
        storage["Compatibility"]["altcmkp"] = altcmkp;
        storage.save(configPath);

        std::ofstream out;
        out.open(keymapPath);
        if (out) out.write(keymap, 256);
        out.close();
        return;
    }

    void load() {
        ini::IniFile storage;
        storage.load(configPath);
        if (storage["Compatibility"]["altcmkp"].as<std::string>() != "") altcmkp = storage["Compatibility"]["altcmkp"].as<int>();

        std::ifstream in;
        in.open(keymapPath);
        if (in) in.getline(keymap, 256);
        in.close();
        return;
    }

    auto getEntries() {
        std::map<std::string, std::string> entries = {
            { "altcmkp",  std::to_string(altcmkp) },
        };
        return entries;
    }
};
#endif
