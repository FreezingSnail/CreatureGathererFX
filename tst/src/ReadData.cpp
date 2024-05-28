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
            std::cout << "Line " << lineIndex << " does not exist in the file\n";
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

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
//            IMPLENTATIONS FOR TESTS
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

MoveBitSet getMovePack(uint8_t index) {
    MoveBitSet move;
    // uint24_t rowAddress = MoveData::movePack + sizeof(MoveBitSet) * index;
    // FX::readDataObject(rowAddress, move);
    return move;
}

// TODO: The rates dont exist yet in flash data
uint8_t getEffectRate(Effect effect) {
    uint8_t rate;
    // uint24_t rowAddress = MoveData::effectRates + sizeof(uint8_t) * static_cast<uint8_t>(effect);
    // FX::readDataObject(rowAddress, rate);
    // return rate;
    return 100;
}

// TODO: add a bitarray for the effect targets
bool selfEffect(Effect effect) {
    uint32_t effectTargets;
    // FX::readDataObject(MoveData::selfEffect, effectTargets);
    return effectTargets >> uint8_t(effect) & 1 == 1;
}

OpponentSeed_t readOpponentSeed(uint8_t index) {
    OpponentSeed_t seed = OpponentSeed_t{0, 0, 1};
    // uint24_t rowAddress = FX::readIndexedUInt24(opts, index);
    // FX::readDataObject(rowAddress, seed);
    return seed;
}

CreatureData_t getCreatureFromStore(uint8_t id) {
    CreatureData_t cseed;
    std::string line = readLineFromCSV(CREATURECSV, id);
    CSVCreature csvCreature = parseCSVLineToCreature(line);
    // printCSVCreature(csvCreature);
    cseed = CSVCreatureConvert(csvCreature);
    return cseed;
}

void arenaLoad(Creature *creature, uint24_t addr, uint8_t lvl) {
    uint8_t data[4];
    // data[0] = FX::readIndexedUInt8(addr, 1);
    // data[1] = FX::readIndexedUInt8(addr, 2);
    // data[2] = FX::readIndexedUInt8(addr, 3);
    // data[3] = FX::readIndexedUInt8(addr, 4);

    // creature->id = FX::readIndexedUInt8(addr, 0);
    CreatureData_t cSeed = getCreatureFromStore(creature->id);

    creature->loadTypes(cSeed);
    creature->level = lvl;
    creature->setStats(cSeed);
    // creature->setMove(FX::readIndexedUInt8(addr, 1), 0);
    // creature->setMove(FX::readIndexedUInt8(addr, 2), 1);
    // creature->setMove(FX::readIndexedUInt8(addr, 3), 2);
    // creature->setMove(FX::readIndexedUInt8(addr, 4), 3);
}

void ReadOpt(Opponent *opt, uint8_t index) {
    uint24_t creatures[3];
    // uint24_t addr = Teams::teamList + sizeof(uint24_t) * 7 * index;
    // creatures[0] = FX::readIndexedUInt24(addr, 0);
    // creatures[1] = FX::readIndexedUInt24(addr, 1);
    // creatures[2] = FX::readIndexedUInt24(addr, 2);
    // opt->levels[0] = uint8_t(FX::readIndexedUInt24(addr, 3));
    // opt->levels[1] = uint8_t(FX::readIndexedUInt24(addr, 4));
    // opt->levels[2] = uint8_t(FX::readIndexedUInt24(addr, 5));
    arenaLoad(&opt->party[0], creatures[0], opt->levels[0]);
    arenaLoad(&opt->party[0], creatures[1], opt->levels[1]);
    arenaLoad(&opt->party[0], creatures[2], opt->levels[2]);

    // opt->nameptr = FX::readIndexedUInt24(addr, 6);
}

void loadEncounterOpt(Opponent *opt, uint8_t id, uint8_t level) {
    CreatureData_t cseed;
    // uint24_t rowAddress = CreatureData::creatureData + (sizeof(CreatureData_t) * id);
    // FX::readDataObject(rowAddress, cseed);
    // opt->levels[0] = 2;
    // opt->levels[1] = 0;
    // opt->levels[2] = 0;
    // load(&opt->party[0], cseed);
    //  this->party[1].load(eseed);
    //  this->party[2].load(eseed);
}