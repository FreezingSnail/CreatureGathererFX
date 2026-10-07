#include "Type.hpp"

const Modifier typeTable[TypeCount][TypeCount] TYPE_TABLE_STORAGE = {
    // Spirit
    {Modifier::Same, Modifier::Same, Modifier::Same, Modifier::Same,
     Modifier::Same, Modifier::Same, Modifier::Same, Modifier::None,
     Modifier::None},
    // Water
    {Modifier::Same, Modifier::Same, Modifier::Double, Modifier::Half,
     Modifier::Double, Modifier::Same, Modifier::Half, Modifier::Half,
     Modifier::None},
    // Wind
    {Modifier::Same, Modifier::Same, Modifier::Same, Modifier::Double,
     Modifier::Half, Modifier::Half, Modifier::Double, Modifier::Half,
     Modifier::None},
    // Earth
    {Modifier::Same, Modifier::Double, Modifier::Half, Modifier::Same,
     Modifier::Same, Modifier::Double, Modifier::Half, Modifier::Half,
     Modifier::None},
    // Fire
    {Modifier::Same, Modifier::None, Modifier::Double, Modifier::Half,
     Modifier::Same, Modifier::Double, Modifier::Double, Modifier::Same,
     Modifier::None},
    // Lightning
    {Modifier::Same, Modifier::Double, Modifier::Double, Modifier::Half,
     Modifier::Same, Modifier::Double, Modifier::Double, Modifier::Same,
     Modifier::None},
    // Plant
    {Modifier::Same, Modifier::None, Modifier::Double, Modifier::Half,
     Modifier::Same, Modifier::Double, Modifier::Double, Modifier::Same,
     Modifier::None},
    // Elder
    {Modifier::Same, Modifier::None, Modifier::Double, Modifier::Half,
     Modifier::Same, Modifier::Double, Modifier::Double, Modifier::Same,
     Modifier::None},
    // Status row remains all zero-initialized Modifier::None values.
    {},
};
