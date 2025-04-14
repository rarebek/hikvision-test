#include "base64.hpp"


std::string base64_encode(const char* data, int size, bool url_encode) {
	const char index_table1[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	const char index_table2[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
	const char* index_table = url_encode ? index_table2 : index_table1;

	std::string encoded_text;
	int encoded_bytes = 0;

	//    encode each 3 bytes by 6bit index's character
	for( ; encoded_bytes + 3 <= size; encoded_bytes += 3 ) {
		unsigned char byte1 = (unsigned char) data[encoded_bytes + 0];
		unsigned char byte2 = (unsigned char) data[encoded_bytes + 1];
		unsigned char byte3 = (unsigned char) data[encoded_bytes + 2];
		int index1 = (byte1 >> 2) & 0x3F;
		int index2 = (byte1 << 4 | byte2 >> 4) & 0x3F;
		int index3 = (byte2 << 2 | byte3 >> 6) & 0x3F;
		int index4 = (byte3) & 0x3F;
		encoded_text += index_table[index1];
		encoded_text += index_table[index2];
		encoded_text += index_table[index3];
		encoded_text += index_table[index4];
	}

	// add padding
	if( (size - encoded_bytes) == 1 ) {
		unsigned char byte1 = (unsigned char) data[encoded_bytes + 0];
		int index1 = (byte1 >> 2) & 0x3F;
		int index2 = (byte1 << 4) & 0x3F;
		encoded_text += index_table[index1];
		encoded_text += index_table[index2];
		encoded_text += "==";
	}
	else if( (size - encoded_bytes) == 2 ) {
		unsigned char byte1 = (unsigned char) data[encoded_bytes + 0];
		unsigned char byte2 = (unsigned char) data[encoded_bytes + 1];
		unsigned char byte3 = (unsigned char) 0;
		int index1 = (byte1 >> 2) & 0x3F;
		int index2 = (byte1 << 4 | byte2 >> 4) & 0x3F;
		int index3 = (byte2 << 2) & 0x3F;
		encoded_text += index_table[index1];
		encoded_text += index_table[index2];
		encoded_text += index_table[index3];
		encoded_text += "=";
	}

	return encoded_text;
}

std::vector<char> base64_decode(const std::string& text) {
	char lut[128];
	std::memset(lut, -1, 128);
	for( int i = 'A'; i <= 'Z'; ++i )
		lut[i] = static_cast<char>(i - 'A');
	for( int i = 'a'; i <= 'z'; ++i )
		lut[i] = static_cast<char>(26 + i - 'a');
	for( int i = '0'; i <= '9'; ++i )
		lut[i] = static_cast<char>(52 + i - '0');
	lut['+'] = lut['-'] = static_cast<char>(62);
	lut['/'] = lut['_'] = static_cast<char>(63);
	
	std::vector<char> decoded_data;
	int decoded_chars = 0;

	//    decode each 4 character to 3 byte
	for( ; decoded_chars + 4 <= text.length(); ) {
		char index1 = lut[text[decoded_chars + 0] & 0x7F]; // 6bit
		char index2 = lut[text[decoded_chars + 1] & 0x7F]; // 6bit
		char index3 = lut[text[decoded_chars + 2] & 0x7F]; // 6bit
		char index4 = lut[text[decoded_chars + 3] & 0x7F]; // 6bit
		
		if (index1 == -1) {
			decoded_chars++;
			continue;
		}
		
		char byte1 = index1 << 2 | index2 >> 4;
		char byte2 = index2 << 4 | index3 >> 2;
		char byte3 = index3 << 6 | index4;
		
		decoded_data.push_back(byte1);
		if( text[decoded_chars + 2] == '=' ) break;
		decoded_data.push_back(byte2);
		if( text[decoded_chars + 3] == '=' ) break;
		decoded_data.push_back(byte3);
		
		decoded_chars += 4;
	}
	return decoded_data;
}
