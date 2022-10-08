/*
	Notes:
		 -> (!) Program assumes file in argv[1] is a valid save file for 4th gen NDS pokemon games
		 -> Program has only been tested on the desmume emulator on linux
*/

#include <climits>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <stdlib.h>

using namespace std;

// - - - Lables for ease of use - - - //

// General Offset Lables
#define trainerNameOffset 0
#define trainerId 1
#define secretId 2
#define smallBlockChecksumOffset 3
#define checksumValueOffset 4
#define leadPokemonOffset 5
#define totalTime 6

// Version Lables for General Offsets
#define diamond 0
#define pearl 0
#define platinum 1
#define heartgold 2
#define soulsilver 2

// Pokemon Data Structure Offset Lables
#define personalityValueOffset 0
#define skipPokemonChecksumOffset 1
#define pokemonChecksumOffset 2
#define speciesID 3
#define heldItem 4
#define otid 5
#define otSecretID 6
#define ability 7
#define moveset 8
#define movePP 9
#define movePPup 10
#define IVs 11
#define nickname 12

// --- Block Addresses --- //

/* Notes:
	-> Small block contains trainer data (name, id, money etc) and party pokemon data (species, ability, EVs, etc)
	-> Big blocks contain data of pokemon stored in the PC boxes (untested)
*/
int smallBlock1 = 0x00000;
int bigBlock1 = 0xcf2c;
int smallBlock2 = 0x40000;
int bigBlock2 = 0x4cf2c;


// --- Small Block Offset for each version --- //

// Offsets for Diamond and Pearl versions
int dp[] = {
	0x64, // trainerNameOffset
	0x74, // trainerID
	0x76, // secretID
	0xc0ec, // smallBlockChecksumOffset
	0xc0fe, // checksumValueOffset
	0x98, // leadPokemonOffset
	0x86 // totalTime - hours: 16bits; minutes: 8bits; seconds: 8bits
};

// Offsets for Platinum versions
int p[] = {
	0x68, // trainerNameOffset
	0x78, // trainerID
	0x7a, // secretID
	0xcf18, // smallBlockChecksumOffset
	0xcf2a, // checksumValueOffset
	0xa0, // leadPokemonOffset
	0x8a // totalTime - hours: 16bits; minutes: 8bits; seconds: 8bits
};

// Offsets for Heartgold and Soulsilver versions
int hgss[] = {
	0x64, // trainerNameOffset
	0x74, // trainerID
	0x76, // secretID
	0xf618, // smallBlockChecksumOffset
	0xf626, // checksumValueOffset
	0x98, // leadPokemonOffset
	0x86 // totalTime - hours: 16bits; minutes: 8bits; seconds: 8bits
};

// - - - Mapping version names to respective offsets - - - //
int* versionNames[] = { {dp}, {p}, {hgss} };


// - - - Offsets for the Pokemon data structure - - - //
int pokemon[] = {

	// Unencrypted Pokemon block offsets
	0x00, // personalityValueOffset
	0x04, // skipPokemonChecksumOffset ( "Bit 0-1: If set, skip checksum checks" ( <- this does not seem to work); "Bit 2: Bad egg flag"; )
	0x06, // pokemonChecksumOffset

	// Encrypted Pokemon block offsets

	// Block A
	0x08, // speciesID
	0x0a, // heldItem
	0x0c, // otid
	0x0e, // otSecretID
	0x15, // ability

	// Block B (untested)
	0x08, // moveset (8 bytes)
	0x10, // Move PP (4 bytes)
	0x14, // Move PP ups (4 bytes)
	0x18, // IVs (4 bytes)

	// Block C
	0x08 // nickname (0x08 - 0x1d)
};


// - - - Handle data from savefile - - - //

// Reading save file data into char vector
void readFile(const char* filename, vector<unsigned char>& data){
	ifstream savefile(filename, ios::binary );
	if(savefile){
		data.assign(istreambuf_iterator<char>(savefile), {});
		savefile.close();
		return;
	}
	savefile.close();
	cout << "Error: could not read file" << endl;
	exit(EXIT_FAILURE);
}

// Partition data from save file and return partition as char vector
vector<unsigned char> getSubVector(vector<unsigned char>& original, int lowerBound, int upperBound){
	auto start = original.begin() + lowerBound;
	auto end = original.begin() + upperBound + 1;
	vector<unsigned char> subVector(upperBound - lowerBound + 1);
	copy(start, end, subVector.begin());
	return subVector;
}

// Write data to savefile from char vector
void writeFile(const char* filename, vector<unsigned char> data){
	ofstream savefile(filename, ios::out | ios::binary );
	if(savefile){
		savefile.write((char*)&data[0], data.size() * sizeof(unsigned char));
		savefile.close();
		return;
	}
	savefile.close();
	cout << "Error: could not write to file" << endl;
	exit(EXIT_FAILURE);

}

// - - - Small Block Functions - - - //

// Calculate savefile checksum for given small block data
int crc16ccitt(vector<unsigned char> dataChunk){

	int table[256] = {0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
			0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
			0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
			0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
			0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
			0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
			0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
			0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
			0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
			0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
			0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
			0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
			0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
			0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
			0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
			0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
			0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
			0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
			0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
			0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
			0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
			0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
			0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
			0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
			0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
			0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
			0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
			0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
			0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
			0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
			0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
			0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0};

	int sum = 0xffff;
	for(unsigned long i = 0; i < (dataChunk.size()-1); i++){
		sum = ( (sum & 0xffff) << 8) ^ table[ ( dataChunk[i] ^ ( (sum >> 8) & 0xff) ) & 0xff  ];
	}

	return (sum & 0xffff);
}

// Update checksum bytes with 'newValue'
void updateChecksum(vector<unsigned char>& data, int newValue, int block, int version){
	if(block == 1){
		int curBlockOffset = smallBlock1;
		data[curBlockOffset + versionNames[version][checksumValueOffset] ] = newValue & 0xff;
		data[curBlockOffset + versionNames[version][checksumValueOffset] + 1] = newValue >> 8;
	}
	else if(block == 2){
		int curBlockOffset = smallBlock2;
		data[curBlockOffset + versionNames[version][checksumValueOffset] ] = newValue & 0xff;
		data[curBlockOffset + versionNames[version][checksumValueOffset] + 1] = newValue >> 8;

	}
	else{
		cout << "Error: could not update Savefile Checksum" << endl;
		exit(EXIT_FAILURE);
	}
}

// Find out in which block the last save was stored
int getCurBlock(vector<unsigned char> data, int version){

	int block1 = data[smallBlock1 + versionNames[version][totalTime]+1];
	int block2 = data[smallBlock2 + versionNames[version][totalTime]+1];

	if(block1 == 0xff){
		cout << "Error: save the game at least twice before editing" << endl;
		exit(EXIT_FAILURE);
	}

	block1 = block1 << 8; block1 += data[smallBlock1 + versionNames[version][totalTime]];
	block1 *= 60*60;
	block1 += data[smallBlock1 + versionNames[version][totalTime]+3];
	block1 += data[smallBlock1 + versionNames[version][totalTime]+2]*60;
	block2 = block2 << 8; block2 += data[smallBlock2 + versionNames[version][totalTime]];
	block2 *= 60*60;
	block2 += data[smallBlock2 + versionNames[version][totalTime]+3];
	block2 += data[smallBlock2 + versionNames[version][totalTime]+2]*60;
	if(block1 > block2){ return 1; }
	else if(block1 < block2){ return 2; }
	else{ return 2;}
}

// - - - Handle Pokemon Data Functions - - - //

// Get the Pokemon's 'Personality Value'
int getPersonalityValue(vector<unsigned char> data, int block, int version){
	if(block == 1){
		int leadPokemon = smallBlock1 + versionNames[version][leadPokemonOffset];
		return (data[leadPokemon+3] << 24) + (data[leadPokemon+2] << 16) + (data[leadPokemon+1] << 8) + data[leadPokemon];
	}
	else if(block == 2){
		int leadPokemon = smallBlock2 + versionNames[version][leadPokemonOffset];
		return (data[leadPokemon+3] << 24) + (data[leadPokemon+2] << 16) + (data[leadPokemon+1] << 8) + data[leadPokemon]; 
	}
	else{
		cout << "Error: could not get Pokemon Personality Value" << endl;
		exit(EXIT_FAILURE);
	}
}

// - - - Character Encoding/Decoding Functions - - - //

int toGameEncoding(char c){

	// Try to return encoding if c is alpha numeric
	if( (c >= 48 && c <= 57) || (c >= 65  && c <= 90) ||  (c >= 97  && c <= 122) ){
		map<char, int> encoding = { {'0', 33,}, {'1', 34,},{'2', 35,},{'3', 36,},{'4', 37,},{'5', 38,},{'6', 39,},{'7', 40,},{'8', 41,},{'9', 42,},{'A', 43,},{'B', 44,},{'C', 45,},{'D', 46,},{'E', 47,},{'F', 48,},{'G', 49,},{'H', 50,},{'I', 51,},{'J', 52,},{'K', 53,},{'L', 54,},{'M', 55,},{'N', 56,},{'O', 57,},{'P', 58,},{'Q', 59,},{'R', 60,},{'S', 61,},{'T', 62,},{'U', 63,},{'V', 64,},{'W', 65,},{'X', 66,},{'Y', 67,},{'Z', 68,},{'a', 69,},{'b', 70,},{'c', 71,},{'d', 72,},{'e', 73,},{'f', 74,},{'g', 75,},{'h', 76,},{'i', 77,},{'j', 78,},{'k', 79,},{'l', 80,},{'m', 81,},{'n', 82,},{'o', 83,},{'p', 84,},{'q', 85,},{'r', 86,},{'s', 87,},{'t', 88,},{'u', 89,},{'v', 90,},{'w', 91,},{'x', 92,},{'y', 93,},{'z', 94,} };
		return encoding[c];
	}
	else{
		// Exits if c is not a valid character
		cout << "Error: invalid characters detected" << endl;
		exit(EXIT_FAILURE);
	}
}

char fromGameEncoding(int n){

	// Try to return encoding if n is withing the game encoding range
	if(n >= 33 && n <= 94){
		map<int, char> encoding = { {33, '0',}, {34, '1',}, {35, '2',}, {36, '3',}, {37, '4',}, {38, '5',}, {39, '6',}, {40, '7',}, {41, '8',}, {42, '9',}, {43, 'A',}, {44, 'B',}, {45, 'C',}, {46, 'D',}, {47, 'E',}, {48, 'F',}, {49, 'G',}, {50, 'H',}, {51, 'I',}, {52, 'J',}, {53, 'K',}, {54, 'L',}, {55, 'M',}, {56, 'N',}, {57, 'O',}, {58, 'P',}, {59, 'Q',}, {60, 'R',}, {61, 'S',}, {62, 'T',}, {63, 'U',}, {64, 'V',}, {65, 'W',}, {66, 'X',}, {67, 'Y',}, {68, 'Z',}, {69, 'a',}, {70, 'b',}, {71, 'c',}, {72, 'd',}, {73, 'e',}, {74, 'f',}, {75, 'g',}, {76, 'h',}, {77, 'i',}, {78, 'j',}, {79, 'k',}, {80, 'l',}, {81, 'm',}, {82, 'n',}, {83, 'o',}, {84, 'p',}, {85, 'q',}, {86, 'r',}, {87, 's',}, {88, 't',}, {89, 'u',}, {90, 'v',}, {91, 'w',}, {92, 'x',}, {93, 'y',}, {94, 'z',} };
		return encoding[n];
	}
	else{
		// Exits if n is not a valid character
		cout << "Error: invalid characters detected" << endl;
		exit(EXIT_FAILURE);
	}
}


// - - - Handle Pokemon Encryption Functions - - - //

// Pokemon data is divided into 4 shuffled blocks of data, this function returns a vector of offsets to each block such that:
	// -> the offset to block A is at index 0
	// -> the offset to block B is at index 1
	// -> the offset to block C is at index 2
	// -> the offset to block D is at index 3

vector<int> getBlockOffsets(int pv){

	// Note: pv stands for Personality Value

	vector<int> ret = {0,0,0,0};

	int orderTable[24][4] = {

			// A offset -> index 0; B offset -> index 1; C offset -> index 2; D offset -> index 3

			{0,32,64,96}, //ABCD
			{0,32,96,64}, //ABDC
			{0,64,32,96}, //ACBD
			{0,96,32,64}, //ACDB
			{0,64,96,32}, //ADBC
			{0,96,64,32}, //ADCB
			{32,0,64,96}, //BACD
			{32,0,96,64}, //BADC
			{64,0,32,96}, //BCAD
			{96,0,32,64}, //BCDA
			{64,0,96,32}, //BDAC
			{96,0,64,32}, //BDCA
			{32,64,0,96}, //CABD
			{32,96,0,64}, //CADB
			{64,32,0,96}, //CBAD
			{96,32,0,64}, //CBDA
			{64,96,0,32}, //CDAB
			{96,64,0,32}, //CDBA
			{32,64,96,0}, //DABC
			{32,96,64,0}, //DACB
			{64,32,96,0}, //DBAC
			{96,32,64,0}, //DBCA
			{64,96,64,0}, //DCAB
			{96,64,32,0}  //DCBA
	};
	int offset = ((pv & 0x3e000) >> 0xd) % 24;
	for(int i = 0; i<4; i++){ ret[i] = orderTable[offset][i];}
	return ret;

}

// Encrypt/Decrypt pokemon data blocks (linear congruential generator)
void prng(vector<unsigned char>& data, long long seed, int block, int version){
	if(block == 1){
		for(int i = 0; i < 128; i += 2){
			seed = ( (0x41C64E6D * seed) + 0x00006073 ) & 0xffffffff;
			data[smallBlock1+versionNames[version][leadPokemonOffset] + 0x08 + i] ^= (seed >> 16) & 0xff;
			data[smallBlock1+versionNames[version][leadPokemonOffset] + 0x08 + i + 1] ^= (seed >> 24) & 0xff;
		}
	}
	else if(block == 2){
		for(int i = 0; i < 128; i += 2){
			seed = ( (0x41C64E6D * seed) + 0x00006073 ) & 0xffffffff;
			data[smallBlock2+versionNames[version][leadPokemonOffset] + 0x08 + i ] ^= (seed >> 16) & 0xff;
			data[smallBlock2+versionNames[version][leadPokemonOffset] + 0x08 + i + 1] ^= (seed >> 24) & 0xff;
		}
	}
	else{
		cout << "Error: could not decrypt Pokemon Data Block" << endl;
		exit(EXIT_FAILURE);
	}
}

// Get the current checksum value of the lead pokemon from the save file data
int getPokemonChecksum(vector<unsigned char> data, int block, int version){
	int ret = 0;
	if(block == 1){
		ret += data[smallBlock1 + versionNames[version][leadPokemonOffset] + pokemon[pokemonChecksumOffset] + 1] << 8;
		ret += data[smallBlock1 + versionNames[version][leadPokemonOffset] + pokemon[pokemonChecksumOffset]];
		return ret;
	}
	else if(block == 2){
		ret += data[smallBlock2 + versionNames[version][leadPokemonOffset] + pokemon[pokemonChecksumOffset] + 1] << 8;
		ret += data[smallBlock2 + versionNames[version][leadPokemonOffset] + pokemon[pokemonChecksumOffset]];
		return ret;
	}
	else{
		cout << "Error: could not get Pokemon Checksum" << endl;
		exit(EXIT_FAILURE);
	}
}

// Recalculate the checksum value of current lead pokemon so that it can be changed after file is edited
int calcPokemonChecksum(vector<unsigned char> dataChunk){
	int sum = 0;
	for(unsigned long i = 0; i < dataChunk.size() - 1; i += 2){
		sum += (dataChunk[i+1] << 8) + dataChunk[i];
	}
	return sum & 0xffff;
}

// Write the new pokemon checksum value to the data vector
void updatePokemonChecksum(vector<unsigned char>& data, int newValue, int block, int version){
	if(block == 1){
		data[smallBlock1 + versionNames[version][leadPokemonOffset] + pokemon[pokemonChecksumOffset]] = newValue & 0xff;
		data[smallBlock1 + versionNames[version][leadPokemonOffset] + pokemon[pokemonChecksumOffset] + 1] = newValue >> 8;
	}
	else if(block == 2){
		data[smallBlock2 + versionNames[version][leadPokemonOffset] + pokemon[pokemonChecksumOffset]] = newValue & 0xff;
		data[smallBlock2 + versionNames[version][leadPokemonOffset] + pokemon[pokemonChecksumOffset] + 1] = newValue >> 8;
	}
	else{
		cout << "Error: could not update Pokemon Checksum" << endl;
		exit(EXIT_FAILURE);
	}
}


// - - - Player Editing Functions - - - //

void changePlayerName(vector<unsigned char>& data, string newName, int block, int version){
	int len = newName.length();
	if(len <= 7 ){
		if(block == 1){
			for(int i = 0; i<16; i++){
				data[smallBlock1 + versionNames[version][trainerNameOffset]+0] = 0;
			}
			for(int i = 0; i<(len*2); i += 2){
				data[smallBlock1 + versionNames[version][trainerNameOffset] + i] = toGameEncoding(newName[i/2]);
				data[smallBlock1 + versionNames[version][trainerNameOffset] + i + 1] = 1;
			}
			data[smallBlock1 + versionNames[version][trainerNameOffset] + len*2] = 0xff;
			data[smallBlock1 + versionNames[version][trainerNameOffset] + len*2+1] = 0xff;
		}
		else if(block == 2){
			for(int i = 0; i<16; i++){
				data[smallBlock2 + versionNames[version][trainerNameOffset]+0] = 0;
			}
			for(int i = 0; i<(len*2); i += 2){
				data[smallBlock2 + versionNames[version][trainerNameOffset] + i] = toGameEncoding(newName[i/2]);
				data[smallBlock2 + versionNames[version][trainerNameOffset] + i + 1] = 1;
			}
			data[smallBlock2 + versionNames[version][trainerNameOffset] + len*2] = 0xff;
			data[smallBlock2 + versionNames[version][trainerNameOffset] + len*2+1] = 0xff;
		}
		else{
			cout << "Error: couldn't change player name"  << endl;
			exit(EXIT_FAILURE);
		}
	}
	else{
		cout << "Invalid name\nMake sure the deisred name is 7 characters long and consists of only alphanumerics." << endl;
		exit(EXIT_FAILURE);
	}

	// Update save file checksum
	vector<unsigned char> tmp = getSubVector(data, smallBlock1, smallBlock1 + versionNames[version][smallBlockChecksumOffset]);
	int sum =  crc16ccitt(tmp);
	updateChecksum(data, sum, block, version);
}


// - - - Pokemon Editing Functions - - - //

// Edit the species of lead pokemon
void editPokemonSpecies(vector<unsigned char>& data, string pokemonName, vector<int> blockOffsets, int block, int version){


	// Get Pokemon Species ID for given 'pokemonName'
	map<string, int> pokedex = { {"Bulbasaur", 1,}, {"Ivysaur", 2,}, {"Venusaur", 3,}, {"Charmander", 4,}, {"Charmeleon", 5,}, {"Charizard", 6,}, {"Squirtle", 7,}, {"Wartortle", 8,}, {"Blastoise", 9,}, {"Caterpie", 10,}, {"Metapod", 11,}, {"Butterfree", 12,}, {"Weedle", 13,}, {"Kakuna", 14,}, {"Beedrill", 15,}, {"Pidgey", 16,}, {"Pidgeotto", 17,}, {"Pidgeot", 18,}, {"Rattata", 19,}, {"Raticate", 20,}, {"Spearow", 21,}, {"Fearow", 22,}, {"Ekans", 23,}, {"Arbok", 24,}, {"Pikachu", 25,}, {"Raichu", 26,}, {"Sandshrew", 27,}, {"Sandslash", 28,}, {"NidoranF", 29,}, {"Nidorina", 30,}, {"Nidoqueen", 31,}, {"NidoranM", 32,}, {"Nidorino", 33,}, {"Nidoking", 34,}, {"Clefairy", 35,}, {"Clefable", 36,}, {"Vulpix", 37,}, {"Ninetales", 38,}, {"Jigglypuff", 39,}, {"Wigglytuff", 40,}, {"Zubat", 41,}, {"Golbat", 42,}, {"Oddish", 43,}, {"Gloom", 44,}, {"Vileplume", 45,}, {"Paras", 46,}, {"Parasect", 47,}, {"Venonat", 48,}, {"Venomoth", 49,}, {"Diglett", 50,}, {"Dugtrio", 51,}, {"Meowth", 52,}, {"Persian", 53,}, {"Psyduck", 54,}, {"Golduck", 55,}, {"Mankey", 56,}, {"Primeape", 57,}, {"Growlithe", 58,}, {"Arcanine", 59,}, {"Poliwag", 60,}, {"Poliwhirl", 61,}, {"Poliwrath", 62,}, {"Abra", 63,}, {"Kadabra", 64,}, {"Alakazam", 65,}, {"Machop", 66,}, {"Machoke", 67,}, {"Machamp", 68,}, {"Bellsprout", 69,}, {"Weepinbell", 70,}, {"Victreebel", 71,}, {"Tentacool", 72,}, {"Tentacruel", 73,}, {"Geodude", 74,}, {"Graveler", 75,}, {"Golem", 76,}, {"Ponyta", 77,}, {"Rapidash", 78,}, {"Slowpoke", 79,}, {"Slowbro", 80,}, {"Magnemite", 81,}, {"Magneton", 82,}, {"Farfetch'd", 83,}, {"Doduo", 84,}, {"Dodrio", 85,}, {"Seel", 86,}, {"Dewgong", 87,}, {"Grimer", 88,}, {"Muk", 89,}, {"Shellder", 90,}, {"Cloyster", 91,}, {"Gastly", 92,}, {"Haunter", 93,}, {"Gengar", 94,}, {"Onix", 95,}, {"Drowzee", 96,}, {"Hypno", 97,}, {"Krabby", 98,}, {"Kingler", 99,}, {"Voltorb", 100,}, {"Electrode", 101,}, {"Exeggcute", 102,}, {"Exeggutor", 103,}, {"Cubone", 104,}, {"Marowak", 105,}, {"Hitmonlee", 106,}, {"Hitmonchan", 107,}, {"Lickitung", 108,}, {"Koffing", 109,}, {"Weezing", 110,}, {"Rhyhorn", 111,}, {"Rhydon", 112,}, {"Chansey", 113,}, {"Tangela", 114,}, {"Kangaskhan", 115,}, {"Horsea", 116,}, {"Seadra", 117,}, {"Goldeen", 118,}, {"Seaking", 119,}, {"Staryu", 120,}, {"Starmie", 121,}, {"Mr. Mime", 122,}, {"Scyther", 123,}, {"Jynx", 124,}, {"Electabuzz", 125,}, {"Magmar", 126,}, {"Pinsir", 127,}, {"Tauros", 128,}, {"Magikarp", 129,}, {"Gyarados", 130,}, {"Lapras", 131,}, {"Ditto", 132,}, {"Eevee", 133,}, {"Vaporeon", 134,}, {"Jolteon", 135,}, {"Flareon", 136,}, {"Porygon", 137,}, {"Omanyte", 138,}, {"Omastar", 139,}, {"Kabuto", 140,}, {"Kabutops", 141,}, {"Aerodactyl", 142,}, {"Snorlax", 143,}, {"Articuno", 144,}, {"Zapdos", 145,}, {"Moltres", 146,}, {"Dratini", 147,}, {"Dragonair", 148,}, {"Dragonite", 149,}, {"Mewtwo", 150,}, {"Mew", 151,}, {"Chikorita", 152,}, {"Bayleef", 153,}, {"Meganium", 154,}, {"Cyndaquil", 155,}, {"Quilava", 156,}, {"Typhlosion", 157,}, {"Totodile", 158,}, {"Croconaw", 159,}, {"Feraligatr", 160,}, {"Sentret", 161,}, {"Furret", 162,}, {"Hoothoot", 163,}, {"Noctowl", 164,}, {"Ledyba", 165,}, {"Ledian", 166,}, {"Spinarak", 167,}, {"Ariados", 168,}, {"Crobat", 169,}, {"Chinchou", 170,}, {"Lanturn", 171,}, {"Pichu", 172,}, {"Cleffa", 173,}, {"Igglybuff", 174,}, {"Togepi", 175,}, {"Togetic", 176,}, {"Natu", 177,}, {"Xatu", 178,}, {"Mareep", 179,}, {"Flaaffy", 180,}, {"Ampharos", 181,}, {"Bellossom", 182,}, {"Marill", 183,}, {"Azumarill", 184,}, {"Sudowoodo", 185,}, {"Politoed", 186,}, {"Hoppip", 187,}, {"Skiploom", 188,}, {"Jumpluff", 189,}, {"Aipom", 190,}, {"Sunkern", 191,}, {"Sunflora", 192,}, {"Yanma", 193,}, {"Wooper", 194,}, {"Quagsire", 195,}, {"Espeon", 196,}, {"Umbreon", 197,}, {"Murkrow", 198,}, {"Slowking", 199,}, {"Misdreavus", 200,}, {"Unown", 201,}, {"Wobbuffet", 202,}, {"Girafarig", 203,}, {"Pineco", 204,}, {"Forretress", 205,}, {"Dunsparce", 206,}, {"Gligar", 207,}, {"Steelix", 208,}, {"Snubbull", 209,}, {"Granbull", 210,}, {"Qwilfish", 211,}, {"Scizor", 212,}, {"Shuckle", 213,}, {"Heracross", 214,}, {"Sneasel", 215,}, {"Teddiursa", 216,}, {"Ursaring", 217,}, {"Slugma", 218,}, {"Magcargo", 219,}, {"Swinub", 220,}, {"Piloswine", 221,}, {"Corsola", 222,}, {"Remoraid", 223,}, {"Octillery", 224,}, {"Delibird", 225,}, {"Mantine", 226,}, {"Skarmory", 227,}, {"Houndour", 228,}, {"Houndoom", 229,}, {"Kingdra", 230,}, {"Phanpy", 231,}, {"Donphan", 232,}, {"Porygon2", 233,}, {"Stantler", 234,}, {"Smeargle", 235,}, {"Tyrogue", 236,}, {"Hitmontop", 237,}, {"Smoochum", 238,}, {"Elekid", 239,}, {"Magby", 240,}, {"Miltank", 241,}, {"Blissey", 242,}, {"Raikou", 243,}, {"Entei", 244,}, {"Suicune", 245,}, {"Larvitar", 246,}, {"Pupitar", 247,}, {"Tyranitar", 248,}, {"Lugia", 249,}, {"Ho-Oh", 250,}, {"Celebi", 251,}, {"Treecko", 252,}, {"Grovyle", 253,}, {"Sceptile", 254,}, {"Torchic", 255,}, {"Combusken", 256,}, {"Blaziken", 257,}, {"Mudkip", 258,}, {"Marshtomp", 259,}, {"Swampert", 260,}, {"Poochyena", 261,}, {"Mightyena", 262,}, {"Zigzagoon", 263,}, {"Linoone", 264,}, {"Wurmple", 265,}, {"Silcoon", 266,}, {"Beautifly", 267,}, {"Cascoon", 268,}, {"Dustox", 269,}, {"Lotad", 270,}, {"Lombre", 271,}, {"Ludicolo", 272,}, {"Seedot", 273,}, {"Nuzleaf", 274,}, {"Shiftry", 275,}, {"Taillow", 276,}, {"Swellow", 277,}, {"Wingull", 278,}, {"Pelipper", 279,}, {"Ralts", 280,}, {"Kirlia", 281,}, {"Gardevoir", 282,}, {"Surskit", 283,}, {"Masquerain", 284,}, {"Shroomish", 285,}, {"Breloom", 286,}, {"Slakoth", 287,}, {"Vigoroth", 288,}, {"Slaking", 289,}, {"Nincada", 290,}, {"Ninjask", 291,}, {"Shedinja", 292,}, {"Whismur", 293,}, {"Loudred", 294,}, {"Exploud", 295,}, {"Makuhita", 296,}, {"Hariyama", 297,}, {"Azurill", 298,}, {"Nosepass", 299,}, {"Skitty", 300,}, {"Delcatty", 301,}, {"Sableye", 302,}, {"Mawile", 303,}, {"Aron", 304,}, {"Lairon", 305,}, {"Aggron", 306,}, {"Meditite", 307,}, {"Medicham", 308,}, {"Electrike", 309,}, {"Manectric", 310,}, {"Plusle", 311,}, {"Minun", 312,}, {"Volbeat", 313,}, {"Illumise", 314,}, {"Roselia", 315,}, {"Gulpin", 316,}, {"Swalot", 317,}, {"Carvanha", 318,}, {"Sharpedo", 319,}, {"Wailmer", 320,}, {"Wailord", 321,}, {"Numel", 322,}, {"Camerupt", 323,}, {"Torkoal", 324,}, {"Spoink", 325,}, {"Grumpig", 326,}, {"Spinda", 327,}, {"Trapinch", 328,}, {"Vibrava", 329,}, {"Flygon", 330,}, {"Cacnea", 331,}, {"Cacturne", 332,}, {"Swablu", 333,}, {"Altaria", 334,}, {"Zangoose", 335,}, {"Seviper", 336,}, {"Lunatone", 337,}, {"Solrock", 338,}, {"Barboach", 339,}, {"Whiscash", 340,}, {"Corphish", 341,}, {"Crawdaunt", 342,}, {"Baltoy", 343,}, {"Claydol", 344,}, {"Lileep", 345,}, {"Cradily", 346,}, {"Anorith", 347,}, {"Armaldo", 348,}, {"Feebas", 349,}, {"Milotic", 350,}, {"Castform", 351,}, {"Kecleon", 352,}, {"Shuppet", 353,}, {"Banette", 354,}, {"Duskull", 355,}, {"Dusclops", 356,}, {"Tropius", 357,}, {"Chimecho", 358,}, {"Absol", 359,}, {"Wynaut", 360,}, {"Snorunt ", 361,}, {"Glalie", 362,}, {"Spheal", 363,}, {"Sealeo", 364,}, {"Walrein", 365,}, {"Clamperl", 366,}, {"Huntail", 367,}, {"Gorebyss", 368,}, {"Relicanth", 369,}, {"Luvdisc", 370,}, {"Bagon", 371,}, {"Shelgon", 372,}, {"Salamence", 373,}, {"Beldum", 374,}, {"Metang", 375,}, {"Metagross", 376,}, {"Regirock", 377,}, {"Regice", 378,}, {"Registeel", 379,}, {"Latias", 380,}, {"Latios", 381,}, {"Kyogre", 382,}, {"Groudon", 383,}, {"Rayquaza", 384,}, {"Jirachi", 385,}, {"Deoxys", 386,}, {"Turtwig", 387,}, {"Grotle", 388,}, {"Torterra", 389,}, {"Chimchar", 390,}, {"Monferno", 391,}, {"Infernape", 392,}, {"Piplup", 393,}, {"Prinplup", 394,}, {"Empoleon", 395,}, {"Starly", 396,}, {"Staravia", 397,}, {"Staraptor", 398,}, {"Bidoof", 399,}, {"Bibarel", 400,}, {"Kricketot", 401,}, {"Kricketune", 402,}, {"Shinx", 403,}, {"Luxio", 404,}, {"Luxray", 405,}, {"Budew", 406,}, {"Roserade", 407,}, {"Cranidos", 408,}, {"Rampardos", 409,}, {"Shieldon", 410,}, {"Bastiodon", 411,}, {"Burmy", 412,}, {"Wormadam", 413,}, {"Mothim", 414,}, {"Combee", 415,}, {"Vespiquen", 416,}, {"Pachirisu", 417,}, {"Buizel", 418,}, {"Floatzel", 419,}, {"Cherubi", 420,}, {"Cherrim", 421,}, {"Shellos", 422,}, {"Gastrodon", 423,}, {"Ambipom", 424,}, {"Drifloon", 425,}, {"Drifblim", 426,}, {"Buneary", 427,}, {"Lopunny", 428,}, {"Mismagius", 429,}, {"Honchkrow", 430,}, {"Glameow", 431,}, {"Purugly", 432,}, {"Chingling", 433,}, {"Stunky", 434,}, {"Skuntank", 435,}, {"Bronzor", 436,}, {"Bronzong", 437,}, {"Bonsly", 438,}, {"Mime Jr.", 439,}, {"Happiny", 440,}, {"Chatot", 441,}, {"Spiritomb", 442,}, {"Gible", 443,}, {"Gabite", 444,}, {"Garchomp", 445,}, {"Munchlax", 446,}, {"Riolu", 447,}, {"Lucario", 448,}, {"Hippopotas", 449,}, {"Hippowdon", 450,}, {"Skorupi", 451,}, {"Drapion", 452,}, {"Croagunk", 453,}, {"Toxicroak", 454,}, {"Carnivine", 455,}, {"Finneon", 456,}, {"Lumineon", 457,}, {"Mantyke", 458,}, {"Snover", 459,}, {"Abomasnow", 460,}, {"Weavile", 461,}, {"Magnezone", 462,}, {"Lickilicky", 463,}, {"Rhyperior", 464,}, {"Tangrowth", 465,}, {"Electivire", 466,}, {"Magmortar", 467,}, {"Togekiss", 468,}, {"Yanmega", 469,}, {"Leafeon", 470,}, {"Glaceon", 471,}, {"Gliscor", 472,}, {"Mamoswine", 473,}, {"Porygon-Z", 474,}, {"Gallade", 475,}, {"Probopass", 476,}, {"Dusknoir", 477,}, {"Froslass", 478,}, {"Rotom", 479,}, {"Uxie", 480,}, {"Mesprit", 481,}, {"Azelf", 482,}, {"Dialga", 483,}, {"Palkia", 484,}, {"Heatran", 485,}, {"Regigigas", 486,}, {"Giratina", 487,}, {"Cresselia", 488,}, {"Phione", 489,}, {"Manaphy", 490,}, {"Darkrai", 491,}, {"Shaymin", 492,}, {"Arceus", 493,} };
	int id = pokedex[pokemonName];
	if(!id){
		cout << "Error: invalid Pokemon name" << endl;
		exit(EXIT_FAILURE);
	}

	// Update pokemon species
	if(block == 1){
		data[smallBlock1 + versionNames[version][leadPokemonOffset] + blockOffsets[0] + pokemon[speciesID] ] = id & 0xff;
		data[smallBlock1 + versionNames[version][leadPokemonOffset] + blockOffsets[0] + pokemon[speciesID] + 1] = id >> 8;
	}
	else if(block == 2){
		data[smallBlock2 + versionNames[version][leadPokemonOffset] + blockOffsets[0] + pokemon[speciesID] ] = id & 0xff;
		data[smallBlock2 + versionNames[version][leadPokemonOffset] + blockOffsets[0] + pokemon[speciesID] + 1] = id >> 8;
	}
	else{
		cout << "Error: could not edit pokemon" << endl;
		exit(EXIT_FAILURE);
	}

	// Update pokemon name
	for(unsigned long i = 0; i<pokemonName.size(); i++){ pokemonName[i] = toupper(pokemonName[i]);}
	if(block == 1){
		for(int i = 0; i<22; i++){
			data[smallBlock1 + versionNames[version][leadPokemonOffset] + blockOffsets[2] + pokemon[nickname] + i ] = 0;
		}
		for(unsigned long i = 0; i<(pokemonName.size()*2); i+=2){
			data[smallBlock1 + versionNames[version][leadPokemonOffset] + blockOffsets[2] + pokemon[nickname] + i ] = toGameEncoding(pokemonName[i/2]);
			data[smallBlock1 + versionNames[version][leadPokemonOffset] + blockOffsets[2] + pokemon[nickname] + i + 1] = 1;
		}
		data[smallBlock1 + versionNames[version][leadPokemonOffset] + blockOffsets[2] + pokemon[nickname] + pokemonName.size()*2] = 0xff;
		data[smallBlock1 + versionNames[version][leadPokemonOffset] + blockOffsets[2] + pokemon[nickname] + pokemonName.size()*2 + 1] = 0xff;
	}
	else if(block == 2){
		for(int i = 0; i<22; i++){
			data[smallBlock2 + versionNames[version][leadPokemonOffset] + blockOffsets[2] + pokemon[nickname] + i ] = 0;
		}
		for(unsigned long i = 0; i<(pokemonName.size()*2); i+=2){
			data[smallBlock2 + versionNames[version][leadPokemonOffset] + blockOffsets[2] + pokemon[nickname] + i ] = toGameEncoding(pokemonName[i/2]);
			data[smallBlock2 + versionNames[version][leadPokemonOffset] + blockOffsets[2] + pokemon[nickname] + i + 1] = 1;
		}
		data[smallBlock2 + versionNames[version][leadPokemonOffset] + blockOffsets[2] + pokemon[nickname] + pokemonName.size()*2] = 0xff;
		data[smallBlock2 + versionNames[version][leadPokemonOffset] + blockOffsets[2] + pokemon[nickname] + pokemonName.size()*2 + 1] = 0xff;
	}
	else{
		cout << "Error: could not edit pokemon" << endl;
		exit(EXIT_FAILURE);
		}

}

// Edit the ability of lead pokemon
void editPokemonAbility(vector<unsigned char>& data, string abilityName, vector<int> blockOffsets, int block, int version){

	// Get the Ability ID for given 'abilityName'

	map<string, int> abilityMap= { {"Adaptability", 91,},{"Aftermath", 106,},{"Air Lock", 76,},{"Anger Point", 83,},{"Anticipation", 107,},{"Arena Trap", 71,},{"Bad Dreams", 123,},{"Battle Armor", 4,},{"Blaze", 66,},{"Chlorophyll", 34,},{"Clear Body", 29,},{"Cloud Nine", 13,},{"Color Change", 16,},{"Compound Eyes", 14,},{"Cute Charm", 56,},{"Damp", 6,},{"Download", 88,},{"Drizzle", 2,},{"Drought", 70,},{"Dry Skin", 87,},{"Early Bird", 48,},{"Effect Spore", 27,},{"Filter", 111,},{"Flame Body", 49,},{"Flash Fire", 18,},{"Flower Gift", 122,},{"Forecast", 59,},{"Forewarn", 108,},{"Frisk", 119,},{"Gluttony", 82,},{"Guts", 62,},{"Heatproof", 85,},{"Honey Gather", 118,},{"Huge Power", 37,},{"Hustle", 55,},{"Hydration", 93,},{"Hyper Cutter", 52,},{"Ice Body", 115,},{"Illuminate", 35,},{"Immunity", 17,},{"Inner Focus", 39,},{"Insomnia", 15,},{"Intimidate", 22,},{"Iron Fist", 89,},{"Keen Eye", 51,},{"Klutz", 103,},{"Leaf Guard", 102,},{"Levitate", 26,},{"Lightning Rod", 31,},{"Limber", 7,},{"Liquid Ooze", 64,},{"Magic Guard", 98,},{"Magma Armor", 40,},{"Magnet Pull", 42,},{"Marvel Scale", 63,},{"Minus", 58,},{"Mold Breaker", 104,},{"Motor Drive", 78,},{"Multitype", 121,},{"Natural Cure", 30,},{"No Guard", 99,},{"Normalize", 96,},{"Oblivious", 12,},{"Overgrow", 65,},{"Own Tempo", 20,},{"Pickup", 53,},{"Plus", 57,},{"Poison Heal", 90,},{"Poison Point", 38,},{"Pressure", 46,},{"Pure Power", 74,},{"Quick Feet", 95,},{"Rain Dish", 44,},{"Reckless", 120,},{"Rivalry", 79,},{"Rock Head", 69,},{"Rough Skin", 24,},{"Run Away", 50,},{"Sand Stream", 45,},{"Sand Veil", 8,},{"Scrappy", 113,},{"Serene Grace", 32,},{"Shadow Tag", 23,},{"Shed Skin", 61,},{"Shell Armor", 75,},{"Shield Dust", 19,},{"Simple", 86,},{"Skill Link", 92,},{"Slow Start", 112,},{"Sniper", 97,},{"Snow Cloak", 81,},{"Snow Warning", 117,},{"Solar Power", 94,},{"Solid Rock", 116,},{"Soundproof", 43,},{"Speed Boost", 3,},{"Stall", 100,},{"Static", 9,},{"Steadfast", 80,},{"Stench", 1,},{"Sticky Hold", 60,},{"Storm Drain", 114,},{"Sturdy", 5,},{"Suction Cups", 21,},{"Super Luck", 105,},{"Swarm", 68,},{"Swift Swim", 33,},{"Synchronize", 28,},{"Tangled Feet", 77,},{"Technician", 101,},{"Thick Fat", 47,},{"Tinted Lens", 110,},{"Torrent", 67,},{"Trace", 36,},{"Truant", 54,},{"Unaware", 109,},{"Unburden", 84,},{"Vital Spirit", 72,},{"Volt Absorb", 10,},{"Water Absorb", 11,},{"Water Veil", 41,},{"White Smoke", 73,},{"Wonder Guard", 25,} };
	int id = abilityMap[abilityName];
	if(!id){
		cout << "Error: invalid Ability name" << endl;
		exit(EXIT_FAILURE);
	}


	// Update pokemon ability
	if(block == 1){
		data[smallBlock1 + versionNames[version][leadPokemonOffset] + blockOffsets[0] + pokemon[ability] ] = id;
	}
	else if(block == 2){
		data[smallBlock2 + versionNames[version][leadPokemonOffset] + blockOffsets[0] + pokemon[ability] ] = id;
	}
	else{
		cout << "Error: could not edit pokemon" << endl;
		exit(EXIT_FAILURE);
	}

}

// Edit the moves of lead pokemon
void editPokemonMove(vector<unsigned char>& data, string moveName, int moveSlot, vector<int> blockOffsets, int block, int version){

	int moveSlotOffset = -1;

	// Make sure the value of 'moveSlot' is valid
	if(moveSlot == 1 || moveSlot == 2 || moveSlot == 3 || moveSlot == 4){

		// Since each move is 2 bytes of memory the value of 'moveSlot' is adjusted to it can be used more intuitively
		moveSlotOffset = moveSlot*2 - 2;
	}
	else{
		cout << "Error: could not edit pokemon" << endl;
		exit(EXIT_FAILURE);
	}

	// Get Move ID and PP amount for given 'moveName'

	map<string, int> moveMap = { {"Pound", 291,},{"Karate Chop", 537,},{"Double Slap", 778,},{"Comet Punch", 1039,},{"Mega Punch", 1300,},{"Pay Day", 1556,},{"Fire Punch", 1807,},{"Ice Punch", 2063,},{"Thunder Punch", 2319,},{"Scratch", 2595,},{"Vise Grip", 2846,},{"Guillotine", 3077,},{"Razor Wind", 3338,},{"Swords Dance", 3604,},{"Cut", 3870,},{"Gust", 4131,},{"Wing Attack", 4387,},{"Whirlwind", 4628,},{"Fly", 4879,},{"Bind", 5140,},{"Slam", 5396,},{"Vine", 5657,},{"Stomp", 5908,},{"Double Kick", 6174,},{"Mega Kick", 6405,},{"Jump Kick", 6666,},{"Rolling Kick", 6927,},{"Sand Attack", 7183,},{"Headbutt", 7439,},{"Horn Attack", 7705,},{"Fury Attack", 7956,},{"Horn Drill", 8197,},{"Tackle", 8483,},{"Body Slam", 8719,},{"Wrap", 8980,},{"Take Down", 9236,},{"Thrash", 9482,},{"Double-Edge", 9743,},{"Tail Whip", 10014,},{"Poison Sting", 10275,},{"Twineedle", 10516,},{"Pin Missile", 10772,},{"Leer", 11038,},{"Bite", 11289,},{"Growl", 11560,},{"Roar", 11796,},{"Sing", 12047,},{"Supersonic", 12308,},{"Sonic Boom", 12564,},{"Disable", 12820,},{"Acid", 13086,},{"Ember", 13337,},{"Flamethrower", 13583,},{"Mist", 13854,},{"Water Gun", 14105,},{"Hydro Pump", 14341,},{"Surf", 14607,},{"Ice Beam", 14858,},{"Blizzard", 15109,},{"Psybeam", 15380,},{"Bubble Beam", 15636,},{"Aurora Beam", 15892,},{"Hyper Beam", 16133,},{"Peck", 16419,},{"Drill Peck", 16660,},{"Submission", 16916,},{"Low Kick", 17172,},{"Counter", 17428,},{"Seismic Toss", 17684,},{"Strength", 17935,},{"Absorb", 18181,},{"Mega Drain", 18447,},{"Leech Seed", 18698,},{"Growth", 18964,},{"Razor Leaf", 19225,},{"Solar Beam", 19466,},{"Poison Powder", 19747,},{"Stun Spore", 19998,},{"Sleep Powder", 20239,},{"Petal Dance", 20490,},{"String Shot", 20776,},{"Dragon Rage", 21002,},{"Fire Spin", 21263,},{"Thunder Shock", 21534,},{"Thunderbolt", 21775,},{"Thunder Wave", 22036,},{"Thunder", 22282,},{"Rock Throw", 22543,},{"Earthquake", 22794,},{"Fissure", 23045,},{"Dig", 23306,},{"Toxic", 23562,},{"Confusion", 23833,},{"Psychic", 24074,},{"Hypnosis", 24340,},{"Meditate", 24616,},{"Agility", 24862,},{"Quick Attack", 25118,},{"Rage", 25364,},{"Teleport", 25620,},{"Night Shade", 25871,},{"Mimic", 26122,},{"Screech", 26408,},{"Double Team", 26639,},{"Recover", 26890,},{"Harden", 27166,},{"Minimize", 27402,},{"Smokescreen", 27668,},{"Confuse Ray", 27914,},{"Withdraw", 28200,},{"Defense Curl", 28456,},{"Barrier", 28692,},{"Light Screen", 28958,},{"Haze", 29214,},{"Reflect", 29460,},{"Focus Energy", 29726,},{"Bide", 29962,},{"Metronome", 30218,},{"Mirror Move", 30484,},{"Self-Destruct", 30725,},{"Egg Bomb", 30986,},{"Lick", 31262,},{"Smog", 31508,},{"Sludge", 31764,},{"Bone Club", 32020,},{"Fire Blast", 32261,},{"Waterfall", 32527,},{"Clamp", 32783,},{"Swift", 33044,},{"Skull Bash", 33290,},{"Spike Cannon", 33551,},{"Constrict", 33827,},{"Amnesia", 34068,},{"Kinesis", 34319,},{"Soft-Boiled", 34570,},{"High Jump Kick", 34826,},{"Glare", 35102,},{"Dream Eater", 35343,},{"Poison Gas", 35624,},{"Barrage", 35860,},{"Leech Life", 36106,},{"Lovely Kiss", 36362,},{"Sky Attack", 36613,},{"Transform", 36874,},{"Bubble", 37150,},{"Dizzy Punch", 37386,},{"Spore", 37647,},{"Flash", 37908,},{"Psywave", 38159,},{"Splash", 38440,},{"Acid Armor", 38676,},{"Crabhammer", 38922,},{"Explosion", 39173,},{"Fury Swipes", 39439,},{"Bonemerang", 39690,},{"Rest", 39946,},{"Rock Slide", 40202,},{"Hyper Fang", 40463,},{"Sharpen", 40734,},{"Conversion", 40990,},{"Tri Attack", 41226,},{"Super Fang", 41482,},{"Slash", 41748,},{"Substitute", 41994,},{"Struggle", 42241,},{"Sketch", 42497,},{"Triple Kick", 42762,},{"Thief", 43033,},{"Spider Web", 43274,},{"Mind Reader", 43525,},{"Nightmare", 43791,},{"Flame Wheel", 44057,},{"Snore", 44303,},{"Curse", 44554,},{"Flail", 44815,},{"Conversion 2", 45086,},{"Aeroblast", 45317,},{"Cotton Spore", 45608,},{"Reversal", 45839,},{"Spite", 46090,},{"Powder Snow", 46361,},{"Protect", 46602,},{"Mach Punch", 46878,},{"Scary Face", 47114,},{"Feint Attack", 47380,},{"Sweet Kiss", 47626,},{"Belly Drum", 47882,},{"Sludge Bomb", 48138,},{"Mud-Slap", 48394,},{"Octazooka", 48650,},{"Spikes", 48916,},{"Zap Cannon", 49157,},{"Foresight", 49448,},{"Destiny Bond", 49669,},{"Perish Song", 49925,},{"Icy Wind", 50191,},{"Detect", 50437,},{"Bone Rush", 50698,},{"Lock-On", 50949,},{"Outrage", 51210,},{"Sandstorm", 51466,},{"Giga Drain", 51722,},{"Endure", 51978,},{"Charm", 52244,},{"Rollout", 52500,},{"False Swipe", 52776,},{"Swagger", 53007,},{"Milk Drink", 53258,},{"Spark", 53524,},{"Fury Cutter", 53780,},{"Steel Wing", 54041,},{"Mean Look", 54277,},{"Attract", 54543,},{"Sleep Talk", 54794,},{"Heal Bell", 55045,},{"Return", 55316,},{"Present", 55567,},{"Frustration", 55828,},{"Safeguard", 56089,},{"Pain Split", 56340,},{"Sacred Fire", 56581,},{"Magnitude", 56862,},{"Dynamic Punch", 57093,},{"Megahorn", 57354,},{"Dragon Breath", 57620,},{"Baton Pass", 57896,},{"Encore", 58117,},{"Pursuit", 58388,},{"Rapid Spin", 58664,},{"Sweet Scent", 58900,},{"Iron Tail", 59151,},{"Metal Claw", 59427,},{"Vital Throw", 59658,},{"Morning Sun", 59909,},{"Synthesis", 60165,},{"Moonlight", 60421,},{"Hidden Power", 60687,},{"Cross Chop", 60933,},{"Twister", 61204,},{"Rain Dance", 61445,},{"Sunny Day", 61701,},{"Crunch", 61967,},{"Mirror Coat", 62228,},{"Psych Up", 62474,},{"Extreme Speed", 62725,},{"Ancient Power", 62981,},{"Shadow Ball", 63247,},{"Future Sight", 63498,},{"Rock Smash", 63759,},{"Whirlpool", 64015,},{"Beat Up", 64266,},{"Fake Out", 64522,},{"Uproar", 64778,},{"Stockpile", 65044,},{"Spit Up", 65290,},{"Swallow", 65546,},{"Heat Wave", 65802,},{"Hail", 66058,},{"Torment", 66319,},{"Flatter", 66575,},{"Will-O-Wisp", 66831,},{"Memento", 67082,},{"Facade", 67348,},{"Focus Punch", 67604,},{"Smelling Salts", 67850,},{"Follow Me", 68116,},{"Nature Power", 68372,},{"Charge", 68628,},{"Taunt", 68884,},{"Helping Hand", 69140,},{"Trick", 69386,},{"Role Play", 69642,},{"Wish", 69898,},{"Assist", 70164,},{"Ingrain", 70420,},{"Superpower", 70661,},{"Magic Coat", 70927,},{"Recycle", 71178,},{"Revenge", 71434,},{"Brick Break", 71695,},{"Yawn", 71946,},{"Knock Off", 72212,},{"Endeavor", 72453,},{"Eruption", 72709,},{"Skill Swap", 72970,},{"Imprison", 73226,},{"Refresh", 73492,},{"Grudge", 73733,},{"Snatch", 73994,},{"Secret Power", 74260,},{"Dive", 74506,},{"Arm Thrust", 74772,},{"Camouflage", 75028,},{"Tail Glow", 75284,},{"Luster Purge", 75525,},{"Mist Ball", 75781,},{"Feather Dance", 76047,},{"Teeter Dance", 76308,},{"Blaze Kick", 76554,},{"Mud Sport", 76815,},{"Ice Ball", 77076,},{"Needle Arm", 77327,},{"Slack Off", 77578,},{"Hyper Voice", 77834,},{"Poison Fang", 78095,},{"Crush Claw", 78346,},{"Blast Burn", 78597,},{"Hydro Cannon", 78853,},{"Meteor Mash", 79114,},{"Astonish", 79375,},{"Weather Ball", 79626,},{"Aromatherapy", 79877,},{"Fake Tears", 80148,},{"Air Cutter", 80409,},{"Overheat", 80645,},{"Odor Sleuth", 80936,},{"Rock Tomb", 81167,},{"Silver Wind", 81413,},{"Metal Sound", 81704,},{"Grass Whistle", 81935,},{"Tickle", 82196,},{"Cosmic Power", 82452,},{"Water Spout", 82693,},{"Signal Beam", 82959,},{"Shadow Punch", 83220,},{"Extrasensory", 83476,},{"Sky Uppercut", 83727,},{"Sand Tomb", 83983,},{"Sheer Cold", 84229,},{"Muddy Water", 84490,},{"Bullet Seed", 84766,},{"Aerial Ace", 85012,},{"Icicle Spear", 85278,},{"Iron Defense", 85519,},{"Block", 85765,},{"Howl", 86056,},{"Dragon Claw", 86287,},{"Frenzy Plant", 86533,},{"Bulk Up", 86804,},{"Bounce", 87045,},{"Mud Shot", 87311,},{"Poison Tail", 87577,},{"Covet", 87833,},{"Volt Tackle", 88079,},{"Magical Leaf", 88340,},{"Water Sport", 88591,},{"Calm Mind", 88852,},{"Leaf Blade", 89103,},{"Dragon Dance", 89364,},{"Rock Blast", 89610,},{"Shock Wave", 89876,},{"Water Pulse", 90132,},{"Doom Desire", 90373,},{"Psycho Boost", 90629,},{"Roost", 90890,},{"Gravity", 91141,},{"Miracle Eye", 91432,},{"Wake-Up Slap", 91658,},{"Hammer Arm", 91914,},{"Gyro Ball", 92165,},{"Healing Wish", 92426,},{"Brine", 92682,},{"Natural Gift", 92943,},{"Feint", 93194,},{"Pluck", 93460,},{"Tailwind", 93711,},{"Acupressure", 93982,},{"Metal Burst", 94218,},{"U-turn", 94484,},{"Close Combat", 94725,},{"Payback", 94986,},{"Assurance", 95242,},{"Embargo", 95503,},{"Fling", 95754,},{"Psycho Shift", 96010,},{"Trump Card", 96261,},{"Heal Block", 96527,},{"Wring Out", 96773,},{"Power Trick", 97034,},{"Gastro Acid", 97290,},{"Lucky Chant", 97566,},{"Me First", 97812,},{"Copycat", 98068,},{"Power Swap", 98314,},{"Guard Swap", 98570,},{"Punishment", 98821,},{"Last Resort", 99077,},{"Worry Seed", 99338,},{"Sucker Punch", 99589,},{"Toxic Spikes", 99860,},{"Heart Swap", 100106,},{"Aqua Ring", 100372,},{"Magnet Rise", 100618,},{"Flare Blitz", 100879,},{"Force Palm", 101130,},{"Aura Sphere", 101396,},{"Rock Polish", 101652,},{"Poison Jab", 101908,},{"Dark Pulse", 102159,},{"Night Slash", 102415,},{"Aqua Tail", 102666,},{"Seed Bomb", 102927,},{"Air Slash", 103183,},{"X-Scissor", 103439,},{"Bug Buzz", 103690,},{"Dragon Pulse", 103946,},{"Dragon Rush", 104202,},{"Power Gem", 104468,},{"Drain Punch", 104714,},{"Vacuum Wave", 104990,},{"Focus Blast", 105221,},{"Energy Ball", 105482,},{"Brave Bird", 105743,},{"Earth Power", 105994,},{"Switcheroo", 106250,},{"Giga Impact", 106501,},{"Nasty Plot", 106772,},{"Bullet Punch", 107038,},{"Avalanche", 107274,},{"Ice Shard", 107550,},{"Shadow Claw", 107791,},{"Thunder Fang", 108047,},{"Ice Fang", 108303,},{"Fire Fang", 108559,},{"Shadow Sneak", 108830,},{"Mud Bomb", 109066,},{"Psycho Cut", 109332,},{"Zen Headbutt", 109583,},{"Mirror Shot", 109834,},{"Flash Cannon", 110090,},{"Rock Climb", 110356,},{"Defog", 110607,},{"Trick Room", 110853,},{"Draco Meteor", 111109,},{"Discharge", 111375,},{"Lava Plume", 111631,},{"Leaf Storm", 111877,},{"Power Whip", 112138,},{"Rock Wrecker", 112389,},{"Cross Poison", 112660,},{"Gunk Shot", 112901,},{"Iron Head", 113167,},{"Magnet Bomb", 113428,},{"Stone Edge", 113669,},{"Captivate", 113940,},{"Stealth Rock", 114196,},{"Grass Knot", 114452,},{"Chatter", 114708,},{"Judgment", 114954,},{"Bug Bite", 115220,},{"Charge Beam", 115466,},{"Wood Hammer", 115727,},{"Aqua Jet", 115988,},{"Attack Order", 116239,},{"Defend Order", 116490,},{"Heal Order", 116746,},{"Head Smash", 116997,},{"Double Hit", 117258,},{"Roar of Time", 117509,},{"Spacial Rend", 117765,},{"Lunar Dance", 118026,},{"Crush Grip", 118277,},{"Magma Storm", 118533,},{"Dark Void", 118794,},{"Seed Flare", 119045,},{"Ominous Wind", 119301,},{"Shadow Force", 119557,}  };
	int id = moveMap[moveName];
	int pp = moveMap[moveName];
	if(!id){
		cout << "Error: invalid Move name" << endl;
		exit(EXIT_FAILURE);
	}
	else{
		id = moveMap[moveName] >> 8;
		pp = moveMap[moveName] & 0xff;
	}

	// Update pokemon move for given slot
	if(block == 1){
		data[smallBlock1 + versionNames[version][leadPokemonOffset] + blockOffsets[1] + pokemon[moveset] + moveSlotOffset] = id & 0xff;
		data[smallBlock1 + versionNames[version][leadPokemonOffset] + blockOffsets[1] + pokemon[moveset] + moveSlotOffset + 1] = id >> 8;
		data[smallBlock1 + versionNames[version][leadPokemonOffset] + blockOffsets[1] + pokemon[movePP] + moveSlot] = pp;
		data[smallBlock1 + versionNames[version][leadPokemonOffset] + blockOffsets[1] + pokemon[movePP] + moveSlot + 1] = pp;
	}
	else if(block == 2){
		data[smallBlock2 + versionNames[version][leadPokemonOffset] + blockOffsets[1] + pokemon[moveset] + moveSlotOffset] = id & 0xff;
		data[smallBlock2 + versionNames[version][leadPokemonOffset] + blockOffsets[1] + pokemon[moveset] + moveSlotOffset + 1] = id >> 8;
		data[smallBlock2 + versionNames[version][leadPokemonOffset] + blockOffsets[1] + pokemon[movePP] + moveSlot] = pp;
		data[smallBlock2 + versionNames[version][leadPokemonOffset] + blockOffsets[1] + pokemon[movePP] + moveSlot + 1] = pp;
	}
	else{
		cout << "Error: could not edit pokemon" << endl;
		exit(EXIT_FAILURE);
	}
}

// Make lead pokemon shiny
void makePokemonShiny(vector<unsigned char>& data, vector<int> blockOffsets, int block, int version){

	int pv;
	if(block == 1){

		// Get the personality value for the pokemon
		pv = data[smallBlock1 + versionNames[version][leadPokemonOffset] + pokemon[personalityValueOffset]];
		pv += (data[smallBlock1 + versionNames[version][leadPokemonOffset] + pokemon[personalityValueOffset]+1] << 8);
		pv += (data[smallBlock1 + versionNames[version][leadPokemonOffset] + pokemon[personalityValueOffset]+2] << 16);
		pv += (data[smallBlock1 + versionNames[version][leadPokemonOffset] + pokemon[personalityValueOffset]+3] << 24);

		// Edit Pokemon OTID and SecretID to lower and upper bytes of pv
		data[smallBlock1 + versionNames[version][leadPokemonOffset] + blockOffsets[0] + pokemon[otid]] = pv;
		data[smallBlock1 + versionNames[version][leadPokemonOffset] + blockOffsets[0] + pokemon[otid]+1] = (pv >> 8) & 0xff;
		data[smallBlock1 + versionNames[version][leadPokemonOffset] + blockOffsets[0] + pokemon[otSecretID]] = (pv >> 16) & 0xff;
		data[smallBlock1 + versionNames[version][leadPokemonOffset] + blockOffsets[0] + pokemon[otSecretID]+1] = pv >> 24;

	}
	else if(block == 2){

		// Get the personality value for the pokemon
		pv = data[smallBlock2 + versionNames[version][leadPokemonOffset] + pokemon[personalityValueOffset]];
		pv += (data[smallBlock2 + versionNames[version][leadPokemonOffset] + pokemon[personalityValueOffset]+1] << 8);
		pv += (data[smallBlock2 + versionNames[version][leadPokemonOffset] + pokemon[personalityValueOffset]+2] << 16);
		pv += (data[smallBlock2 + versionNames[version][leadPokemonOffset] + pokemon[personalityValueOffset]+3] << 24);

		// Edit Pokemon OTID and SecretID to lower and upper bytes of pv
		data[smallBlock2 + versionNames[version][leadPokemonOffset] + blockOffsets[0] + pokemon[otid]] = pv;
		data[smallBlock2 + versionNames[version][leadPokemonOffset] + blockOffsets[0] + pokemon[otid]+1] = (pv >> 8) & 0xff;
		data[smallBlock2 + versionNames[version][leadPokemonOffset] + blockOffsets[0] + pokemon[otSecretID]] = (pv >> 16) & 0xff;
		data[smallBlock2 + versionNames[version][leadPokemonOffset] + blockOffsets[0] + pokemon[otSecretID]+1] = pv >> 24;

	}
	else{
		cout << "Error: could not edit pokemon" << endl;
		exit(EXIT_FAILURE);
	}

}

// Function that handles the encryption and calls specified pokemon edit function
void editPokemon(vector<unsigned char>& data, string pokemonName, string abilityName, string moveName, int moveSlot, vector<int> blockOffsets, int block, int version, int option){

    cout << block << endl;

	// Get the current pokemon checksum do decrypt the pokemon data block
	int curPokemonChecksum = getPokemonChecksum(data, block, version);

	// Decrypt pokemon data block
	prng(data, curPokemonChecksum, block, version);


	// Apply changes to pokemon data block
	switch(option){
		case 1:
			// Edit Pokemon Species
			editPokemonSpecies(data, pokemonName, blockOffsets, block, version);
			break;
		case 2:
			// Edit Pokemon Ability
			editPokemonAbility(data, abilityName, blockOffsets, block, version);
			break;
		case 3:
			// Edit Pokemon Name
			editPokemonMove(data, moveName, moveSlot, blockOffsets, block, version);
			break;
		case 4:
			// Make Pokemon Shiny
			makePokemonShiny(data, blockOffsets, block, version);
			break;
		default:
			cout << "Error: could not edit pokemon" << endl;
			exit(EXIT_FAILURE);
	}


	// Create chunk of data to calculate new pokemon checksum
	vector<unsigned char> chunk;
	if(block == 1){
		chunk = getSubVector(data, smallBlock1+versionNames[version][leadPokemonOffset]+0x08, smallBlock1+versionNames[version][leadPokemonOffset]+0x88);
		int newPokemonChecksum = calcPokemonChecksum(chunk);

		// Update pokemon checksum
		updatePokemonChecksum(data, newPokemonChecksum, block, version);

		// Encrypt pokemon data block
		prng(data, newPokemonChecksum, block, version);


		// Update save file checksum
		vector<unsigned char> tmp = getSubVector(data, smallBlock1, smallBlock1 + versionNames[version][smallBlockChecksumOffset]);
		int sum =  crc16ccitt(tmp);
		updateChecksum(data, sum, block, version);
	}
	else if (block == 2){
		chunk = getSubVector(data, smallBlock2+versionNames[version][leadPokemonOffset]+0x08, smallBlock2+versionNames[version][leadPokemonOffset]+0x88);
		int newPokemonChecksum = calcPokemonChecksum(chunk);

		// Update pokemon checksum
		updatePokemonChecksum(data, newPokemonChecksum, block, version);

		// Encrypt pokemon data block
		prng(data, newPokemonChecksum, block, version);


		// Update save file checksum
		vector<unsigned char> tmp = getSubVector(data, smallBlock2, smallBlock2 + versionNames[version][smallBlockChecksumOffset]);
		int sum =  crc16ccitt(tmp);
		updateChecksum(data, sum, block, version);
	}
	else{
		cout << "Error: could not update pokemon checksum" << endl;
		exit(EXIT_FAILURE);
	}
}


// - - - Menu Handling Functions - - - //

// Prints a menu to the console for user interaction
void printMenu(string title, vector<string> options){

	unsigned long len = title.size();

	for(unsigned long i = 0; i < (len + 8); i++){ cout << "-"; } cout << "\n    ";
	cout << title << endl;
	for(unsigned long i = 0; i < (len + 8); i++){ cout << "-"; } cout << "\n";


	for(unsigned long i = 1; i <= options.size(); i++){
		cout << char(i + 48) << ") " << options[i-1] << endl;
	}
	cout << "> ";
}

// Read user input and use it as an integer
int readInt(int* op){

	long toInt;
	char buffer[1024];

	if(fgets(buffer, 1024, stdin) != NULL){
		size_t len = strlen(buffer);

		// Clear stdin if more than 1024 characters are read
		if(len > 0 && buffer[len-1] != '\n'){
			int clear;
			while((clear = getchar()) != '\n' && clear != EOF);
		}
	}
	else{ printf("\n"); exit(1);}

	// Convert string input into long and ignore not digit characters
	char* end; errno = 0;
	toInt = strtol(buffer, &end, 10);


	// Check errors

	if(errno == ERANGE){ return 0; }

	if(end == buffer){ return 0; }

	if(*end && *end != '\n'){ return 0; }

	if(toInt > INT_MAX || toInt < INT_MIN){ return 0; }

	*op = (int)toInt;
	return 1;
}

// Main function parses command line arguments and lets the user select what they want to edit

int main(int argc, char *argv[]){

	if(argc != 3){
		cout << "Usage: ./saveditor [FileName] [VersionName]" << endl;
		cout << "Versions available: 'diamond', 'pearl', 'platinum', 'heartgold', 'soulsilver'" << endl;
		exit(EXIT_FAILURE);
	}

	// Attempt to read file from argv[1] and save it's contents to data
	vector<unsigned char> data;
	readFile(argv[1], data);

	// Make sure the provided version is valid
	string v = argv[2];
	int version = -1;
	if(v.compare("diamond") == 0){ version = diamond; }
	else if(v.compare("pearl") == 0){ version = pearl; }
	else if(v.compare("platinum") == 0){ version = platinum; }
	else if(v.compare("heartgold") == 0){ version = heartgold; }
	else if(v.compare("soulsilver") == 0){ version = soulsilver; }
	else{
		cout << "Error: version not found" << endl;
		cout << "Versions available: 'diamond', 'pearl', 'platinum', 'heartgold', 'soulsilver'" << endl;
		exit(EXIT_FAILURE);
	}

	// Find which block should be edited
	int block = getCurBlock(data, version);

	cout << block << endl;

	int pv = getPersonalityValue(data, block, version);
	vector<int> blockOffsets = getBlockOffsets(pv);

	string title = "Pokemon Savefile Editor";
	vector<string> optionsMain = {"Edit player", "Edit Pokemon", "Exit"};
	vector<string> optionsPlayer = {"Edit player Name", "Back"};
	vector<string> optionsPokemon = {"Edit Pokemon Species", "Edit Pokemon Ability", "Edit Pokemon Moves", "Make Pokemon Shiny", "Back"};



	while(true){
		printMenu(title, optionsMain);
		int n; if(!readInt(&n)){
			cout << "Error: invalid input" << endl;
			exit(EXIT_FAILURE);
		}

		switch(n){
			case 1:

				while(true){

					string newName;;
					bool flag = false;

					// Edit Player Menu
					printMenu(title, optionsPlayer);
					if(!readInt(&n)){
						cout << "Error: invalid input" << endl;
						exit(EXIT_FAILURE);
					}
					switch(n){
						case 1:
							cout << "Enter new name > ";
							getline(cin, newName);
							changePlayerName(data, newName, block, version);
							writeFile(argv[1], data);
							break;
						case 2:
							flag = true;
							break;
						default:
							cout << "Invalid Option!" << endl;
					}
					if(flag) { break; };
				}
				break;
			case 2:

				while(true){

					string change;
					int moveSlot;
					bool flag = false;

					// Edit Pokemon Menu
					printMenu(title, optionsPokemon);
					if(!readInt(&n)){
						cout << "Error: invalid input" << endl;
						exit(EXIT_FAILURE);
					}
					switch(n){
						case 1:
							cout << "Enter species name (Example: Pikachu) > ";
							getline(cin, change);
							editPokemon(data, change, "", "", 0, blockOffsets, block, version, 1);
							writeFile(argv[1], data);
							break;
						case 2:
							cout << "Enter ability name (Example: Static) > ";
							getline(cin, change);
							editPokemon(data, "", change, "", 0, blockOffsets, block, version, 2);
							writeFile(argv[1], data);
							break;
						case 3:
							cout << "Enter move name (Example: Volt Tackle) > ";
							getline(cin, change);
							cout << "Enter move slot [1-4] > ";
							readInt(&moveSlot);
							editPokemon(data, "", "", change, moveSlot, blockOffsets, block, version, 3);
							writeFile(argv[1], data);
							break;
						case 4:
							editPokemon(data, "", "", "", 0, blockOffsets, block, version, 4);
							writeFile(argv[1], data);
							break;
						case 5:
							flag = true;
							break;
						default:
							cout << "Invalid Option!" << endl;

					}
					if(flag){break;}
				}
				break;
			case 3:
				// Exit
				exit(EXIT_SUCCESS);
			default:
				cout << "Invalid Option!" << endl;
		}
	}
	return 0;
}
