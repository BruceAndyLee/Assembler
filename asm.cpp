#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <iostream>
#include <cerrno>
#include <climits>

/*
ASSEMBLER BY ARBUCE LEE V.1 YYY 1897
*/


enum ARGS_TYPES
{
	TYPE_RGSR,
	TYPE_CNST,
	TYPE_ADDR,
	TYPE_LABL
};

enum WORK_CONSTANTS
{
	DEF_STRS_NUM		= 10,
	DEF_BIN_STR_LEN		= 128,
	MAX_CMD_NAME_LEN	= 10,
	MAX_LBL_NAME_LEN	= 20
};

int strtoint(const char* str)
{
    char *endptr;
    int val;

	if (!str)
	{
		printf("strtoint: invalid string was received\n");
		exit(EXIT_FAILURE);
	}

    errno = 0;    /* To distinguish success/failure after call */
    val = (int) strtol(str, &endptr, 10);

    if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN))
                                             || (errno != 0 && val == 0)) {
        perror("strtol");
        exit(EXIT_FAILURE);
    }

    if (endptr == str) {
        fprintf(stderr, "No digits were found\n");
        exit(EXIT_FAILURE);
    }
	return val;
}

char** extend(char** buf, size_t prev_size, size_t msize)
{
	if (!buf) return NULL;
	if (!msize) 
	{
		printf("extend: got invalid size: %lu\n", msize);
		return NULL;
	}
	char** new_buf = (char**) calloc (msize, sizeof(char*));
	for (size_t i = 0; i < prev_size; i++)
		new_buf[i] = buf[i];
	free(buf);
	buf = NULL;
	return new_buf;
}


size_t getflen(FILE* f)
{
	if (!f)
	{
		printf("flen: error: invalid descriptor pointer\n");
		return 1;
	}
	long cur = ftell(f);
	size_t len = 0;
	fseek(f, 0, SEEK_END);
	len = (size_t) ftell(f);
	fseek(f, cur, SEEK_SET);
	return len;
}

char** splitstr(char* str, const char* token)
{
	if (!str) return NULL;

	size_t i			= 0;
	size_t str_counter  = DEF_STRS_NUM;
	char** strs			= (char**) calloc (str_counter, sizeof(char*));
	if (!strs)
	{
		printf("splitstr: error finding memory for head\n");
		exit(EXIT_FAILURE);
	}
	char* interim_str = NULL;
	for (i = 0; ; i++, str = NULL)
	{
		if (i == (str_counter - 1))
		{
			if ( !( strs = extend(strs, str_counter, str_counter * 2) ) )
			{
				printf("splistr: error extending buffer\n");
				exit(EXIT_FAILURE);
			}
			str_counter *= 2;
		}
		interim_str = strtok (str, token);

		if ((strs[i] = interim_str) == NULL)
			break;
			
		strs[i] = strndup(interim_str, strlen(interim_str));
	}
	/*if ( !(strs = (char**) realloc (strs, (i + 1) * sizeof(char*))) ) //to get rid of the unused allocated tail
	{
		printf("splitstr: realloc: error: %s(%d)\n", strerror(errno), errno);
		exit(EXIT_FAILURE);
	}*/
	//strs[i] = (char*) NULL; //terminating strings array with zero pointer
	return strs;
}

typedef struct _Label
{
	char*	name;
	size_t	pc;
} Label;

class Asm
{
public:
	Asm(char* str);
	~Asm();

	int process_code();

private:
	int		find_labels();
	int		erase_commentary();
	int		check_cmd();
	int		check_arguments();
	int		parse_arguments();
	void	sprint_str();
	void	fprint_str();
	size_t  get_label_pc(const char* name);

	char**	asm_code_;
	Label*  labels_;
	char**	cur_str_;
	char*	cur_cmd_;
	int		cur_cmd_num_;
	int		argsnum_;
	int		type1_;
	int		type2_;
	int		arg1_;
	int		arg2_;
	size_t	ip_;
	size_t  line_;
	size_t	bin_bias_;

	FILE*	bin_f_;
};

Asm::Asm(char* asm_str):
asm_code_	(NULL),
labels_		(NULL),
cur_str_	(NULL),
cur_cmd_	(NULL),
cur_cmd_num_(0),
argsnum_	(0),
type1_		(0),
type2_		(0),
arg1_		(0),
arg2_		(0),
ip_			(0),
line_		(0),
bin_bias_	(0),
bin_f_		(fopen("bin.asm", "wb"))
{
	if (!bin_f_)
	{
		printf ("Asm: error opening/creating o-file\n");
		exit(EXIT_FAILURE);
	}
	size_t lines = 0;
	if (!asm_str)
	{
		printf("Asm: c_tor: error: got invalid asm string\n");
		exit(EXIT_FAILURE);
	}
	asm_code_ = splitstr(asm_str, "\n");
	while (asm_code_[lines++]);
	

	if ( !( labels_ = (Label*) calloc (lines + 1, sizeof(Label)) ) )
	{
		printf("Asm: error finding memory to keep labels\n");
		exit(EXIT_FAILURE);
	}
	
	for (size_t i = 0; i < lines; i++)
	{
		if ( !(labels_[i].name = (char*) calloc (MAX_LBL_NAME_LEN, sizeof(char))) )
		{
			printf("Asm: error finding memory to keep labels array\n");
			exit(EXIT_FAILURE);
		}
	}

	if ( !(cur_cmd_ = (char*) calloc (MAX_CMD_NAME_LEN, sizeof(char))) )
	{
		printf("Asm: error finding memory to keep for cmd name\n");
		exit(EXIT_FAILURE);
	}
}

Asm::~Asm()
{
	int i = 0;
	while (asm_code_[i])
	{
		free(asm_code_[i]);
		asm_code_[i] = NULL;
	}
	free(asm_code_);
	asm_code_ = NULL;
	i = 0;
	free(labels_);
	labels_ = NULL;
}

size_t Asm::get_label_pc(const char* name)
{
	size_t i = 0;
	for (i = 0; strcmp(labels_[i].name, name); i++);
	return labels_[i].pc;
}

int Asm::find_labels()
{
	for (size_t i = 0; asm_code_[i]; i++)
		if (strchr(asm_code_[i], ':'))
		{
			sscanf(asm_code_[i], " %[A-Za-z]:", labels_[i].name); //variable i can be interpreted as the cmd number that label points at
			sprintf(asm_code_[i], "\n");//process_code thus will know that cur str is invalid to parse a cmd from
			printf("asm: find_labels(): got label '%s': [%lu]\n", labels_[i].name, i);
			labels_[i].pc = i;
		}
	return 0;
}

int Asm::erase_commentary()
{
	if (cur_str_[0][0] == ';')
		return 1;
	
	int i = 0;
	while (cur_str_[i] && cur_str_[i++][0] != ';');

	for (int j = i; cur_str_[j]; j++)
	{
		free(cur_str_[j]);
		cur_str_[j] = NULL;
	}
	return 0;
}

int Asm::check_cmd()
{	
#define _CPU_CMDS_

#define CPU_CMD(funcName, argsNum, funcNum)		\
    if(!strcmp(cur_str_[0], #funcName))			\
	{											\
		argsnum_ = argsNum;						\
        return funcNum;							\
	}
#include "supercmd.txt"

#undef CPU_CMD
#undef _CPU_CMDS_
	
    return -1;
}

int Asm::check_arguments()
{
	return 0;
}

int Asm::parse_arguments()
{
	if (argsnum_ == 0)
		return 0;

	if (!cur_str_[1])
		return 1;
	else
	{
		if		(cur_str_[1][0] == '%')		type1_ = TYPE_RGSR; //means register-number to be used
		else if (isdigit(cur_str_[1][0]))	type1_ = TYPE_CNST; //means constant type
		else if (cur_str_[1][0] == '$')		type1_ = TYPE_ADDR; //means using address
		else if (isalpha(cur_str_[1][0]) || 
				 (cur_str_[1][0] == '.') )	type1_ = TYPE_LABL;
		else return 1;
	}
	switch (type1_)
	{
		case TYPE_RGSR:
			arg1_ = strtoint(&cur_str_[1][1]); // starting off of the char that goes after '%'
			break;
		case TYPE_CNST:
			arg1_ = strtoint(cur_str_[1]);
			break;
		case TYPE_ADDR:
			if (isdigit(cur_str_[1][1]))
			{
				arg1_ = strtoint(&cur_str_[1][1]);
				break;
			}
			if (cur_str_[1][1] == '%')
			{
				arg1_ = strtoint(&cur_str_[1][2]);
				break;
			}
		case TYPE_LABL:
			arg1_  = (int) get_label_pc(cur_str_[1]);
			type1_ = TYPE_ADDR;
			break;
		default:
			printf("unknown specifier '%c' for command %s at line %lu\n", cur_str_[1][0], cur_str_[0], line_ + 1);
			exit(EXIT_FAILURE);
	}

	if (argsnum_ == 1)
		return 0;

	if (!cur_str_[2])
		return 1;
	else
	{
		if		(cur_str_[2][0] == '%')		type2_ = TYPE_RGSR;
		else if (isdigit(cur_str_[2][0]))	type2_ = TYPE_CNST;
		else if (cur_str_[2][0] == '$')		type2_ = TYPE_ADDR;
		else if (isalpha(cur_str_[2][0]) || 
				 cur_str_[2][0] == '.' )	type2_ = TYPE_LABL;
		else return 1;
	}


	switch (type2_)
	{
		case '%':
			arg2_ = strtoint(&cur_str_[2][1]); // starting off the char that goes after '%'
			break;
		case TYPE_CNST:
			arg2_ = strtoint(cur_str_[2]);
			break;
		case '$':
			if (isdigit(cur_str_[2][1]))
			{
				arg2_ = strtoint(&cur_str_[2][1]);
				break;
			}
			if (cur_str_[2][1] == '%')
			{
				arg2_ = strtoint(&cur_str_[2][2]);
				break;
			}
		case TYPE_LABL:
			arg2_  = (int) get_label_pc(cur_str_[2]);
			type2_ = TYPE_ADDR;
			break;
		default:
			printf("unknown specifier '%c' for command %s at line %lu\n", cur_str_[2][0], cur_str_[0], line_);
			exit(EXIT_FAILURE);
	}

	return 0;
}

void Asm::fprint_str()
{
	fprintf(bin_f_, "%d", cur_cmd_num_);
	if (type1_ != -1)
		fprintf(bin_f_, "%d%d", type1_, arg1_);
	
	if (type2_ != -1)
		fprintf(bin_f_, "%d%d", type2_, arg2_);
}

int Asm::process_code()
{
	find_labels(); // receiving all label valuing numbers and deliting extra strings from asm code str
	ip_ = 0;
	while (asm_code_[line_])
	{
		printf("processing %lu line\n", line_);
		cur_cmd_num_ = -1;
		type1_ = -1;
		type2_ = -1; 
		arg1_  = -1;
		arg2_  = -1;

		if (asm_code_[line_][0] == ';' || asm_code_[line_][0] == '\n')//skipping str that used to contain label or commentary
		{
			line_++;
			continue;
		}

		cur_str_ = splitstr(asm_code_[line_], " ");

		if (!cur_str_)
		{
			printf("process_code: error splitting str %lu\n", line_);
			exit(EXIT_FAILURE);
		}

		if ((cur_cmd_num_ = check_cmd()) < 0)
		{
			printf("Asm: error: unknown command '%s' line %lu\n", cur_str_[0], line_ + 1);
			exit(EXIT_FAILURE);
		}
		
		if (check_arguments())
		{
			printf("Asm: error: invalid arguments for command '%s' line %lu\n", cur_str_[0], line_ + 1);
			exit(EXIT_FAILURE);
		}
			
		if (parse_arguments())
		{
			printf("Asm: error: cannot parse arguments for command '%s' at line %lu\n", cur_str_[0], line_ + 1);
			exit(EXIT_FAILURE);
		}
		printf("cmd: '%s'; arg1_ %d; arg2_ %d\n", cur_str_[0], arg1_, arg2_);
		fprint_str();	

		for (int i = 0; cur_str_[i]; i++)
		{
			free(cur_str_[i]);
			cur_str_[i] = NULL;
		}

		printf("\n");
		free(cur_str_);
		ip_++;
		line_++;
	}
	return 0;
}

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		printf("Usage: %s [file.asm]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	FILE* fasm = NULL;
	if (!(fasm = fopen(argv[1], "r")))
	{
		printf("ASM: FOPEN: ERROR: %s(%d)\n", strerror(errno), errno);
		exit(EXIT_FAILURE);
	}
	size_t fasmlen = getflen(fasm);
	char* asm_str = (char*) calloc (fasmlen + 1, sizeof(char));

	/* receiving a string containing file*/
	size_t read_chars = 0;
	if ((read_chars = fread(asm_str, sizeof(char), fasmlen, fasm)) != fasmlen)
		printf("fread: %lu chars read, %lu char expected\n", read_chars, fasmlen);

	/*Asm is now in charge of making all of the stuff*/
	Asm my_ass(asm_str);

	my_ass.process_code();
	
	fclose(fasm);
	exit(EXIT_SUCCESS);
}

