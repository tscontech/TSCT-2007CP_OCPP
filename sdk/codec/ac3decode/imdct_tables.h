﻿/*
 * imdct_tables.c
 * Copyright (C) 2004-2007 Kuoping Hsu <kuoping@ite.com>
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of a52dec, a free ATSC A-52 stream decoder.
 * See http://liba52.sourceforge.net/ for updates.
 *
 * a52dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * a52dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __IMDCT_TABLES_H__
#define __IMDCT_TABLES_H__

static sample_t a52_imdct_window[256] = {
  0x00023a64, 0x0003fefe, 0x00060139, 0x000851ad, 0x000afa57, 0x000e03ce,
  0x00117660, 0x00155a62, 0x0019b855, 0x001e98f2, 0x0024052f, 0x002a0643,
  0x0030a5a6, 0x0037ed12, 0x003fe67d, 0x00489c1c, 0x00521861, 0x005c65f3,
  0x00678fb3, 0x0073a0b3, 0x0080a435, 0x008ea5a9, 0x009db0a7, 0x00add0ec,
  0x00bf1257, 0x00d180e4, 0x00e528a6, 0x00fa15c8, 0x01105480, 0x0127f111,
  0x0140f7c3, 0x015b74e0, 0x017774a9, 0x0195035a, 0x01b42d1a, 0x01d4fe00,
  0x01f78203, 0x021bc4fc, 0x0241d29c, 0x0269b668, 0x02937bb1, 0x02bf2d91,
  0x02ecd6e1, 0x031c8239, 0x034e39e1, 0x038207d3, 0x03b7f5b4, 0x03f00cc7,
  0x042a55f1, 0x0466d9ab, 0x04a5a002, 0x04e6b08d, 0x052a126b, 0x056fcc3b,
  0x05b7e418, 0x06025f93, 0x064f43b0, 0x069e94dc, 0x06f056f2, 0x07448d2a,
  0x079b3a22, 0x07f45fce, 0x084fff7d, 0x08ae19d1, 0x090eaebe, 0x0971bd85,
  0x09d744b4, 0x0a3f421f, 0x0aa9b2e4, 0x0b169364, 0x0b85df46, 0x0bf79170,
  0x0c6ba40c, 0x0ce21086, 0x0d5acf88, 0x0dd5d8ff, 0x0e532419, 0x0ed2a747,
  0x0f54583e, 0x0fd82bf6, 0x105e16b1, 0x10e60bfa, 0x116ffea5, 0x11fbe0d8,
  0x1289a40a, 0x13193909, 0x13aa8ffc, 0x143d9867, 0x14d24134, 0x156878b2,
  0x16002c9e, 0x16994a27, 0x1733bdf6, 0x17cf7431, 0x186c5884, 0x190a5627,
  0x19a957e3, 0x1a49481d, 0x1aea10db, 0x1b8b9bcb, 0x1c2dd24b, 0x1cd09d75,
  0x1d73e622, 0x1e1794f3, 0x1ebb925d, 0x1f5fc6b0, 0x20041a1c, 0x20a874c1,
  0x214cbeb0, 0x21f0dffc, 0x2294c0bb, 0x23384914, 0x23db6149, 0x247df1ba,
  0x251fe2f5, 0x25c11dba, 0x26618b04, 0x27011417, 0x279fa281, 0x283d2027,
  0x28d9774e, 0x297492a1, 0x2a0e5d39, 0x2aa6c2a5, 0x2b3daef4, 0x2bd30eb8,
  0x2c66cf10, 0x2cf8ddb0, 0x2d8928e4, 0x2e179f97, 0x2ea4315d, 0x2f2ece73,
  0x2fb767c7, 0x303deefd, 0x30c25674, 0x3144914a, 0x31c4935c, 0x32425150,
  0x32bdc094, 0x3336d760, 0x33ad8cbb, 0x3421d87c, 0x3493b34a, 0x3503169c,
  0x356ffcbf, 0x35da60cf, 0x36423ebd, 0x36a79349, 0x370a5c06, 0x376a9754,
  0x37c84460, 0x38236321, 0x387bf457, 0x38d1f985, 0x392574f0, 0x39766999,
  0x39c4db3a, 0x3a10ce42, 0x3a5a47cd, 0x3aa14da2, 0x3ae5e62b, 0x3b281872,
  0x3b67ec16, 0x3ba56948, 0x3be098c7, 0x3c1983d0, 0x3c503421, 0x3c84b3ec,
  0x3cb70dcf, 0x3ce74ccf, 0x3d157c51, 0x3d41a80e, 0x3d6bdc0e, 0x3d9424a1,
  0x3dba8e54, 0x3ddf25eb, 0x3e01f858, 0x3e2312b6, 0x3e42823d, 0x3e60543c,
  0x3e7c9613, 0x3e975529, 0x3eb09ee5, 0x3ec880ab, 0x3edf07cd, 0x3ef4418e,
  0x3f083b14, 0x3f1b0163, 0x3f2ca15d, 0x3f3d27b4, 0x3f4ca0eb, 0x3f5b194d,
  0x3f689ceb, 0x3f753797, 0x3f80f4df, 0x3f8be00b, 0x3f960417, 0x3f9f6bb3,
  0x3fa82140, 0x3fb02ecc, 0x3fb79e10, 0x3fbe7872, 0x3fc4c6ff, 0x3fca926e,
  0x3fcfe31e, 0x3fd4c114, 0x3fd933fe, 0x3fdd4332, 0x3fe0f5af, 0x3fe4521c,
  0x3fe75ecc, 0x3fea21be, 0x3feca09d, 0x3feee0c4, 0x3ff0e741, 0x3ff2b8d3,
  0x3ff459ef, 0x3ff5cec2, 0x3ff71b33, 0x3ff842e7, 0x3ff94942, 0x3ffa3169,
  0x3ffafe46, 0x3ffbb28c, 0x3ffc50b8, 0x3ffcdb13, 0x3ffd53b6, 0x3ffdbc8e,
  0x3ffe175b, 0x3ffe65b7, 0x3ffea914, 0x3ffee2c4, 0x3fff13f6, 0x3fff3dba,
  0x3fff6106, 0x3fff7eb6, 0x3fff978c, 0x3fffac35, 0x3fffbd4c, 0x3fffcb58,
  0x3fffd6cf, 0x3fffe019, 0x3fffe790, 0x3fffed82, 0x3ffff233, 0x3ffff5dd,
  0x3ffff8af, 0x3ffffad4, 0x3ffffc70, 0x3ffffd9e, 0x3ffffe77, 0x3fffff0e,
  0x3fffff75, 0x3fffffb7, 0x3fffffe0, 0x3ffffff6
};

static sample_t roots16[3] = {
  0x3b20d79e, 0x2d413ccc, 0x187de2a6
};

static sample_t roots32[7] = {
  0x3ec52f9f, 0x3b20d79e, 0x3536cc52, 0x2d413ccc, 0x238e7673, 0x187de2a6,
  0x0c7c5c1e
};

static sample_t roots64[15] = {
  0x3fb11b47, 0x3ec52f9f, 0x3d3e82ad, 0x3b20d79e, 0x387165e3, 0x3536cc52,
  0x317900d6, 0x2d413ccc, 0x2899e64a, 0x238e7673, 0x1e2b5d38, 0x187de2a6,
  0x1294062e, 0x0c7c5c1e, 0x0645e9af
};

static sample_t roots128[31] = {
  0x3fec43c6, 0x3fb11b47, 0x3f4eaafe, 0x3ec52f9f, 0x3e14fdf7, 0x3d3e82ad,
  0x3c424209, 0x3b20d79e, 0x39daf5e8, 0x387165e3, 0x36e5068a, 0x3536cc52,
  0x3367c08f, 0x317900d6, 0x2f6bbe44, 0x2d413ccc, 0x2afad269, 0x2899e64a,
  0x261feff9, 0x238e7673, 0x20e70f32, 0x1e2b5d38, 0x1b5d1009, 0x187de2a6,
  0x158f9a75, 0x1294062e, 0x0f8cfcbd, 0x0c7c5c1e, 0x09640837, 0x0645e9af,
  0x0323ecbe
};

static complex_t pre1[128] = {
  {0x2d64b9da,0x2d1da3d5}, {0x003243f1,0x3fffec42}, {0x18ac4b86,0x3b0d8908},
  {0xe7b09556,0x3b3401bb}, {0x23b836c9,0x351acedc}, {0xf3b4f46d,0x3eceeaad},
  {0xdc9b5fd2,0x3552a8f4}, {0x0cada4f4,0x3ebb4dda}, {0x28c0b4d2,0x31590e3d},
  {0xf9ec1e3c,0x3fb5f4ea}, {0x12c41a4e,0x3d2fd86c}, {0xe201009a,0x38890662},
  {0xd78d014a,0x3198d4ea}, {0x0677edba,0x3fac1a5b}, {0x1e57a86d,0x3859a292},
  {0xed9c1967,0x3d4d0727}, {0x2b2003ab,0x2f49ee0f}, {0xfd0e48ac,0x3feea776},
  {0x15bee78b,0x3c31405f}, {0xe4d068e3,0x39f061d1}, {0x21122240,0x36cb1e29},
  {0xf0a3ca5d,0x3e212179}, {0xda087b6a,0x3385a221}, {0x0995bdfc,0x3f473758},
  {0xd52a795d,0x2f8d7139}, {0x03562037,0x3fe9b8a9}, {0x1b8a7814,0x39c5664f},
  {0xea9fbfee,0x3c531e88}, {0x264843d8,0x3349bf48}, {0xf6cdb35a,0x3f55f796},
  {0xdf441828,0x36fecd0d}, {0x0fbdba40,0x3e08b429}, {0x2c45c89f,0x2e37592c},
  {0xfea02b2e,0x3ffc38d0}, {0x173763c9,0x3ba3fde7}, {0xe63e82bc,0x3a96b636},
  {0x2267d39f,0x35f71fb1}, {0xf22b4e66,0x3e7cd778}, {0xdb4f1967,0x34703094},
  {0x0b228d41,0x3f061e94}, {0x27878893,0x3255483f}, {0xf85c5201,0x3f8adc76},
  {0x11423eef,0x3da106bd}, {0xe0a0211a,0x37c836c2}, {0xd6588726,0x3096e223},
  {0x04e767c4,0x3fcfd50a}, {0x1cf34bae,0x3913eb0e}, {0xec1c6417,0x3cd4c38a},
  {0xd4030685,0x2e7cab1c}, {0x01c454f4,0x3ff9c139}, {0x1a1d6543,0x3a6df8f7},
  {0xe926679d,0x3bc82c1e}, {0x250317de,0x34364da5}, {0xf5407fbd,0x3f174e6f},
  {0xdded1b6f,0x362ce854}, {0x0e36c829,0x3e66d0b4}, {0x29f3984b,0x30553827},
  {0xfb7cda63,0x3fd73a4a}, {0x144310dc,0x3cb53aaa}, {0xe366803d,0x39411e33},
  {0xd8c7b839,0x329321c7}, {0x08077456,0x3f7e8e1e}, {0x1fb7575c,0x3796a996},
  {0xef1ea4b2,0x3dbbd6d4}, {0xd329e182,0xd254a022}, {0x0096cb58,0xc000b1a7},
  {0xe80db22d,0xc4a617a7}, {0x1908ef81,0xc519815f}, {0xdcef4dc2,0xca760086},
  {0x0d101f0d,0xc158e9c1}, {0x240b7542,0xcb1d8e43}, {0xf417ac23,0xc11e137a},
  {0xd7db1b34,0xce27deca}, {0x06dbe9bb,0xc05e5d4f}, {0xedfc7a7d,0xc296615e},
  {0x1eb00695,0xc7d64c48}, {0x290e0660,0xcee73232}, {0xfa503931,0xc040cdbb},
  {0xe259f3a4,0xc748214d}, {0x13241fb6,0xc2eded49}, {0xd5756016,0xd02f80f1},
  {0x03ba80df,0xc01bd3d7}, {0xeafe9c24,0xc38b9828}, {0x1be51517,0xc66623bf},
  {0xdf9aa355,0xc8ce0bc1}, {0x101f1806,0xc2105236}, {0x2698a4a5,0xccf2a21e},
  {0xf7313b60,0xc09be473}, {0x2b6a164c,0xd0fa09c9}, {0xfd72b8d3,0xc00d077c},
  {0xe52b8cee,0xc5e531a2}, {0x161d595c,0xc3f1324e}, {0xda59985a,0xcc3efa25},
  {0x09f917ab,0xc0c82507}, {0x21680b0f,0xc96917ed}, {0xf10574e1,0xc1c70a84},
  {0x2c8e2a86,0xd20e6acd}, {0xff04aeb5,0xc001ed79}, {0xe69aaa49,0xc5411d1b},
  {0x1794f5e6,0xc480c379}, {0xdba1a534,0xcb566ddf}, {0x0b857ec6,0xc10bacc8},
  {0x22bc6dc9,0xca3f2e1a}, {0xf28d8716,0xc16dbbf3}, {0xd6a50d5e,0xcf27ebc5},
  {0x054b9dd2,0xc0382da9}, {0xec7c0a1d,0xc30c49ad}, {0x1d4cd02b,0xc719d4ed},
  {0x27d667d5,0xcde90d7a}, {0xf8c02b32,0xc06971f9}, {0xe0f7e6fa,0xc806c5b5},
  {0x11a2f7fb,0xc27a616a}, {0xd44c4232,0xd13e75a8}, {0x0228d0bb,0xc0095438},
  {0xe9846b64,0xc414392b}, {0x1a790cd3,0xc5bb5473}, {0xde425e90,0xc99dd4b5},
  {0x0e98bba6,0xc1afd008}, {0x2554edd0,0xcc04161e}, {0xf5a3a741,0xc0d81d61},
  {0x2a3f5039,0xcfece916}, {0xfbe127ad,0xc021fdfb}, {0xe3c092b9,0xc6923bec},
  {0x14a253d1,0xc36ae401}, {0xd91759c9,0xcd2f817b}, {0x086b26de,0xc08e5ce6},
  {0x200e8190,0xc89b6cbf}, {0xef7fb1fb,0xc229f168}
};

static complex_t post1[64] = {
  {0x3fffb10b,0x006487c3}, {0x3ffd3968,0x012d936b}, {0x3ff84a3b,0x01f69373},
  {0x3ff0e3b5,0x02bf801a}, {0x3fe7061f,0x038851a2}, {0x3fdab1d9,0x0451004d},
  {0x3fcbe75e,0x0519845e}, {0x3fbaa73f,0x05e1d61a}, {0x3fa6f228,0x06a9edc9},
  {0x3f90c8d9,0x0771c3b2}, {0x3f782c2f,0x08395023}, {0x3f5d1d1c,0x09008b6a},
  {0x3f3f9cab,0x09c76dd8}, {0x3f1fabff,0x0a8defc2}, {0x3efd4c53,0x0b540982},
  {0x3ed87efb,0x0c19b374}, {0x3eb14562,0x0cdee5f9}, {0x3e87a10b,0x0da39977},
  {0x3e5b9392,0x0e67c659}, {0x3e2d1ea7,0x0f2b650f}, {0x3dfc4418,0x0fee6e0d},
  {0x3dc905c4,0x10b0d9cf}, {0x3d9365a7,0x1172a0d7}, {0x3d5b65d1,0x1233bbab},
  {0x3d21086c,0x12f422da}, {0x3ce44fb6,0x13b3cefa}, {0x3ca53e08,0x1472b8a5},
  {0x3c63d5d0,0x1530d880}, {0x3c201994,0x15ee2737}, {0x3bda0bef,0x16aa9d7d},
  {0x3b91af96,0x1766340f}, {0x3b470752,0x1820e3b0}, {0x3afa1605,0x18daa52e},
  {0x3aaadea5,0x19937161}, {0x3a596441,0x1a4b4127}, {0x3a05a9fd,0x1b020d6c},
  {0x39afb313,0x1bb7cf23}, {0x395782d3,0x1c6c7f49}, {0x38fd1ca4,0x1d2016e8},
  {0x38a08402,0x1dd28f14}, {0x3841bc7f,0x1e83e0ea}, {0x37e0c9c2,0x1f340596},
  {0x377daf89,0x1fe2f64b}, {0x371871a4,0x2090ac4d}, {0x36b113fd,0x213d20e8},
  {0x36479a8e,0x21e84d76}, {0x35dc0968,0x22922b5e}, {0x356e64b2,0x233ab413},
  {0x34feb0a5,0x23e1e117}, {0x348cf190,0x2487abf7}, {0x34192bd5,0x252c0e4e},
  {0x33a363eb,0x25cf01c7}, {0x332b9e5d,0x2670801a}, {0x32b1dfc9,0x2710830b},
  {0x32362cdf,0x27af0471}, {0x31b88a66,0x284bfe2f}, {0x3138fd34,0x28e76a37},
  {0x30b78a35,0x2981428b}, {0x30343667,0x2a19813e}, {0x2faf06d9,0x2ab02071},
  {0x2f2800ae,0x2b451a54}, {0x2e9f291b,0x2bd8692b}, {0x2e148566,0x2c6a0746},
  {0x2d881ae7,0x2cf9ef09}
};

static complex_t pre2[64] = {
  {0x3fffb10b,0xff9b783d}, {0x2d881ae7,0x2cf9ef09}, {0x3b470752,0x1820e3b0},
  {0x18daa52e,0x3afa1605}, {0x3ed87efb,0x0c19b374}, {0x23e1e117,0x34feb0a5},
  {0x0cdee5f9,0x3eb14562}, {0x356e64b2,0x233ab413}, {0x3fbaa73f,0x05e1d61a},
  {0x28e76a37,0x3138fd34}, {0x38a08402,0x1dd28f14}, {0x12f422da,0x3d21086c},
  {0x06a9edc9,0x3fa6f228}, {0x31b88a66,0x284bfe2f}, {0x3d5b65d1,0x1233bbab},
  {0x1e83e0ea,0x3841bc7f}, {0x3ff0e3b5,0x02bf801a}, {0x2b451a54,0x2f2800ae},
  {0x3a05a9fd,0x1b020d6c}, {0x15ee2737,0x3c201994}, {0x3e2d1ea7,0x0f2b650f},
  {0x213d20e8,0x36b113fd}, {0x09c76dd8,0x3f3f9cab}, {0x33a363eb,0x25cf01c7},
  {0x038851a2,0x3fe7061f}, {0x2faf06d9,0x2ab02071}, {0x3c63d5d0,0x1530d880},
  {0x1bb7cf23,0x39afb313}, {0x3f5d1d1c,0x09008b6a}, {0x2670801a,0x332b9e5d},
  {0x0fee6e0d,0x3dfc4418}, {0x371871a4,0x2090ac4d}, {0x3ffd3968,0x012d936b},
  {0x2c6a0746,0x2e148566}, {0x3aaadea5,0x19937161}, {0x1766340f,0x3b91af96},
  {0x3e87a10b,0x0da39977}, {0x22922b5e,0x35dc0968}, {0x0b540982,0x3efd4c53},
  {0x348cf190,0x2487abf7}, {0x3f90c8d9,0x0771c3b2}, {0x27af0471,0x32362cdf},
  {0x37e0c9c2,0x1f340596}, {0x1172a0d7,0x3d9365a7}, {0x0519845e,0x3fcbe75e},
  {0x30b78a35,0x2981428b}, {0x3ce44fb6,0x13b3cefa}, {0x1d2016e8,0x38fd1ca4},
  {0x01f69373,0x3ff84a3b}, {0x2e9f291b,0x2bd8692b}, {0x3bda0bef,0x16aa9d7d},
  {0x1a4b4127,0x3a596441}, {0x3f1fabff,0x0a8defc2}, {0x252c0e4e,0x34192bd5},
  {0x0e67c659,0x3e5b9392}, {0x36479a8e,0x21e84d76}, {0x3fdab1d9,0x0451004d},
  {0x2a19813e,0x30343667}, {0x395782d3,0x1c6c7f49}, {0x1472b8a5,0x3ca53e08},
  {0x08395023,0x3f782c2f}, {0x32b1dfc9,0x2710830b}, {0x3dc905c4,0x10b0d9cf},
  {0x1fe2f64b,0x377daf89}
};

static complex_t post2[32] = {
  {0x3ffec42d,0x00c90e8f}, {0x3ff4e5df,0x025b0cae}, {0x3fe12acb,0x03ecadcf},
  {0x3fc395f9,0x057db402}, {0x3f9c2bfa,0x070de171}, {0x3f6af2e3,0x089cf867},
  {0x3f2ff249,0x0a2abb58}, {0x3eeb3347,0x0bb6ecef}, {0x3e9cc076,0x0d415012},
  {0x3e44a5ee,0x0ec9a7f2}, {0x3de2f147,0x104fb80e}, {0x3d77b191,0x11d3443f},
  {0x3d02f756,0x135410c2}, {0x3c84d496,0x14d1e242}, {0x3bfd5cc4,0x164c7ddd},
  {0x3b6ca4c4,0x17c3a931}, {0x3ad2c2e7,0x19372a63}, {0x3a2fcee8,0x1aa6c82b},
  {0x3983e1e7,0x1c1249d8}, {0x38cf1669,0x1d79775b}, {0x3811884c,0x1edc1952},
  {0x374b54ce,0x2039f90e}, {0x367c9a7d,0x2192e09a}, {0x35a5793c,0x22e69ac7},
  {0x34c61236,0x2434f332}, {0x33de87de,0x257db64b}, {0x32eefde9,0x26c0b162},
  {0x31f79947,0x27fdb2a6}, {0x30f8801f,0x29348937}, {0x2ff1d9c6,0x2a650525},
  {0x2ee3cebe,0x2b8ef77c}, {0x2dce88a9,0x2cb2324b}
};

#endif // __IMDCT_TABLES_H__

