#pragma once
constexpr uint8_t creatureSpritesWidth = 32;
constexpr uint8_t creatureSpritesHeight = 32;

const uint8_t PROGMEM creatureSprites[] = {
    creatureSpritesWidth, creatureSpritesHeight,
    // Frame 0
    0xE8, 0x54, 0x14, 0x14, 0xE4, 0x04, 0xC8, 0xA8, 0x28, 0x28, 0xC8, 0x10,
    0x00, 0x00, 0x80, 0x40, 0x40, 0x20, 0x20, 0xA0, 0xA0, 0xA0, 0xA0, 0x20,
    0x20, 0x40, 0x40, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE1, 0x39, 0x07,
    0x00, 0x00, 0x01, 0xE2, 0x32, 0x1E, 0x01, 0x7C, 0x82, 0x01, 0x38, 0xC4,
    0x82, 0x02, 0x31, 0x1D, 0x04, 0x84, 0x64, 0x3D, 0x01, 0xC1, 0x63, 0x1C,
    0x01, 0x82, 0x7C, 0x00, 0x00, 0x87, 0xBC, 0xC0, 0xC0, 0x60, 0x6E, 0x7B,
    0x60, 0x30, 0x30, 0x30, 0x30, 0x31, 0x32, 0x34, 0x74, 0x79, 0x79, 0x7D,
    0x7D, 0x7C, 0x7C, 0xFA, 0xA9, 0xA4, 0xA4, 0x62, 0x41, 0xC0, 0x80, 0x80,
    0x00, 0x03, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x08, 0x10, 0x10, 0x10,
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x11, 0x11, 0x11, 0x11, 0x11, 0x12,
    0x1E, 0x0C, 0x09, 0x0D, 0x07, 0x06, 0x03, 0x03,

    // Frame 1
    0x78, 0xA4, 0xB4, 0x84, 0x78, 0x00, 0x78, 0xA4, 0xB4, 0x84, 0x78, 0x00,
    0x00, 0x00, 0x80, 0x80, 0x40, 0x20, 0x20, 0x20, 0x70, 0x90, 0xA8, 0xB8,
    0x48, 0x1C, 0x24, 0x24, 0xAA, 0x72, 0x0C, 0x00, 0x00, 0x00, 0x1F, 0xF0,
    0x00, 0x00, 0xFC, 0x07, 0x00, 0x01, 0x78, 0xA4, 0x4E, 0xD9, 0x90, 0x20,
    0x23, 0x22, 0x42, 0x82, 0x84, 0x84, 0x88, 0x69, 0x29, 0x95, 0xF2, 0x1E,
    0x01, 0x00, 0x00, 0x00, 0x00, 0xE0, 0xBE, 0xC1, 0xC0, 0x60, 0x63, 0x7E,
    0x60, 0x38, 0x36, 0x11, 0x92, 0x94, 0xD8, 0xD9, 0xD1, 0xD1, 0xD1, 0x52,
    0x52, 0x52, 0x7A, 0x6A, 0x65, 0xE3, 0xE0, 0xDC, 0x62, 0xA2, 0xA2, 0x1C,
    0x07, 0x0F, 0x0D, 0x18, 0x18, 0x30, 0x30, 0x30, 0x31, 0x71, 0x71, 0x61,
    0x68, 0x68, 0x6C, 0x64, 0x64, 0x62, 0x62, 0x62, 0x6A, 0x6B, 0x65, 0x25,
    0x25, 0x25, 0x36, 0x16, 0x1B, 0x0E, 0x0C, 0x07,

    // Frame 2
    0x00, 0x70, 0x8C, 0x44, 0x34, 0x8C, 0x70, 0x00, 0x00, 0x70, 0x8C, 0x44,
    0x34, 0x8C, 0x70, 0x00, 0x00, 0x00, 0x80, 0xC0, 0xF0, 0x78, 0x1C, 0x1C,
    0x22, 0x62, 0x5E, 0x4E, 0x46, 0x6C, 0xF0, 0x00, 0x00, 0x00, 0x0D, 0x77,
    0xC1, 0x01, 0x00, 0x80, 0xC0, 0x40, 0x31, 0x1D, 0x81, 0xC1, 0xF8, 0x1C,
    0x06, 0x3F, 0x63, 0x80, 0x03, 0x06, 0x0C, 0x18, 0x10, 0x20, 0x20, 0x20,
    0x20, 0x10, 0x19, 0xFF, 0xF0, 0x18, 0x8C, 0x06, 0x47, 0x0C, 0x3F, 0x61,
    0x40, 0x58, 0xF8, 0x86, 0x01, 0x01, 0x03, 0x06, 0x0C, 0x18, 0x10, 0x30,
    0x21, 0x46, 0xC4, 0x88, 0x88, 0x88, 0x88, 0x88, 0x48, 0x24, 0x12, 0xFF,
    0x01, 0x06, 0x04, 0x08, 0x10, 0x11, 0x20, 0x20, 0x25, 0x20, 0x42, 0x41,
    0x49, 0x52, 0x44, 0x48, 0x48, 0x48, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50,
    0x48, 0x48, 0x6C, 0x64, 0x72, 0x39, 0x39, 0x39,

    // Frame 3
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x88,
    0x04, 0x02, 0x02, 0x01, 0x01, 0x11, 0x68, 0x90, 0xD0, 0x60, 0x00, 0x00,
    0x00, 0x00, 0x60, 0xD0, 0x90, 0x68, 0x11, 0x01, 0x01, 0x02, 0x02, 0x04,
    0x88, 0x70, 0x00, 0x00, 0x00, 0xF0, 0x08, 0x04, 0x65, 0x86, 0x0A, 0xF4,
    0x04, 0xF4, 0x08, 0xE8, 0x18, 0xC8, 0x28, 0x19, 0x19, 0x28, 0xC8, 0x18,
    0xE8, 0x08, 0xF4, 0x04, 0xF4, 0x0A, 0x86, 0x65, 0x04, 0x08, 0xF0, 0x00,
    0x00, 0x00, 0x01, 0x02, 0x02, 0x02, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00,
    0x01, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x01, 0x02, 0x02, 0x02, 0x01, 0x00, 0x00,

    // Frame 4
    0x00, 0xE0, 0x10, 0x08, 0x04, 0x04, 0x04, 0xE4, 0x14, 0x18, 0x10, 0xE0,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x08,
    0x0C, 0x14, 0x64, 0x04, 0x08, 0xF0, 0x00, 0x00, 0xC0, 0x67, 0x28, 0x10,
    0x60, 0xA0, 0x20, 0x21, 0xA0, 0x90, 0x88, 0x47, 0x40, 0x40, 0x60, 0x20,
    0x20, 0x60, 0x40, 0x40, 0x80, 0x80, 0x00, 0xC1, 0x3C, 0x02, 0x02, 0x1C,
    0xE1, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x06, 0x04, 0x0C, 0x09, 0xFD,
    0x83, 0x18, 0x24, 0x34, 0x24, 0x98, 0x40, 0x80, 0x80, 0x40, 0x98, 0x24,
    0x34, 0x25, 0x19, 0x83, 0x7C, 0xC4, 0x84, 0x03, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xF8, 0x04, 0x02, 0x03, 0xE1, 0x19, 0x09, 0xEE, 0x36, 0x14,
    0xCC, 0x38, 0x09, 0x09, 0x09, 0x09, 0x18, 0xE4, 0x04, 0x0E, 0xFA, 0x01,
    0x03, 0xFC, 0x01, 0x02, 0x04, 0xF8, 0x00, 0x00,

    // Frame 5
    0x00, 0x00, 0x60, 0x90, 0x20, 0x40, 0x80, 0x00, 0x00, 0x00, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00,
    0x00, 0x80, 0x40, 0x20, 0xC0, 0x00, 0x00, 0x00, 0xF0, 0x08, 0xE4, 0x92,
    0x97, 0xEA, 0x09, 0x01, 0x01, 0x21, 0x10, 0x00, 0x04, 0x84, 0x44, 0x40,
    0x40, 0x84, 0x04, 0x04, 0x00, 0x10, 0x21, 0x01, 0x01, 0x09, 0xEA, 0x97,
    0x92, 0xE4, 0x08, 0xF0, 0x80, 0x41, 0x32, 0xCD, 0x05, 0x84, 0x58, 0x68,
    0x48, 0x88, 0x10, 0x70, 0x95, 0x12, 0xD3, 0x33, 0x33, 0xD2, 0x15, 0x90,
    0x70, 0x10, 0x88, 0x48, 0x68, 0x58, 0x84, 0x05, 0xCD, 0x32, 0x41, 0x80,
    0x03, 0x04, 0x00, 0x00, 0x1F, 0x30, 0x60, 0x60, 0x4E, 0x50, 0x5F, 0x28,
    0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x28, 0x5F, 0x50, 0x4E,
    0x60, 0x60, 0x30, 0x1F, 0x00, 0x00, 0x04, 0x03,

    // Frame 6
    0x3C, 0x66, 0xC2, 0x83, 0x21, 0x23, 0xC2, 0x04, 0x2C, 0xC8, 0x18, 0x38,
    0x78, 0x50, 0xF0, 0xA0, 0x20, 0x60, 0xC0, 0x80, 0x00, 0x70, 0x88, 0x18,
    0x24, 0x24, 0x24, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x6F,
    0xC6, 0x18, 0x70, 0xC3, 0x06, 0x08, 0x13, 0x06, 0x04, 0x8C, 0x40, 0x20,
    0xA1, 0x21, 0x42, 0x87, 0x1F, 0x0C, 0x19, 0xF1, 0x82, 0x06, 0x04, 0x18,
    0x70, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC1, 0x43, 0x46, 0x48,
    0x8B, 0x96, 0x9C, 0x30, 0x20, 0x63, 0x44, 0xC9, 0x8B, 0x89, 0xF4, 0xEF,
    0x5A, 0xE2, 0x9E, 0x66, 0x8F, 0x38, 0x70, 0x90, 0x10, 0x1F, 0x00, 0x00,
    0x00, 0x70, 0x88, 0x8F, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    0x01, 0x02, 0x04, 0x0C, 0x18, 0x11, 0x11, 0x11, 0x16, 0x1D, 0x03, 0x0E,
    0x39, 0x23, 0x06, 0x04, 0x0D, 0x09, 0x00, 0x00,

    // Frame 7
    0x00, 0x00, 0x00, 0xC0, 0x30, 0x08, 0x04, 0x84, 0xE2, 0x32, 0x0A, 0x84,
    0x7C, 0x00, 0x00, 0x00, 0x00, 0x80, 0xE0, 0x30, 0x88, 0xC4, 0x08, 0x10,
    0x10, 0x08, 0x10, 0x10, 0x78, 0x98, 0x04, 0x00, 0x00, 0x00, 0x00, 0x01,
    0xE6, 0x34, 0x18, 0x08, 0x0C, 0x04, 0x02, 0xC1, 0x60, 0x18, 0xCC, 0x26,
    0x13, 0x0D, 0x04, 0x03, 0x01, 0x00, 0x80, 0xC0, 0x40, 0xA0, 0xB0, 0x58,
    0x23, 0x14, 0x08, 0x00, 0x00, 0x00, 0xC0, 0x20, 0x2F, 0xB0, 0x70, 0x88,
    0x04, 0x72, 0xDB, 0x89, 0xD9, 0x73, 0x04, 0x88, 0x78, 0x24, 0x34, 0x16,
    0x0A, 0x0D, 0x85, 0xC6, 0x63, 0xB1, 0x58, 0x4C, 0x24, 0xD8, 0x60, 0x00,
    0x00, 0x21, 0x21, 0x20, 0x18, 0x47, 0x40, 0x7E, 0x83, 0xFA, 0xC4, 0x04,
    0x7C, 0xCE, 0x89, 0x18, 0x20, 0x20, 0x20, 0x20, 0x30, 0x1C, 0x07, 0x04,
    0x04, 0x02, 0x02, 0x03, 0x01, 0x01, 0x00, 0x00,

    // Frame 8
    0x86, 0xEE, 0x1C, 0x7C, 0xF8, 0xF4, 0xA2, 0x42, 0x42, 0x42, 0xC2, 0x82,
    0x82, 0x44, 0x68, 0x32, 0x3C, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x30, 0x68,
    0xC4, 0x04, 0xFC, 0x04, 0x0C, 0xF8, 0x00, 0x00, 0x3F, 0x41, 0x80, 0x80,
    0xC0, 0x63, 0x3E, 0x01, 0x02, 0x24, 0x08, 0x10, 0x24, 0x40, 0x80, 0x00,
    0x00, 0x30, 0x83, 0x8C, 0xB0, 0xC0, 0x00, 0x00, 0x03, 0x8C, 0x98, 0x34,
    0x23, 0xC1, 0x00, 0x00, 0x01, 0x02, 0x07, 0x0D, 0x08, 0x10, 0x10, 0x10,
    0x20, 0x22, 0x22, 0x42, 0x78, 0xC4, 0xA4, 0xB5, 0x84, 0x7F, 0xD0, 0x54,
    0x36, 0x30, 0x2F, 0x58, 0xD4, 0x23, 0x60, 0x60, 0x90, 0x0F, 0x00, 0x00,
    0x00, 0x00, 0x3E, 0x69, 0x49, 0x4B, 0x42, 0x46, 0x7C, 0x30, 0x26, 0x28,
    0x2C, 0x22, 0x3F, 0x1B, 0x01, 0x3F, 0xE6, 0xCC, 0x08, 0x13, 0x2E, 0x3C,
    0x38, 0x1F, 0x00, 0x00, 0x07, 0x18, 0x18, 0x00,

    // Frame 9
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x80, 0x80, 0x80, 0x40, 0x40, 0x20, 0x20, 0x10, 0xB0, 0x5C,
    0xF2, 0x12, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0xF0,
    0x90, 0x68, 0xA8, 0x64, 0xA4, 0x62, 0xA2, 0x62, 0xA1, 0x41, 0xC0, 0x80,
    0x00, 0x00, 0x80, 0x40, 0xA8, 0x55, 0xFA, 0x07, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x0C, 0x18,
    0xF0, 0x60, 0xB0, 0xBC, 0xFE, 0xBF, 0xBF, 0x63, 0xCF, 0x0E, 0x9E, 0x35,
    0xEA, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x0F, 0x1B, 0x3B,
    0x3D, 0x3B, 0x1B, 0x0F, 0x07, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // Frame 10
    0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x60, 0x60, 0xE0, 0xC0,
    0xC0, 0xC0, 0x00, 0x00, 0x80, 0x00, 0xA0, 0x00, 0x08, 0x00, 0x00, 0xF0,
    0xFC, 0x3C, 0x1C, 0x38, 0x70, 0x60, 0xE0, 0xC0, 0x00, 0x04, 0x00, 0x00,
    0x08, 0x50, 0xA0, 0x50, 0x80, 0x40, 0xFC, 0xDF, 0xA7, 0x90, 0x28, 0x35,
    0xB8, 0x1D, 0x1A, 0x9C, 0x3E, 0xFD, 0xEB, 0x93, 0x2B, 0x50, 0xAA, 0x44,
    0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x78, 0x1C, 0x0E, 0x06, 0x04,
    0x9E, 0x5D, 0xBA, 0x5D, 0x2D, 0x4F, 0xCF, 0xEC, 0xFC, 0xDA, 0xDA, 0xC0,
    0xE0, 0xFC, 0x3F, 0x47, 0xAE, 0x16, 0x26, 0x56, 0x86, 0x1E, 0xFE, 0xF0,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x18, 0x18, 0x1D,
    0x1F, 0x0F, 0x03, 0x05, 0x0A, 0x01, 0x0B, 0x00, 0x01, 0x1F, 0x0B, 0x0D,
    0x0E, 0x0E, 0x1E, 0x18, 0x00, 0x01, 0x08, 0x00,

    // Frame 11
    0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xE0, 0xD0, 0x70, 0x70, 0x70, 0x70,
    0x70, 0x50, 0x60, 0x70, 0x50, 0x10, 0x50, 0x50, 0x70, 0x40, 0x70, 0x70,
    0xF0, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xFE, 0x1E, 0xFF, 0x00, 0x9F, 0xE3, 0xFF, 0x04, 0xF7, 0xD7, 0x76,
    0x75, 0xD5, 0xF3, 0x06, 0xBD, 0x81, 0xFF, 0x00, 0x7D, 0x7D, 0xFF, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x71, 0x7E, 0xFF,
    0x00, 0x5F, 0x5F, 0x5F, 0x50, 0x46, 0x76, 0x71, 0x57, 0x54, 0x55, 0x10,
    0x7F, 0x03, 0x7F, 0x00, 0xBF, 0x30, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x04, 0x07, 0x07, 0x05, 0x05, 0x04,
    0x05, 0x05, 0x05, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x05, 0x06, 0x07,
    0x07, 0x04, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,

    // Frame 12
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
    0x80, 0x80, 0x00, 0x00, 0x00, 0x80, 0x80, 0x40, 0x40, 0x40, 0x80, 0x80,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xFC, 0xAE, 0x57, 0xAB, 0x57, 0xAF, 0x1D, 0x2B, 0x3D, 0xAF, 0x77,
    0xAB, 0x77, 0x3F, 0x1D, 0x2F, 0x5D, 0xFF, 0x57, 0xAB, 0x7E, 0xFC, 0x80,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0xFF, 0xAA, 0xF5,
    0xBA, 0x7F, 0xEB, 0xDC, 0x8A, 0x14, 0x8A, 0x95, 0x8A, 0x15, 0x9A, 0x16,
    0xBA, 0x57, 0xBE, 0x7D, 0xFA, 0x55, 0xFF, 0xF3, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x40, 0x43, 0x46, 0x6D, 0x2B, 0x2F, 0xEA, 0x7F,
    0x2B, 0x2F, 0x3B, 0x1F, 0x3E, 0x6F, 0x5E, 0x3D, 0x2E, 0x2D, 0x2A, 0x5F,
    0x6B, 0x65, 0x43, 0x40, 0x40, 0x00, 0x00, 0x00,

    // Frame 13
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x60,
    0x20, 0x10, 0x10, 0x20, 0x20, 0x20, 0x40, 0x80, 0x80, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x80, 0xFE, 0xC3, 0x00, 0x00, 0x0C, 0x04, 0x0C,
    0x0C, 0x04, 0x84, 0x84, 0xC5, 0x4B, 0x7C, 0xC0, 0x80, 0x80, 0x80, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x0C, 0x03, 0x61, 0x20,
    0x30, 0x18, 0x08, 0x0F, 0x04, 0x04, 0x84, 0x84, 0xCC, 0x98, 0x9F, 0x90,
    0x00, 0x00, 0x20, 0x10, 0x10, 0x20, 0x21, 0x03, 0x0E, 0xF8, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x07, 0x08, 0x08, 0x0C, 0x0C, 0x0C, 0x08, 0x08, 0x08,
    0x09, 0x09, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0C,
    0x04, 0x04, 0x06, 0x02, 0x01, 0x00, 0x00, 0x00,

    // Frame 14
    0x00, 0x00, 0x60, 0x90, 0xB0, 0x90, 0x60, 0x80, 0x40, 0x20, 0x20, 0x20,
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x20, 0x20, 0x20, 0x40,
    0x80, 0x60, 0x90, 0xB0, 0x90, 0x60, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x38,
    0x04, 0x02, 0x01, 0x00, 0x8C, 0x84, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x01, 0x00, 0x00, 0x80, 0x80, 0xC0, 0x42, 0x00, 0x01, 0x02, 0x04,
    0xB8, 0x40, 0x80, 0x00, 0x00, 0x00, 0x1F, 0xE0, 0x00, 0x40, 0x31, 0x11,
    0x10, 0x01, 0x81, 0x01, 0x00, 0x00, 0x10, 0x00, 0x18, 0x10, 0x11, 0x09,
    0x01, 0x00, 0xA0, 0x40, 0xA8, 0x54, 0xAA, 0x55, 0xAA, 0x15, 0x02, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x10, 0x28, 0x14,
    0x28, 0x54, 0x28, 0x54, 0x28, 0x54, 0x28, 0x54, 0x2A, 0x15, 0x2A, 0x15,
    0x0A, 0x05, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00,

    // Frame 15
    0x00, 0xF0, 0x90, 0x28, 0x48, 0x28, 0x90, 0xF0, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
    0x3D, 0x49, 0x92, 0xD2, 0xD3, 0x8A, 0x44, 0xB8, 0xC0, 0x80, 0x80, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0,
    0x50, 0xB1, 0x9F, 0xF2, 0x54, 0x34, 0x12, 0x0F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x06, 0x3A, 0xC7,
    0xA2, 0x92, 0xCE, 0x38, 0x48, 0x84, 0xC4, 0xA4, 0xA4, 0x97, 0x48, 0x38,
    0x0C, 0x0A, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,

    // Frame 16
    0x00, 0x00, 0x80, 0xE0, 0x30, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x20,
    0xE0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x09,
    0x13, 0x20, 0x20, 0x23, 0x31, 0x19, 0x7C, 0xE6, 0x80, 0x03, 0x86, 0x6C,
    0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0xB8, 0x18, 0x30,
    0x60, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xC0, 0x40,
    0x20, 0x30, 0xF8, 0xCF, 0xC0, 0xC9, 0x68, 0x2E, 0x18, 0x0F, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xC0, 0x3F, 0x80, 0x80, 0x08, 0x39, 0xFF, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x0F, 0x39, 0x6C, 0x50, 0x50, 0xD8, 0x8C, 0x8F,
    0x99, 0xD0, 0xD0, 0xB0, 0xB0, 0x90, 0xD0, 0xB0, 0xB0, 0xD0, 0xD8, 0xE6,
    0x63, 0x40, 0x60, 0x20, 0x3F, 0x0E, 0x03, 0x00,

    // Frame 17
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x80, 0xF8, 0x0C, 0x06, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x38,
    0xCC, 0x0C, 0x18, 0x20, 0x60, 0x80, 0x80, 0x80, 0xC0, 0x60, 0x20, 0x10,
    0x08, 0x8C, 0x44, 0x76, 0x1A, 0x0A, 0x03, 0x1F, 0xF9, 0x80, 0x60, 0x20,
    0x30, 0x10, 0x90, 0xF0, 0x00, 0x00, 0x0F, 0x10, 0xE1, 0x87, 0x0C, 0x00,
    0x00, 0x60, 0x91, 0x93, 0x92, 0x62, 0x02, 0x00, 0x00, 0x60, 0x90, 0x94,
    0x94, 0x66, 0x06, 0x03, 0x01, 0x00, 0x10, 0x08, 0xC6, 0x61, 0x1C, 0x07,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x04, 0x08, 0x08, 0x08, 0x08, 0x12,
    0x10, 0x14, 0x14, 0x14, 0x14, 0x14, 0x10, 0x12, 0x08, 0x08, 0x0C, 0x0C,
    0x04, 0x06, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00,

    // Frame 18
    0x00, 0x00, 0x00, 0x00, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xFC, 0xDC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0xE0, 0x10, 0x0F,
    0x00, 0x01, 0x0E, 0x38, 0xC0, 0x00, 0x00, 0x00, 0xC0, 0x30, 0x0F, 0x01,
    0x07, 0x3C, 0xE0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x3E, 0x07,
    0x03, 0x0F, 0xF0, 0x00, 0x00, 0x07, 0xF8, 0x80, 0x00, 0xC0, 0x22, 0xA4,
    0x29, 0xDE, 0x34, 0xC3, 0x01, 0x00, 0xCC, 0x62, 0x62, 0xCC, 0x00, 0x01,
    0xE6, 0x34, 0xC8, 0x24, 0xAB, 0x24, 0xC2, 0x00, 0x80, 0x7C, 0x03, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x06, 0x0D, 0x1A, 0x12, 0x23, 0x21, 0x20, 0x31,
    0x31, 0x48, 0x58, 0x68, 0x68, 0x58, 0x48, 0x31, 0x31, 0x20, 0x21, 0x13,
    0x12, 0x12, 0x19, 0x0C, 0x03, 0x00, 0x00, 0x00,

    // Frame 19
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x40, 0x40, 0x20, 0x10,
    0x08, 0xF8, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x80, 0x40, 0x20, 0x20, 0x10, 0x08, 0x08, 0x04,
    0x02, 0x01, 0x01, 0x00, 0x00, 0x00, 0x80, 0xC0, 0x3F, 0xE0, 0x80, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0x93,
    0x20, 0x26, 0x40, 0x40, 0x40, 0x66, 0x60, 0xD0, 0x10, 0x18, 0x24, 0x24,
    0x66, 0xC7, 0x89, 0x49, 0xBE, 0x6C, 0xE3, 0xDD, 0x2C, 0x16, 0x0A, 0x04,
    0x00, 0x20, 0x50, 0x28, 0x28, 0x17, 0x0D, 0x08, 0x09, 0x06, 0x02, 0x0C,
    0x1A, 0x1C, 0x1A, 0x1D, 0x1A, 0x1C, 0x1A, 0x1C, 0x0A, 0x05, 0x06, 0x05,
    0x02, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,

    // Frame 20
    0x00, 0x00, 0x70, 0x88, 0xE8, 0xE8, 0x88, 0x70, 0x40, 0x20, 0x20, 0x20,
    0x90, 0x90, 0x90, 0x90, 0x90, 0x10, 0x20, 0x20, 0x40, 0x70, 0x88, 0xE8,
    0xE8, 0x88, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x60, 0x18,
    0x86, 0x61, 0x18, 0x04, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01, 0x01, 0x01, 0x02, 0x04, 0x18, 0x61, 0x86, 0x18, 0x60, 0x80,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x78, 0x80, 0x3F, 0xC0, 0x80, 0xC0,
    0xA0, 0x60, 0xB0, 0x50, 0xB0, 0x50, 0xB0, 0x50, 0xB0, 0x50, 0xB0, 0x50,
    0xB0, 0x60, 0xC0, 0xC0, 0x3F, 0x80, 0x78, 0x07, 0x00, 0x00, 0x00, 0x00,
    0x78, 0xBC, 0x7C, 0xBD, 0x7E, 0x18, 0x23, 0x45, 0x4A, 0x55, 0x8A, 0x95,
    0xAA, 0xB5, 0xAA, 0xB5, 0xAA, 0x95, 0x9A, 0x55, 0x4A, 0x45, 0x33, 0x0C,
    0x7B, 0xBC, 0x7C, 0xBC, 0x78, 0x00, 0x00, 0x00,

    // Frame 21
    0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x20, 0x60, 0x20, 0xC0, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x20, 0x60,
    0x20, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x20, 0x10,
    0x10, 0x11, 0x0B, 0x0A, 0x0B, 0x05, 0x04, 0x24, 0x14, 0xF4, 0x04, 0x04,
    0x0C, 0x74, 0x84, 0x04, 0x04, 0x05, 0x0B, 0x0A, 0x0B, 0x11, 0x10, 0x10,
    0x20, 0xC0, 0x00, 0x00, 0x00, 0x03, 0x06, 0x05, 0x0A, 0x14, 0x0A, 0x14,
    0x08, 0x30, 0x28, 0x12, 0x2A, 0x13, 0x2A, 0x10, 0x28, 0x16, 0x29, 0x10,
    0x28, 0x34, 0x08, 0x14, 0x1A, 0x04, 0x0A, 0x0D, 0x02, 0x03, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // Frame 22
    0x00, 0x70, 0xA8, 0xE8, 0x98, 0x70, 0xC0, 0x30, 0x08, 0x08, 0x08, 0x08,
    0x30, 0xC0, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x30, 0x08, 0x08, 0x08, 0x08,
    0x30, 0xC0, 0x70, 0xA8, 0xE8, 0x98, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xFC, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFC, 0x00,
    0x00, 0xFC, 0x03, 0x00, 0x88, 0x44, 0x24, 0x18, 0x00, 0x03, 0xFC, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xA8, 0x50,
    0xB0, 0x60, 0xA0, 0x50, 0xB0, 0x48, 0xFF, 0x00, 0x00, 0xFF, 0xA8, 0x50,
    0xA1, 0x61, 0xA1, 0x51, 0xB0, 0x48, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x15, 0x2A, 0x55, 0x2A, 0x55,
    0x2A, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x35, 0x6A, 0x55, 0x6A, 0x55,
    0x2A, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // Frame 23
    0x00, 0x78, 0x84, 0x02, 0x3A, 0x39, 0x01, 0x01, 0x01, 0x01, 0x39, 0x3A,
    0x02, 0x84, 0x7C, 0x18, 0x60, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
    0x03, 0x06, 0x06, 0x0A, 0x0A, 0x12, 0x12, 0x21, 0x21, 0xA0, 0xC0, 0x40,
    0x80, 0x81, 0x06, 0x08, 0x30, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x40, 0x40, 0x20, 0x10,
    0x88, 0x84, 0x82, 0x42, 0xC1, 0xA1, 0xA0, 0xB0, 0x10, 0x10, 0x11, 0x09,
    0x0A, 0x0C, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x02, 0x02, 0x03, 0x03, 0x05, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x0C, 0x08, 0x08, 0x09, 0x12, 0x12, 0x16, 0x14, 0x14, 0x08, 0x08,
    0x18, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,

    // Frame 24
    0x00, 0x00, 0x18, 0xF0, 0x20, 0x40, 0x80, 0x00, 0x00, 0x70, 0x0E, 0x38,
    0x00, 0x00, 0x00, 0xE0, 0x1C, 0xE0, 0x00, 0x00, 0x20, 0x10, 0x78, 0x0C,
    0x00, 0x80, 0x40, 0xB0, 0x68, 0x18, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00,
    0x0F, 0x70, 0x80, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x60, 0x1E, 0x01,
    0x00, 0x01, 0x1E, 0x60, 0x40, 0x30, 0x08, 0x04, 0xC3, 0x38, 0x06, 0x01,
    0x20, 0x60, 0xC0, 0x00, 0x0C, 0x1E, 0x11, 0x00, 0x00, 0x00, 0xF9, 0x07,
    0x00, 0x00, 0x83, 0x42, 0x20, 0x20, 0xC4, 0x06, 0x06, 0x04, 0xE0, 0x22,
    0x43, 0x80, 0x0C, 0x33, 0x40, 0x80, 0x04, 0x06, 0x02, 0x01, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x70, 0x2F, 0x10, 0x08, 0x06, 0x01, 0x38, 0x60,
    0x18, 0x00, 0x03, 0x1C, 0x60, 0x7C, 0x03, 0x08, 0x30, 0x60, 0x19, 0x02,
    0x02, 0x04, 0x0B, 0x14, 0x18, 0x20, 0x00, 0x00,

    // Frame 25
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x80, 0x80, 0x80, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x60, 0x10,
    0x08, 0x04, 0x02, 0x01, 0x19, 0x18, 0x00, 0x00, 0xC0, 0xC0, 0xC0, 0xC0,
    0xC0, 0xC0, 0xC0, 0xC0, 0x18, 0x18, 0x00, 0x01, 0x01, 0x02, 0x04, 0x08,
    0x10, 0x60, 0x80, 0x00, 0x00, 0x7F, 0x80, 0x9E, 0x7E, 0xB0, 0x40, 0x80,
    0x7C, 0xFC, 0xF0, 0x80, 0xFC, 0xFC, 0xC0, 0x00, 0x00, 0xF0, 0xF8, 0x7C,
    0x0C, 0xC0, 0xF8, 0xFC, 0x0C, 0x80, 0x7E, 0xBE, 0x40, 0x80, 0x7F, 0x00,
    0x00, 0x00, 0x03, 0x0A, 0x15, 0x2A, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA,
    0xD5, 0xAA, 0x55, 0xAB, 0x55, 0xAB, 0xD5, 0xAA, 0xD5, 0xAA, 0x55, 0xAA,
    0x55, 0xAA, 0x55, 0x2A, 0x0D, 0x02, 0x00, 0x00,

    // Frame 26
    0x00, 0x00, 0x00, 0x00, 0x20, 0x40, 0x40, 0x88, 0x08, 0x10, 0xF0, 0x10,
    0xE8, 0x1C, 0xF6, 0x0E, 0x0E, 0xF6, 0x1C, 0xE8, 0x14, 0xE2, 0x03, 0x20,
    0x11, 0x10, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x82, 0x0C, 0x10, 0x20, 0xF1, 0x0F, 0x00, 0xF8, 0x07, 0xC0, 0xE1, 0x00,
    0x00, 0xE1, 0xC0, 0x07, 0xF8, 0x00, 0x0F, 0xF4, 0x04, 0x82, 0x42, 0x40,
    0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x20, 0x41, 0x82, 0x04,
    0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x11, 0x60, 0x60, 0x11, 0x00, 0x00,
    0xFF, 0x00, 0x00, 0xFF, 0x20, 0x10, 0x10, 0x08, 0x08, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x08, 0x08, 0x10, 0x10, 0x11, 0x21, 0x3E, 0xE0, 0x1F,
    0xE0, 0x00, 0xC0, 0x00, 0x00, 0xC0, 0x00, 0xE0, 0x1F, 0xE0, 0x5E, 0x43,
    0x42, 0x21, 0x21, 0x20, 0x20, 0x00, 0x00, 0x00,

    // Frame 27
    0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x40, 0x40, 0x40, 0x40, 0xC0,
    0x80, 0x60, 0x20, 0x10, 0x10, 0x90, 0x90, 0x90, 0x20, 0x20, 0x40, 0xC0,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x7F, 0xC1, 0x00, 0xFC, 0x86, 0x02, 0x06, 0x7F, 0x80, 0x00, 0x0E, 0x03,
    0x01, 0x00, 0x00, 0x01, 0x03, 0x66, 0x3C, 0x00, 0x01, 0x8E, 0x70, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xEC,
    0x30, 0x48, 0x06, 0x43, 0x07, 0x08, 0x88, 0x58, 0x10, 0x10, 0xF0, 0x30,
    0xF0, 0xFC, 0x9C, 0x86, 0xA7, 0xB1, 0xD9, 0x42, 0x3E, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x06, 0x04, 0x04,
    0x04, 0x04, 0x06, 0x02, 0x02, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // Frame 28
    0xC0, 0x70, 0x18, 0x08, 0x18, 0x20, 0x60, 0xC0, 0x00, 0x00, 0xE0, 0x30,
    0x10, 0x90, 0x30, 0x60, 0x80, 0x00, 0x00, 0x00, 0x80, 0x40, 0x60, 0x10,
    0x18, 0x0C, 0x0C, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x78, 0xC0, 0x00,
    0x00, 0x1F, 0x61, 0x80, 0x01, 0x02, 0x05, 0x0E, 0xB8, 0x71, 0x30, 0x8C,
    0xC7, 0x62, 0x31, 0x19, 0x88, 0xE4, 0x24, 0x30, 0x38, 0x64, 0x43, 0xC0,
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x83, 0x46, 0x24, 0x2C, 0x18,
    0x04, 0x04, 0x12, 0x19, 0x88, 0xA4, 0xC6, 0x33, 0x18, 0x0C, 0x84, 0x0F,
    0x14, 0x3C, 0x28, 0x48, 0x8A, 0x8B, 0x08, 0x08, 0x0F, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x07, 0x08, 0x09, 0x08, 0x08, 0x09, 0x04, 0x02, 0x02,
    0x01, 0x00, 0x00, 0x03, 0x02, 0x04, 0x19, 0x13, 0x36, 0x24, 0x44, 0x44,
    0x40, 0xC1, 0x82, 0x7C, 0x00, 0x00, 0x00, 0x00,

    // Frame 29
    0x00, 0xE0, 0x9C, 0x06, 0x02, 0x81, 0x7D, 0x65, 0x45, 0x45, 0x45, 0xE7,
    0x20, 0x20, 0x90, 0xB0, 0x60, 0x47, 0x45, 0x45, 0x4D, 0xAD, 0x29, 0x39,
    0x81, 0x83, 0x47, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02,
    0x0F, 0x10, 0xE0, 0x2E, 0x2E, 0x20, 0x20, 0x30, 0x1F, 0x00, 0x3F, 0x70,
    0x20, 0x2E, 0xAE, 0xA0, 0x20, 0x10, 0x0F, 0x1B, 0x7E, 0xE8, 0xF8, 0xF0,
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x4E, 0xC3, 0xC0, 0x86,
    0x82, 0x81, 0xC1, 0xC1, 0xC1, 0x42, 0xEE, 0x30, 0x38, 0x0C, 0x87, 0x80,
    0x60, 0x20, 0x9C, 0x86, 0xE0, 0xFD, 0xFF, 0x3F, 0xBF, 0x04, 0xF8, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x03, 0x3F, 0x2F, 0x04, 0x04, 0x04, 0x04, 0x2F,
    0x3F, 0x02, 0xE3, 0xE1, 0xF1, 0xDF, 0xB9, 0xB4, 0xB4, 0xB6, 0xBA, 0xBB,
    0xDD, 0xCD, 0xC6, 0xF2, 0xF9, 0xFC, 0xE7, 0xC0,

    // Frame 30
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x20, 0x50, 0x28, 0x14,
    0x0A, 0x04, 0x03, 0x05, 0x03, 0x05, 0x0B, 0x14, 0x0A, 0x14, 0x2C, 0x58,
    0x20, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x0F, 0x30, 0xC0, 0x1C, 0xA2, 0x2E, 0x2E, 0x22, 0x9C,
    0x00, 0x9C, 0x22, 0x2E, 0x2E, 0xA2, 0x1C, 0xC0, 0x30, 0x0F, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x07, 0x19, 0x60, 0x28, 0x70, 0x33, 0x70, 0x33, 0x70, 0x28,
    0x60, 0x19, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
    0x06, 0x07, 0x06, 0x07, 0x06, 0x07, 0x06, 0x07, 0x06, 0x03, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // Frame 31
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xF0,
    0xF0, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x0F, 0x0F, 0x1F, 0x1F, 0xBF, 0xBF, 0xFF, 0xF8, 0xF0, 0x00,
    0x00, 0x00, 0x00, 0x80, 0xC0, 0xE0, 0xF1, 0xFF, 0x7F, 0x3F, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x0E,
    0x0E, 0x0F, 0x0F, 0x07, 0x07, 0x07, 0x01, 0x00, 0x00, 0x0E, 0x0F, 0x0F,
    0x07, 0x07, 0x0F, 0x1F, 0x1E, 0x1C, 0x3C, 0x3C, 0x3C, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
