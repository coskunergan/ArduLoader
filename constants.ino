
/*
 *  8051 ICSP Programmer Loader 
 *
 *  Created on: Dec 25, 2020
 *
 *  Author: Coskun ERGAN
 * 
 *  V-1.0
 */

const PROGMEM char ReadInitalize[]  =
"0100100010001010010111000000001100000000";

const PROGMEM char EraseStep1[]  =
"0100100010001010010111000000110100000000";

const PROGMEM char EraseStep2[]  =
"0100100010001010010111000001100100000000";

const PROGMEM char EraseStep3[]  =
"0100100010001010010111000010000100000000";

//------------------

const PROGMEM char WriteStep1[]  =
"0100100010001010010111000000000100000000";

const PROGMEM char WriteStep2[]  =
"0100100010001010010111000001010100000000";

const PROGMEM char ReadStep1[]  =
"0100100010001010010111000000000100000000";

//------------------

const PROGMEM char ReadInit[]  =
" \
0110111010101001 0110100001    \
0010111101101010 1100100111    \
0001001000110100 0011111001    \
1010110001010000 1001101111    \
1100101000100110 0100101111    \
0001111011010001 0000001111 \
011100001 \
0101101011110110 0100101001 \
1000010011101100 0111101001 \
0100010111101000 1011001001 \
";

const PROGMEM char ReadSeperate[]  =
" \
0011010111100100 0011101111 \
1101010110111000 0110101111 \
0101000111010011 0111000001 \
000000000 \
1000010011101100 0111101001 \
0100010111101000 1011001001 \
";

const PROGMEM char WriteStart[]  = 
" \
0110001100010111 1100111001 \
            \
1101110010000000 1101010111 \
0101101011110110 0100101001 \
1100101101011001 0100010111 \
1101111110111100 1101011111 \
0101110010010100 0000100111 \
";

const PROGMEM char WriteSeperate[]  = 
" \
1010010010010001 1100011111 \
1110101001101100 0110001001 \
1101010110111000 0110101111 \
0101000111010011 0111000001 \
000000000 \
1100101101011001 0100010111 \
1101111110111100 1101011111 \
0101110010010100 0000100111 \
";

const PROGMEM char WriteFinish[]  = 
" \
1010010010010001 1100011111 \
1110101001101100 0110001001 \
1101010110111000 0110101111 \
0101000111010011 0111000001 \
000000000 \
1010010110010110 0100001111 \
1000001110101110 0011001001 \
1001100110111100 0110101111 \
0111101000100110 1111110001 \
1111010010100010 0000100001 \
";

const PROGMEM char WriteInitalize1[]  = 
" \
0110111010101001 0110100001    \
0010111101101010 1100100111    \
0001001000110100 0011111001    \
1010110001010000 1001101111    \
1100101000100110 0100101111    \
0001111011010001 0000001111 \
011100001 \
1000010101011010 1000011001 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
0111101111100110 0111010111 \
000110111 \
1000010011111001 0100111111 \
0111101111100110 0111010111 \
011001000 \
1000010011111001 0100111111 \
";

const PROGMEM char WriteInitalize2[]  = 
" \
1000010101011010 1000011001 \
0111101111100110 0111010111 \
000101110 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
011010001 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
011111111 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
000000000 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
001110001 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
010001110 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
011100111 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
000011000 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
001111100 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
010000011 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
000011100 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
011100011 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
010011111 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
001100000 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
001111110 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
010000001 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
011110111 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
000001000 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
001000110 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
010111001 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
000000100 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
011111011 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
000000000 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
000000000 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
000110010 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
010111111 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
001010101 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
010101101 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
001011101 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
000010101 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
011110111 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
001011110 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
010110001 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
001110100 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
000000111 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0101100011100110 0010101111 \
0111101111100110 0111010111 \
011111110 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
011111111 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
000000000 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
1000010011111001 0100111111 \
1000010011111001 0100111111 \
0111101111100110 0111010111 \
010111110 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
010111111 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000 \
0111101111100110 0111010111 \
000000000 \
1000010011111001 0100111111 \
0101000111010011 0111000001 \
000000000     \
0101101011110110 0100101001 \
0011110010110101 1001100111 \
                \
0001101001000101 1001011111 \
";
