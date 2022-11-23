const uint8_t bootblocks[] PROGMEM = {  
  0xea, 0xa2, 0x70, 0xa9, 0x20, 0x85, 0xf0, 0xa9, 0x60, 0x85, 0xf3, 0x86,
  0x43, 0x8a, 0x4a, 0x4a, 0x4a, 0x4a, 0x29, 0x07, 0x09, 0xc0, 0x85, 0xf2,
  0xa0, 0x00, 0x84, 0xf1, 0x88, 0xb1, 0xf1, 0x85, 0xf1, 0x20, 0x58, 0xfc,
  0x20, 0x9e, 0x09, 0xa9, 0x00, 0x85, 0x25, 0x20, 0x22, 0xfc, 0xa5, 0x25,
  0x85, 0xf5, 0x85, 0xf6, 0x20, 0x8c, 0x09, 0xa9, 0x00, 0x85, 0x24, 0xa5,
  0x25, 0x20, 0xe3, 0xfd, 0xe6, 0x24, 0x20, 0x76, 0x09, 0x20, 0x02, 0x09,
  0xa9, 0x14, 0x85, 0x24, 0xa5, 0x25, 0x20, 0xe3, 0xfd, 0xe6, 0x24, 0xa5,
  0x43, 0x09, 0x80, 0x85, 0x43, 0x20, 0x76, 0x09, 0x20, 0x02, 0x09, 0xa5,
  0x43, 0x29, 0x7f, 0x85, 0x43, 0xe6, 0x25, 0xa5, 0x25, 0xc9, 0x10, 0x90,
  0xbe, 0xa9, 0x00, 0x85, 0x24, 0xa9, 0x12, 0x85, 0x25, 0x20, 0x22, 0xfc,
  0xa2, 0x14, 0x20, 0x62, 0x09, 0x20, 0x5d, 0x09, 0xa9, 0x0a, 0x85, 0x24,
  0xa5, 0xf7, 0x20, 0xf5, 0x08, 0xa9, 0x14, 0x85, 0x24, 0x20, 0x58, 0x09,
  0xa9, 0x1e, 0x85, 0x24, 0xa5, 0xf8, 0x20, 0xf5, 0x08, 0xa9, 0x0a, 0x85,
  0x24, 0x20, 0xc7, 0x08, 0x85, 0xf5, 0x20, 0xf5, 0x08, 0xa9, 0x1e, 0x85,
  0x24, 0x20, 0xc7, 0x08, 0x85, 0xf6, 0x20, 0xf5, 0x08, 0x20, 0x88, 0x09,
  0x4c, 0xb3, 0x09, 0xa5, 0xf7, 0x85, 0xf5, 0xa5, 0xf8, 0x85, 0xf6, 0x20,
  0x8c, 0x09, 0x68, 0x68, 0x4c, 0xb3, 0x09, 0x20, 0x0c, 0xfd, 0xc9, 0x9b,
  0xf0, 0xe9, 0xc9, 0xa1, 0xf0, 0x20, 0xc9, 0xe1, 0x90, 0x03, 0x38, 0xe9,
  0x20, 0xc9, 0xc1, 0x90, 0x04, 0xc9, 0xc7, 0x90, 0x0b, 0xc9, 0xb0, 0x90,
  0xe2, 0xc9, 0xba, 0xb0, 0xde, 0x29, 0x0f, 0x60, 0x38, 0xe9, 0x07, 0x29,
  0x0f, 0x60, 0xa9, 0xff, 0x60, 0xc9, 0xff, 0xf0, 0x03, 0x4c, 0xe3, 0xfd,
  0xa9, 0xa1, 0x4c, 0xed, 0xfd, 0x60, 0xa2, 0x00, 0xb0, 0x23, 0xad, 0x05,
  0x10, 0x30, 0x1e, 0xad, 0x04, 0x10, 0x29, 0xf0, 0xc9, 0xf0, 0xd0, 0x15,
  0xad, 0x04, 0x10, 0x29, 0x0f, 0x85, 0xf9, 0xbd, 0x05, 0x10, 0x09, 0x80,
  0x20, 0xed, 0xfd, 0xe8, 0xe4, 0xf9, 0xd0, 0xf3, 0x60, 0x4c, 0x62, 0x09,
  0xbc, 0xce, 0xcf, 0xa0, 0xd6, 0xcf, 0xcc, 0xd5, 0xcd, 0xc5, 0xbe, 0x00,
  0xc3, 0xc1, 0xd2, 0xc4, 0xa0, 0xb0, 0xba, 0x00, 0xc4, 0xc1, 0xce, 0xa0,
  0xdd, 0xdb, 0xa0, 0xd6, 0xcf, 0xcc, 0xd5, 0xcd, 0xc5, 0xa0, 0xd3, 0xc5,
  0xcc, 0xc5, 0xc3, 0xd4, 0xcf, 0xd2, 0x8d, 0x00, 0xa9, 0xb1, 0x8d, 0x3d,
  0x09, 0xa2, 0x0c, 0x4c, 0x62, 0x09, 0xbd, 0x2c, 0x09, 0xf0, 0x0e, 0x20,
  0xed, 0xfd, 0xe8, 0xd0, 0xf5, 0xa9, 0x00, 0x85, 0x44, 0xa9, 0x10, 0x85,
  0x45, 0x60, 0xa9, 0x01, 0x85, 0x42, 0x20, 0x6d, 0x09, 0xa9, 0x02, 0x85,
  0x46, 0xa9, 0x00, 0x85, 0x47, 0x4c, 0xf0, 0x00, 0xa9, 0x07, 0xd0, 0x02,
  0xa9, 0x06, 0x85, 0x42, 0x20, 0x6d, 0x09, 0xa5, 0xf5, 0x85, 0x46, 0xa5,
  0xf6, 0x85, 0x47, 0x4c, 0xf0, 0x00, 0xa9, 0x05, 0x85, 0x42, 0x20, 0x6d,
  0x09, 0x20, 0xf0, 0x00, 0xad, 0x00, 0x10, 0x85, 0xf7, 0xad, 0x01, 0x10,
  0x85, 0xf8, 0x60, 0xa9, 0x00, 0x85, 0xf1, 0x6c, 0xf1, 0x00
};  
