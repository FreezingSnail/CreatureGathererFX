#pragma once
#include "../../src/lib/ReadData.hpp"

#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <map>

#define CREATURECSV "data/Creaturesheet.csv"

struct CSVCreature {
    std::string name;
    int id;
    std::string type1;
    std::string type2;
    int evelevel;
    int atk;
    int deff;
    int spcatk;
    int spcdef;
    int hp;
    int spd;
    int move1;
    int move2;
    int move3;
    int move4;
};

#include <iostream>

void printCSVCreature(const CSVCreature &creature) {
    std::string blue = "\033[1;34m";
    std::string reset = "\033[0m";

    std::cout << blue << "Name: " << creature.name << reset << "\n";
    std::cout << blue << "ID: " << creature.id << reset << "\n";
    std::cout << blue << "Type 1: " << creature.type1 << reset << "\n";
    std::cout << blue << "Type 2: " << creature.type2 << reset << "\n";
    std::cout << blue << "Evo Level: " << creature.evelevel << reset << "\n";
    std::cout << blue << "Atk Seed: " << creature.atk << reset << "\n";
    std::cout << blue << "Def Seed: " << creature.deff << reset << "\n";
    std::cout << blue << "Spc Atk Seed: " << creature.spcatk << reset << "\n";
    std::cout << blue << "Spc Def Seed: " << creature.spcdef << reset << "\n";
    std::cout << blue << "HP Seed: " << creature.hp << reset << "\n";
    std::cout << blue << "Spd Seed: " << creature.spd << reset << "\n";
    std::cout << blue << "Move 1: " << creature.move1 << reset << "\n";
    std::cout << blue << "Move 2: " << creature.move2 << reset << "\n";
    std::cout << blue << "Move 3: " << creature.move3 << reset << "\n";
    std::cout << blue << "Move 4: " << creature.move4 << reset << "\n";
}

std::map<std::string, int> typeMap = {
    {"spirit", static_cast<int>(Type::SPIRIT)}, {"water", static_cast<int>(Type::WATER)}, {"wind", static_cast<int>(Type::WIND)},
    {"earth", static_cast<int>(Type::EARTH)},   {"fire", static_cast<int>(Type::FIRE)},   {"lightning", static_cast<int>(Type::LIGHTNING)},
    {"plant", static_cast<int>(Type::PLANT)},   {"elder", static_cast<int>(Type::ELDER)}, {"none", static_cast<int>(Type::NONE)},
};

int getTypeValue(const std::string &type) {
    return typeMap[type];
}

std::string readLineFromCSV(const std::string &filename, int lineIndex) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << "\n";
        exit(1);
    }
    std::string line;
    for (int i = 0; i <= lineIndex + 1; ++i) {
        if (!std::getline(file, line)) {
            std::cout << "Line " << i << " does not exist in the file\n";
            exit(1);   // Return an empty string if the line doesn't exist
        }
    }
    return line;
}

CSVCreature parseCSVLineToCreature(const std::string &line) {
    std::stringstream ss(line);
    std::string field;
    CSVCreature creature;

    std::getline(ss, creature.name, '\t');
    std::getline(ss, field, '\t');
    creature.id = std::stoi(field);
    std::getline(ss, creature.type1, '\t');
    std::getline(ss, creature.type2, '\t');
    std::getline(ss, field, '\t');
    creature.evelevel = std::stoi(field);
    std::getline(ss, field, '\t');
    creature.atk = std::stoi(field);
    std::getline(ss, field, '\t');
    creature.deff = std::stoi(field);
    std::getline(ss, field, '\t');
    creature.spcatk = std::stoi(field);
    std::getline(ss, field, '\t');
    creature.spcdef = std::stoi(field);
    std::getline(ss, field, '\t');
    creature.hp = std::stoi(field);
    std::getline(ss, field, '\t');
    creature.spd = std::stoi(field);
    std::getline(ss, field, '\t');
    creature.move1 = std::stoi(field);
    std::getline(ss, field, '\t');
    creature.move2 = std::stoi(field);
    std::getline(ss, field, '\t');
    creature.move3 = std::stoi(field);
    std::getline(ss, field, '\t');
    creature.move4 = std::stoi(field);

    return creature;
}

CreatureData_t CSVCreatureConvert(const CSVCreature &csvCreature) {
    CreatureData_t creature;
    creature.id = csvCreature.id;
    creature.type1 = getTypeValue(csvCreature.type1);
    creature.type2 = getTypeValue(csvCreature.type2);
    creature.evoLevel = csvCreature.evelevel;
    creature.atkSeed = csvCreature.atk;
    creature.defSeed = csvCreature.deff;
    creature.spcAtkSeed = csvCreature.spcatk;
    creature.spcDefSeed = csvCreature.spcdef;
    creature.hpSeed = csvCreature.hp;
    creature.spdSeed = csvCreature.spd;
    creature.move1 = csvCreature.move1;
    creature.move2 = csvCreature.move2;
    creature.move3 = csvCreature.move3;
    creature.move4 = csvCreature.move4;

    return creature;
}

#define OPTCSV "data/opponents.csv"

struct OpponentCreature {
    int id;
    int level;
    int moves[4];
};

uint32_t convertToUInt32(const OpponentCreature &creature) {

    // std::cout << "Creature ID: " << creature.id << "\n";
    // std::cout << "Creature Level: " << creature.level << "\n";
    // std::cout << "Creature Moves: " << creature.moves[0] << " " << creature.moves[1] << " " << creature.moves[2] << " " <<
    // creature.moves[3]
    //           << "\n";

    uint32_t result = 0;
    result |= ((creature.id & 0x1111) << 25);   // Shift id to the left by 25 bits and bitwise OR with result
    // std::cout << "level id bf " << std::bitset<8>((creature.id)) << "\n";
    // std::cout << "level id af " << std::bitset<8>((creature.id & 0x1111)) << "\n";
    // std::cout << "Result after id: " << std::bitset<32>(result) << "\n";
    result |= ((creature.level) << 20);   // Shift level to the left by 20 bits and bitwise OR with result
    // std::cout << "level bin bf " << std::bitset<8>((creature.level)) << "\n";
    // std::cout << "level bin af " << std::bitset<8>((creature.level & 0x1111)) << "\n";
    // std::cout << "Result after level: " << std::bitset<32>(result) << "\n";
    result |= ((creature.moves[3] & 0x1111) << 15);   // Shift move4 to the left by 15 bits and bitwise OR with result
    // std::cout << "Result after move4: " << std::bitset<32>(result) << "\n";
    result |= ((creature.moves[2] & 0x1111) << 10);   // Shift move3 to the left by 10 bits and bitwise OR with result
    // std::cout << "Result after move3: " << std::bitset<32>(result) << "\n";
    result |= ((creature.moves[1] & 0x1111) << 5);   // Shift move2 to the left by 5 bits and bitwise OR with result
    // std::cout << "Result after move2: " << std::bitset<32>(result) << "\n";
    result |= (creature.moves[0] & 0x1111);   // No need to shift move1 and bitwise OR with result
    // std::cout << "Result after move1: " << std::bitset<32>(result) << "\n";
    return result;
}

struct OpponentCSV {
    int opID;
    OpponentCreature creatures[3];
};

OpponentSeed_t convertToOpponentSeed(const OpponentCSV &opponent) {
    OpponentSeed_t opponentSeed;
    opponentSeed.firstCreature = convertToUInt32(opponent.creatures[0]);
    opponentSeed.secondCreature = convertToUInt32(opponent.creatures[1]);
    opponentSeed.thirdCreature = convertToUInt32(opponent.creatures[2]);
    return opponentSeed;
}

OpponentCSV parseOpponentCSVLine(const std::string &line) {
    std::stringstream ss(line);
    OpponentCSV opponent;

    ss >> opponent.opID;
    ss.ignore();   // Ignore the comma

    for (int i = 0; i < 3; ++i) {
        ss >> opponent.creatures[i].id;
        ss.ignore();   // Ignore the comma

        ss >> opponent.creatures[i].level;
        ss.ignore();   // Ignore the comma

        for (int j = 0; j < 4; ++j) {
            ss >> opponent.creatures[i].moves[j];
            ss.ignore();   // Ignore the comma
        }
    }

    return opponent;   // Return the Opponent struct
}