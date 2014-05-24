#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include "MIDIKeyChange.h"
	
FILE *in, *out;
int shift[128];

const int MAJOR = 0;
const int MINOR = 1;

using namespace std;

void           compute_key_change(char* key);
void           parse_key(char* key, int* tonic, int* tonality);
void           handle_status(char status);
vector<string> &split(const string &s, char delim, vector<string> &elems);
vector<string> split(const string &s, char delim);
void           copy_string(char expected[]);
void           copy_int(int expected);
int            copy_next_byte();
int            copy_next_short_int();
int            copy_next_int();
int            next_int();
short int      next_short_int();
char           next_byte();
int            copy_next_var_len_int();
int            is_note_status(unsigned char status);
void           write_int(int x);

int main(int argc, char** args)
{
	struct stat buffer;
	int         status;

	compute_key_change(args[2]);
	
	string file_path = args[1];
	status = stat(args[1], &buffer);	
	in = fopen(args[1], "rb"); 
	int ext_index = (file_path).find_last_of("."); 
	string output_file_path = (file_path).substr(0, ext_index); 
	output_file_path += ".transposed.mid";
	out = fopen(output_file_path.c_str(), "wb");
	copy_string("MThd");
	copy_int(6);

	int format      = copy_next_short_int();
	int n_tracks    = copy_next_short_int();
	int division    = copy_next_short_int();

	int track, MTrk_length;

	for (track = 0; track < n_tracks; track++)
	{
		copy_string("MTrk");
		MTrk_length = copy_next_int();
		int time;
		unsigned char status;
		fpos_t p, start;
		fgetpos(in, &start);

		for (p = start; p - start < MTrk_length; fgetpos(in, &p))
		{
			time = copy_next_var_len_int();
			status = copy_next_byte();
			handle_status(status);
		}
	}
	
	cout << "Transposed file output to " << output_file_path;
	
	fclose(in);
	fclose(out);
}

void compute_key_change(char* key)
{
	int new_scale[12];
	int tonic = 0, tonality = 5;

	parse_key(key, &tonic, &tonality);

	if (tonic == -1 || tonality == -1)
		printf("Error: improperly formatted key input\n");


	new_scale[0] = 0;
	new_scale[1] = 0;
	new_scale[2] = 0;
	new_scale[3] = (tonality == MINOR)?  1 : 0;
	new_scale[4] = (tonality == MAJOR)? -1 : 0;
	new_scale[5] = 0;
	new_scale[6] = 0;
	new_scale[7] = 0;
	new_scale[8] = (tonality == MINOR)?   1 : 0;
	new_scale[9] = (tonality == MAJOR)?  -1 : 0;
	new_scale[10] = (tonality == MINOR)?  1 : 0;
	new_scale[11] = (tonality == MAJOR)? -1 : 0;

	int i;

	printf("\n");
	for (i = 0; i < 128; i++)
	{
		shift[i] = new_scale[(i-tonic+12) % 12];
	}	
}

void parse_key(char* key, int* tonic, int* tonality)
{
	switch (key[0])
	{
		case 'C':
		case 'c':
			*tonic = 0;
			break;
		case 'D':
		case 'd':
			*tonic = 2;
			break;
		case 'E':
		case 'e':
			*tonic = 4;
			break;
		case 'F':
		case 'f':
			*tonic = 5;
			break;
		case 'G':
		case 'g':
			*tonic = 7;
			break;
		case 'A':
		case 'a':	
			*tonic = 9;
			break;
		case 'B':
		case 'b':
			*tonic = 11;
			break;
		delault:
			*tonic = -1;
	}


	if (key[1] == 'b')
		*tonic = (*tonic != 0)? *tonic-1 : 11;
	else if (key[1] == '#')
		*tonic = (*tonic != 11)? *tonic+1 : 0;
	else if (key[1] != '\0' && key[1] != 'm')
		*tonic = -1;

	*tonality = 
		(key[1] == '\0')? MAJOR : 
		(key[1] == 'm')?  MINOR :
		(key[2] == '\0')? MAJOR :
		(key[2] == 'm')?  MINOR : 
		-1;
}

void handle_status(char status)
{
	unsigned int stat_len, type;
	unsigned char data1, data2;

	if ((unsigned char)status == 0xFF) 
	{
		type = copy_next_byte();
		stat_len = copy_next_var_len_int();
		for (; stat_len-- > 0;)
			copy_next_byte();
	}
	else if ((status & 0xE0) == 0xC0)
	{
		data1 = copy_next_byte();
	}
	else if (is_note_status(status))
	{ 
		data1  = next_byte();
		data1 += shift[data1];
		fputc(data1, out);
		data2 = copy_next_byte();
	}
	else
	{
		data1 = copy_next_byte();
		data2 = copy_next_byte();
	}
}

// from http://stackoverflow.com/questions/236129/how-to-split-a-string-in-c
vector<string> &split(const string &s, char delim, vector<string> &elems) {
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
} 

// from http://stackoverflow.com/questions/236129/how-to-split-a-string-in-c
vector<string> split(const string &s, char delim) {
    vector<string> elems;
    split(s, delim, elems);
    return elems;
}
void copy_string(char* expected)
{
	int i;
	unsigned char next;

	for (i = 0; expected[i] != 0; i++) 
	{
		next = fgetc(in);
		if (next != expected[i])
		{
			printf("Error: Improperly formatted .mid file");
			fputc('x', out);
			exit(EXIT_FAILURE);
		}
	}

	fputs(expected, out);
}

void copy_int(int expected)
{
	int next = next_int();
	if (next != expected)
	{
		printf("Error: Improperly formatted .mid file.\n");
		exit(EXIT_FAILURE);
	}

	write_int(expected);
}

int next_int()
{
	char* new_int = (char*) malloc(4*sizeof(char));
	
	int i = 0;
	for (i = 0; i < 4; i++)
		new_int[3-i] = fgetc(in);

	return *((int*) new_int);
}


short int next_short_int()
{
	char* new_short_int = (char*) malloc(2*sizeof(char));
	
	new_short_int[1] = fgetc(in);
	new_short_int[0] = fgetc(in);

	return *((short int*) new_short_int);
}

char next_byte()
{
	return fgetc(in);
}

int copy_next_var_len_int()
{
	int i = 0, bytes = 0;
	unsigned char val[4], c;
	memset(val, 0, 4);
	
	do 
	{
		c = fgetc(in);
		fputc(c, out);
		bytes++;
		val[i++] = c;
	} 
	while(c & 0x80);

	for (i = 0; i < bytes; i++)
	{
		val[3-i] = val[bytes-i-1];
		val[bytes-i-1] = 0;
	}

	val[3] = val[3] | (val[2] << 7);
	val[2] = ((val[2] >> 1) & 0x3F) | (val[1] << 6);
	val[1] = ((val[1] >> 2) & 0x1F) | (val[0] << 5);
	val[0] = (val[0] >> 3) & 0x0F;
	
	char temp0 = val[0];
	char temp1 = val[1];
	val[0] = val[3];
	val[1] = val[2];
	val[2] = temp1;
	val[3] = temp0;

	return *((int*) val);
}

int is_note_status(unsigned char status)
{
	return (status & 0xE0) == 0x80;
}

void write_int(int x)
{	
	char* write_x = (char*) &x;

	fputc(write_x[3], out);
	fputc(write_x[2], out);
	fputc(write_x[1], out);
	fputc(write_x[0], out);
}

void write_short_int(short int x)
{	
	char* write_x = (char*) &x;

	fputc(write_x[1], out);
	fputc(write_x[0], out);
}

int copy_next_byte()
{	
	char next = fgetc(in);
	fputc(next, out);
	return next;
}

int copy_next_int()
{	
	int next = next_int();
	write_int(next);
	return next;
}


int copy_next_short_int()
{	
	int next = next_short_int();
	write_short_int(next);
	return next;
}
