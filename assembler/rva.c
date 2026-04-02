/* RVASM - RISC-V assembler  Alpha- version  J. Garside  UofM 13/3/25         */
// To do(inherited):
//		Proper shakedown testing (improving)
//		Macros
//		Conditional assembly
//		"mnemonics" file not found if executed via $PATH; improve search
//		'record'/'structure' directive for creating offsets
// 16/6/23  Couple of bug fixes: x10 & store offsets
// 7/4/24   Started on load/store global pseudo-ops.
// 7/2/25   Bug fix on JALR vs. extended load store addressing mode
// 8/2/25   Optional ':' after labels
// 13/3/25  Shortened LI (=> LUI only) form spliced in

#include <stdio.h>
#include <string.h>                           /* For {strcat, strlen, strcpy} */
#include <stdlib.h>                                     /* For {malloc, exit} */

#define TRUE               (0 == 0)
#define FALSE              (0 != 0)

#define MAX_PASSES               20    /* No of reiterations before giving up */
#define SHRINK_STOP  (MAX_PASSES-10)  /* First pass where shrinkage forbidden */

#define VERILOG_MAX         0x10000     /* Maximum size of Verilog ROM output */

#define IF_STACK_SIZE            10          /* Maximum nesting of IF clauses */

#define SYM_TAB_HASH_BITS         4
#define SYM_TAB_LIST_COUNT       (1 << SYM_TAB_HASH_BITS)
#define SYM_TAB_LIST_MASK        (SYM_TAB_LIST_COUNT - 1)

#define SYM_NAME_MAX             32
#define LINE_LENGTH             256

#define SYM_TAB_CASE_FLAG         1     /* Bit mask for case insensitive flag */
#define SYM_TAB_EXPORT_FLAG       2                       /* Keep whole table */

#define SYM_REC_DEF_FLAG     0x0100     /* Bit mask for `symbol defined' flag */
#define SYM_REC_EXPORT_FLAG  0x0200             /* Bit mask for `export' flag */
#define SYM_REC_EQU_FLAG     0x0400           /* Indicate `type' as EQU (abs) */
#define SYM_REC_USR_FLAG     0x0800           /* Indicate `type' as DEF (abs) */
#define SYM_REC_EQUATED     (SYM_REC_EQU_FLAG | SYM_REC_USR_FLAG)   /* Either */
#define SYM_REC_USR_FLAG     0x0800           /* Indicate `type' as DEF (abs) */

#define SYM_REC_THUMB_FLAG   0x1000         /* Indicate label in `Thumb' area */
                                         /* Lowest 8 bits used for pass count */
#define SYM_REC_DATA_FLAG    0x2000                      /* Data space offset */

#define ALLOW_ON_FIRST_PASS   0x00010000       /* Bit masks to prevent errors */
#define ALLOW_ON_INTER_PASS   0x00020000       /*  occurring when not wanted. */
#define WARNING_ONLY          0x00040000
#define ALL_EXCEPT_LAST_PASS (ALLOW_ON_FIRST_PASS | ALLOW_ON_INTER_PASS)

#define SYM_NO_ERROR               0
#define SYM_ERR_SYNTAX        0x0100
#define SYM_ERR_NO_MNEM       0x0200
#define SYM_ERR_NO_EQU        0x0300
#define SYM_BAD_REG           0x0400
//#define SYM_BAD_REG_COMB    0x0500
//#define SYM_NO_REGLIST      0x0600  // Was ARM
#define SYM_NO_COMMA_LBR      0x0600
//#define SYM_NO_RSQUIGGLE    0x0700
#define SYM_OORANGE          (0x0800 | ALL_EXCEPT_LAST_PASS)
#define SYM_ENDLESS_STRING    0x0900
#define SYM_DEF_TWICE         0x0A00
#define SYM_NO_COMMA          0x0B00
#define SYM_NO_TABLE          0x0C00
#define SYM_GARBAGE           0x0D00
#define SYM_ERR_NO_EXPORT    (0x0E00 | WARNING_ONLY)
#define SYM_INCONSISTENT      0x0F00
#define SYM_ERR_NO_FILENAME   0x1000
#define SYM_NO_LBR            0x1100
#define SYM_NO_RBR            0x1200
#define SYM_ADDR_MODE_ERR     0x1300
//#define SYM_ADDR_MODE_BAD   0x1400
//#define SYM_NO_LSQUIGGLE    0x1500
#define SYM_OFFSET_TOO_BIG   (0x1600 | ALL_EXCEPT_LAST_PASS)
//#define SYM_BAD_COPRO       0x1700
#define SYM_BAD_VARIANT       0x1800
//#define SYM_NO_COND         0x1900
//#define SYM_BAD_CP_OP       0x1A00
#define SYM_NO_LABELS        (0x1B00 | WARNING_ONLY)
#define SYM_DOUBLE_ENTRY      0x1C00
#define SYM_NO_INCLUDE        0x1D00
//#define SYM_NO_BANG         0x1E00
#define SYM_MISALIGNED       (0x1F00 | ALL_EXCEPT_LAST_PASS)
#define SYM_OORANGE_BRANCH   (0x2000 | ALL_EXCEPT_LAST_PASS)
#define SYM_UNALIGNED_BRANCH (0x2100 | ALL_EXCEPT_LAST_PASS)
#define SYM_VAR_INCONSISTENT  0x2200
#define SYM_NO_IDENTIFIER     0x2300
#define SYM_MANY_IFS          0x2400
#define SYM_MANY_FIS          0x2500
#define SYM_LOST_ELSE         0x2600
#define SYM_NO_HASH           0x2700
#define SYM_NO_IMPORT         0x2800
#define SYM_ADRL_PC           0x2900
#define SYM_ERR_NO_SHFT       0x2A00
#define SYM_NO_REG_HASH       0x2B00
#define SYM_ERR_BROKEN        0xFF00                  /* TEMP uncommitted @@@ */

/* evaluate return error states */
/*
#define eval_okay             0x0000		// Rationalise @@@
*/
#define eval_okay             SYM_NO_ERROR
#define eval_no_operand       0x3000
#define eval_no_operator      0x3100
#define eval_not_closebr      0x3200
#define eval_not_openbr       0x3300
#define eval_mathstack_limit  0x3400
#define eval_no_label        (0x3500 | ALLOW_ON_FIRST_PASS)
#define eval_label_undef     (0x3600 | ALL_EXCEPT_LAST_PASS)
#define eval_out_of_radix     0x3700
#define eval_div_by_zero     (0x3800 | ALL_EXCEPT_LAST_PASS)
#define eval_operand_error    0x3900
#define eval_bad_loc_lab      0x3A00
#define eval_no_label_yet     0x3B00 /* Label not defined `above' (for `IF's)*/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Expression evaluator                                                       */

#define MATHSTACK_SIZE 20
                     /* Enumerations used for both unary and binary operators */
#define PLUS                 0
#define MINUS                1
#define NOT                  2
#define MULTIPLY             3
#define DIVIDE               4
#define MODULUS              5
#define CLOSEBR              6
#define LEFT_SHIFT           7
#define RIGHT_SHIFT          8
#define AND                  9
#define OR                  10
#define XOR                 11
#define EQUALS              12
#define NOT_EQUAL           13
#define LOWER_THAN          14                        /* Unsigned comparisons */
#define LOWER_EQUAL         15
#define HIGHER_THAN         16
#define HIGHER_EQUAL        17
#define LESS_THAN           18                          /* Signed comparisons */
#define LESS_EQUAL          19
#define GREATER_THAN        20
#define GREATER_EQUAL       21
#define LOG                 22
#define END                 23

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* List file formatting constants                                             */
#define LIST_LINE_LENGTH   120                           /* Total line length */
#define LIST_LINE_ADDRESS   10                         /* Address field width */
#define LIST_BYTE_COUNT      4                    /* Number of bytes per line */
#define LIST_BYTE_FIELD   (LIST_LINE_ADDRESS + 3 * LIST_BYTE_COUNT + 2)
#define LIST_LINE_LIST    (LIST_LINE_LENGTH  - 1 - LIST_BYTE_FIELD)

#define HEX_LINE_ADDRESS    10
#define HEX_BYTE_COUNT      16
#define HEX_LINE_LENGTH    (HEX_LINE_ADDRESS + 3 * HEX_BYTE_COUNT)

#define ELF_TEMP_LENGTH     20

#define ELF_MACHINE       0xF3                                      /* RISC-V */
#define ELF_EHSIZE          52                         /* Defined in standard */
#define ELF_PHENTSIZE      (4 * 8)                     /* Defined in standard */
#define ELF_SHENTSIZE      (4 * 10)                    /* Defined in standard */

#define ELF_SHN_ABS        0xFFF1                      /* Defined in standard */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#define	v3	0xFF		// Want to add RISC-V extension identifiers
#define	v3M	0xFE
#define	v4	0xFC
#define	v4xM	0xFD
#define	v4T	0xF8
#define	v4TxM	0xF9
#define	v5	0xF0
#define	v5xM	0xF1
#define	v5T	0xF0
#define	v5TxM	0xF1
#define	v5TE	0xC0
#define	v5TExP	0xE0

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

typedef int boolean;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

typedef enum { TYPE_BYTE, TYPE_HALF, TYPE_WORD, TYPE_CPRO }  type_size;
typedef enum { NO_LABEL, MAYBE_SYMBOL, SYMBOL, LOCAL_LABEL } label_type;
typedef enum { ALL, EXPORTED, DEFINED, UNDEFINED }           label_category;
typedef enum { ALPHABETIC, VALUE, DEFINITION, FOR_ELF }      label_sort;

typedef enum
  {
  SYM_REC_ADDED,      SYM_REC_DEFINED,    SYM_REC_REDEFINED,
  SYM_REC_UNCHANGED,  SYM_REC_ERROR
  }
defn_return;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

typedef struct sym_record_name                    /* Symbol record definition */
  {
  struct sym_record_name *pNext;                    /* Pointer to next record */
  unsigned int            count;              /* Number of characters in name */
  unsigned int             hash;
  unsigned int            flags;
  unsigned int       identifier;  /* Record identifier, also definition order */
  unsigned int      elf_section;    /* Section number - purely for ELF driver */
  int                     value;
  char       name[SYM_NAME_MAX];              /* Fixed field for name (quick) */
  }
sym_record;

typedef struct                              /* Symbol table header definition */
  {
  char             *name;
  unsigned int     symbol_number;
  unsigned int     flags;
  sym_record      *pList[SYM_TAB_LIST_COUNT];
  }
sym_table;

typedef struct sym_table_item_name   /* So we can make lists of symbol tables */
  {
  sym_table                  *pTable;    /* Pointer to symbol table (or NULL) */
  struct sym_table_item_name *pNext;     /* Pointer to next record  (or NULL) */
  }
sym_table_item;

typedef struct local_label_name             /* Local label element definition */
  {
  struct local_label_name *pNext;                   /* Pointer to next record */
  struct local_label_name *pPrev;               /* Pointer to previous record */
  unsigned int             label;          /* The value of the label (number) */
  unsigned int             value;                     /* The label word value */
  unsigned int             flags;
  }
local_label;

typedef struct own_label_name          /* Definition of label on current line */
  {
  label_type                 sort;            /* What `sort' of label, if any */
  struct sym_record_name  *symbol;    /* Pointer to symbol record, if present */
  struct local_label_name  *local;      /* Pointer to local label, if present */
  }
own_label;

typedef struct elf_temp_name
  {
  struct elf_temp_name *pNext;
  boolean               continuation;
  unsigned int          section;
  unsigned int          address;
  unsigned int          count;
  char                  data[ELF_TEMP_LENGTH];
  }
elf_temp;

typedef struct elf_info_name                 /* Section info collecting point */
  {                                  /* Just the bits I think need collecting */
  struct elf_info_name *pNext;
  unsigned int          name;
  unsigned int          address;
  unsigned int          position;
  unsigned int          size;
  }
elf_info;

typedef struct size_record_name          /* Size of variable length operation */
  {                                                /*  (form an ordered list) */
  struct size_record_name *pNext;
  unsigned int             size;
  }
size_record;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

const int SYM_RECORD_SIZE     = sizeof(sym_record);
const int SYM_TABLE_SIZE      = sizeof(sym_table);
const int SYM_TABLE_ITEM_SIZE = sizeof(sym_table_item);
const int LOCAL_LABEL_SIZE    = sizeof(local_label);
const int ELF_TEMP_SIZE       = sizeof(elf_temp);
const int ELF_INFO_SIZE       = sizeof(elf_info);
const int SIZE_RECORD_SIZE    = sizeof(size_record);

/*----------------------------------------------------------------------------*/

boolean      set_options(int argc, char *argv[]);

boolean      input_line(FILE*, char*, unsigned int);
boolean      parse_mnemonic_line(char*, sym_table*, sym_table*, sym_table*);
unsigned int parse_source_line(char*, sym_table_item*, sym_table*, int, int,
                               char**, char*);
void         print_error(char*, unsigned int, unsigned int, char*, int);
unsigned int assemble_line(char*, unsigned int, unsigned int,
                           own_label*, sym_table*, int, int, char**, char*);

unsigned int imm_jal(unsigned int);

unsigned int variable_item_size(int, unsigned int);

int          get_thing(char*, unsigned int*, sym_table*);
int          get_reg(char*, unsigned int*);

void         redefine_symbol(char*, sym_record*, sym_table*);
void         assemble_redef_label(unsigned int,  int, own_label*,
                                  unsigned int*, int, int, int, char*);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

unsigned int evaluate(char*, unsigned int*, int*, sym_table*);
int        get_variable(char*, unsigned int*, int*, int*, boolean*, sym_table*);
int        get_operator(char*, unsigned int*, int*, int*);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

sym_table   *sym_create_table(char*, unsigned int);
int          sym_delete_table(sym_table*, boolean);
defn_return  sym_define_label(char*, unsigned int, unsigned int,
                              sym_table*, sym_record**);
int          sym_locate_label(char*, unsigned int, sym_table*, sym_record**);
sym_record  *sym_find_label_list(char*, sym_table_item*);
sym_record  *sym_find_label(char*, sym_table*);
sym_record  *sym_create_record(char*, unsigned int, unsigned int, unsigned int);
void         sym_delete_record(sym_record*);
int          sym_delete_record_list(sym_record**, int);
int          sym_add_to_table(sym_table*, sym_record*);
sym_record  *sym_find_record(sym_table*, sym_record*);
void         sym_string_copy(char*, sym_record*, unsigned int);
char        *sym_strtab(sym_record*, unsigned int, unsigned int*);
sym_record  *sym_sort_symbols(sym_table*, label_category, label_sort);
unsigned int sym_count_symbols(sym_table*, label_category);
void         sym_print_table(sym_table*,label_category,label_sort,int,char*);
void         local_label_dump(local_label*, FILE*);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void byte_dump(unsigned int, unsigned int, char*, int);

FILE *open_output_file(int, char*);
void close_output_file(FILE*, char*, int);
void hex_dump(unsigned int, char);
void hex_dump_flush(void);

void elf_dump(unsigned int, char);
void elf_new_section_maybe(void);
void elf_dump_out(FILE*, sym_table*);

void list_file_out(void);
void list_start_line(unsigned int, int);
void list_mid_line(unsigned int, char*, int);
void list_end_line(char*);
void list_symbols(FILE*, sym_table*);
void list_buffer_init(char*, unsigned int, int);
void list_hex(unsigned int, unsigned int, char*);

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

int           skip_spc(char*, int);
char          *file_path(char*);
char          *pathname(char*, char*);
boolean       cmp_next_non_space(char*, int*, int, char);
boolean       test_eol(char);
unsigned int  get_identifier(char*, unsigned int, char*, unsigned int);
boolean       alpha_numeric(char);
boolean       alphabetic(char);
unsigned char c_char_esc(unsigned char);
int           get_num(char*, int*, int*, unsigned int);
int           allow_error(unsigned int, boolean, boolean);

/*----------------------------------------------------------------------------*/
/* Global variables                                                           */

char      *input_file_name;
char      *symbols_file_name;
char      *list_file_name;
char      *hex_file_name;
char      *elf_file_name;
char      *verilog_file_name;
FILE      *fList, *fHex, *fElf, *fVerilog;
int        symbols_stdout, list_stdout, hex_stdout, elf_stdout;   /* Booleans */
boolean    verilog_stdout;
boolean    verilog_bytes;
label_sort symbols_order;
int        list_sym, list_kmd;

unsigned char Verilog_array[VERILOG_MAX]; /* Assemble Verilog output in array */
unsigned int verilog_mem_size;                       /* Size of buffer =used= */

unsigned int sym_print_extras;              /* <0> set for locals  (only ATM) */

unsigned int rv_variant;

unsigned int assembly_pointer;       /* Address at which to plant instruction */
unsigned int def_increment;           /* Offset reached from assembly_pointer */
boolean      assembly_pointer_defined;
unsigned int entry_address;
boolean      entry_address_defined;
unsigned int data_pointer;           /* Used for creating record offsets etc. */
unsigned int undefined_count;  /* Number of references to undefined variables */
unsigned int   defined_count;        /* Number of variables defined this pass */
unsigned int redefined_count;      /* Number of variables redefined this pass */
unsigned int pass_count;                          /* Pass number, starts at 0 */
unsigned int pass_errors;                     /* Errors occurred in this pass */
boolean      div_zero_this_pass;  /* Indicates /0 in pass, prevents code dump */
boolean      dump_code;         /* Allow output (FALSE on last pass if error) */

own_label *evaluate_own_label;	                                /* Yuk! @@@@@ */
/* Because evaluate needs to know if there is a local label on -current- line */

local_label    *loc_lab_list;            /* Start of the list of local labels */
local_label    *loc_lab_position;         /* The current local label position */

size_record    *size_record_list;     /* Start of list of ADRL (etc.) lengths */
size_record    *size_record_current;          /* Current record in above list */
unsigned int    size_changed_count;   /* Number of `instruction' size changes */

boolean      if_stack[IF_STACK_SIZE + 1];      /* Used for nesting IF clauses */
int if_SP;

unsigned int list_address;
unsigned int list_byte;
unsigned int list_line_position;     /* Pos. in the src line copied to output */
char         list_buffer[LIST_LINE_LENGTH];

unsigned int hex_address;
boolean      hex_address_defined;
char         hex_buffer[HEX_LINE_LENGTH];

int          elf_section_valid;   /* Flag: true if code dumped in elf_section */
unsigned int elf_section;          /* Current elf section number (for labels) */
unsigned int elf_section_old;
boolean      elf_new_block;

elf_temp     *elf_record_list;
elf_temp     *current_elf_record;

sym_table        *arch_table;    /* Table of possible processor architectures */
sym_table    *operator_table;
sym_table    *register_table;
sym_table       *shift_table;
sym_table         *csr_table;

/*----------------------------------------------------------------------------*/
/* Entry point */

int main(int argc, char *argv[])
{
FILE *fMnemonics, *fSource;
char c, line[LINE_LENGTH+1];

sym_table *rv32i_mnemonic_table, *directive_table;
sym_table *symbol_table;
sym_table_item *rv32i_mnemonic_list;                            /* Real lists */
int i, *j;
boolean finished, last_pass;
unsigned int error_code;

  void code_pass(FILE *fHandle, char *filename)/* Recursion for INCLUDE files */
    {
    unsigned int line_number;
    char *include_file_path;        /* Path as far as directory of "filename" */
    char *include_name;
    FILE *incl_handle;

    include_file_path = file_path(filename);      /* Path to directory in use */
    line_number = 1;

    while (!feof(fHandle))
      {
      include_name = NULL;                  /* Don't normally return anything */
      input_line(fHandle, line, LINE_LENGTH);           /* Errors ignored @@@ */

      error_code = parse_source_line(line, rv32i_mnemonic_list, symbol_table,
                                     pass_count, last_pass,
                                     &include_name, include_file_path);

/*
printf("Hello Y %08X %s\n", symbol_table->pList[0], line);
*/

      if (error_code != eval_okay)
        print_error(line, line_number, error_code, filename, last_pass);
      else
        if (include_name != NULL)
          {
          char *pInclude;

          if (include_name[0] == '/') pInclude = include_name;    /* Absolute */
          else                           /* Relative path - create new string */
            pInclude = pathname(include_file_path, include_name); /* Add path */

          if ((incl_handle = fopen(pInclude, "r")) == NULL)
            {
            print_error(line, line_number, SYM_NO_INCLUDE, filename, last_pass);
            fprintf(stderr, "Can't open \"%s\"\n", include_name);
            finished = TRUE;
            }
          else
            {
            code_pass(incl_handle, pInclude);
            fclose(incl_handle);             /* Doesn't leave file locked @@@ */
            }
          if (pInclude != include_name) free(pInclude); /* If allocated (yuk) */
          free(include_name);
          }
      line_number++;                                         /* Local to file */
      }

    free(include_file_path);
    return;
    }

  /* Create and initialise a symbol table */
  sym_table *build_table(char *table_name, unsigned int flags,
                         char **sym_names, int *values)
    {
    sym_table *table;
    int i;
    sym_record *dummy;                         /* Don't want returned pointer */

    table = sym_create_table(table_name, flags);

    for (i = 0; *(sym_names[i]) != '\0'; i++)        /* repeat until "" found */
      sym_define_label(sym_names[i], values[i], 0, table, &dummy);

    return table;
    }


rv32i_mnemonic_list = NULL;
symbols_file_name = "";                                           /* Defaults */
list_file_name    = "";
hex_file_name     = "";
elf_file_name     = "";
verilog_file_name = "";
symbols_stdout    = FALSE;
list_stdout       = FALSE;
hex_stdout        = FALSE;
elf_stdout        = FALSE;
verilog_stdout    = FALSE;
verilog_mem_size  = VERILOG_MAX;                   /* Default to maximum size */
verilog_bytes     = FALSE;                /* Divide Verilog output into bytes */

if (set_options(argc, argv))/* Parse command line and set options accordingly */
  {                                  /* We have a source file name, at least! */
  char *pChar, full_name[200];					// Size? @@

                                          /* Set up tables of operators, etc. */

  {                                                     /* Architecture names */
  char *arch_name[] = {    "v3",    "v3m",    "v4",  "v4xm",   "v4t", "v4txm",
                           "v5",   "v5xm",   "v5t", "v5txm",  "v5te","v5texp",
                          "all",    "any",      "" };

  int  arch_value[] = {      v3,      v3M,      v4,    v4xM,     v4T,   v4TxM,
                             v5,     v5xM,     v5T,   v5TxM,    v5TE,  v5TExP,
                              0,        0,      -1 };

  arch_table = build_table("Architectures", SYM_TAB_CASE_FLAG,
                            arch_name, arch_value);
  }

  {                                 /* Diadic expression operator definitions */
  char *op_name[] = {      "and",          "or",         "xor",         "eor",
                           "shl",         "lsl",         "shr",         "lsr",
                           "div",         "mod",          "eq",          "ne",
                            "lo",          "ls",          "hi",          "hs",
                            "lt",          "le",          "gt",          "ge",
                              "" };
  int  op_value[] = {        AND,            OR,           XOR,           XOR,
                      LEFT_SHIFT,    LEFT_SHIFT,   RIGHT_SHIFT,   RIGHT_SHIFT,
                          DIVIDE,       MODULUS,        EQUALS,     NOT_EQUAL,
                      LOWER_THAN,   LOWER_EQUAL,   HIGHER_THAN,  HIGHER_EQUAL,
                       LESS_THAN,    LESS_EQUAL,  GREATER_THAN, GREATER_EQUAL,
                              -1 };

  operator_table = build_table("Operators", SYM_TAB_CASE_FLAG,
                                op_name, op_value);
  }

  {                                              /* Register name definitions */
  char *reg_name[] = {"x0",   "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7",  
                      "x8",   "x9", "x10", "x11", "x12", "x13", "x14", "x15", 
                      "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23", 
                      "x24", "x25", "x26", "x27", "x28", "x29", "x30", "x31",
                      "zero", "ra",  "sp",  "gp",  "tp",  "t0",  "t1",  "t2",
                      "fp",
                      "s0",   "s1",  "a0",  "a1",  "a2",  "a3",  "a4",  "a5",
                      "a6",   "a7",  "s2",  "s3",  "s4",  "s5",  "s6",  "s7",
                      "s8",   "s9", "s10", "s11",  "t3",  "t4",  "t5",  "t6",
                        "" };
  int  reg_value[] = {  0,     1,     2,     3,     4,     5,     6,     7,
                        8,     9,    10,    11,    12,    13,    14,    15,
                       16,    17,    18,    19,    20,    21,    22,    23,
                       24,    25,    26,    27,    28,    29,    30,    31,
                        0,     1,     2,     3,     4,     5,     6,     7,
                        8,
                        8,     9,    10,    11,    12,    13,    14,    15,
                       16,    17,    18,    19,    20,    21,    22,    23,
                       24,    25,    26,    27,    28,    29,    30,    31,
                       -1 };

  register_table = build_table("Registers", SYM_TAB_CASE_FLAG,
                                reg_name, reg_value);
  }


  {                                                      /* Shift definitions */
  char *shift_name[] = {"lsl", "asl", "lsr", "asr", "ror", "rrx",    ""};
  int  shift_value[] = {    0,     0,     1,     2,     3,     7,    -1};
  shift_table = build_table("Shifts",SYM_TAB_CASE_FLAG,shift_name,shift_value);
  }

  rv32i_mnemonic_table = sym_create_table("RV32 Mnemonics", SYM_TAB_CASE_FLAG);
//csr_table            = sym_create_table("CSR Addresses",  SYM_TAB_CASE_FLAG);
  csr_table            = sym_create_table("CSR Addresses",  0);
  directive_table      = sym_create_table("Directives",     SYM_TAB_CASE_FLAG);

  /* Following is crude hack for test/commissioning purposes.        @@@@@    */
  realpath(argv[0], full_name);			// full path to binary (?) @@
  for (pChar = full_name; *pChar != '\0'; pChar++);	// find end of string @@
  while (*pChar != '/') pChar--;			// Cut off last element
  pChar[1] = '\0';					// Terminate
  strcat(full_name, "mnemonics");			// Then append filename

  if ((fMnemonics = fopen(full_name, "r")) == NULL)         /* Read mnemonics */
    fprintf(stderr, "Can't open %s\n", "mnemonics");
  else
    {
    while (!feof(fMnemonics))
      {
      input_line(fMnemonics, line, LINE_LENGTH);        /* Errors ignored @@@ */
      if (!parse_mnemonic_line(line, rv32i_mnemonic_table, csr_table,
                               directive_table))
        fprintf(stderr, "Mnemonic file error\n %s\n", &line[0]);
      }
                                                       /* no error checks @@@ */
    fclose(fMnemonics);

    {                                         /* Make up mnemonic table lists */
    sym_table_item *pMnem, *pDir;

    pMnem = (sym_table_item*) malloc(SYM_TABLE_ITEM_SIZE);     /* RV32 defns. */
    pDir  = (sym_table_item*) malloc(SYM_TABLE_ITEM_SIZE);    /* (hand built) */
    pMnem->pTable = rv32i_mnemonic_table;
    pMnem->pNext  = pDir;
    pDir->pTable  = directive_table;
    pDir->pNext   = NULL;
    rv32i_mnemonic_list = pMnem;
    }

    symbol_table = sym_create_table("Labels", 0);/* Labels are case sensitive */
    loc_lab_list = NULL;
    size_record_list = NULL;

    pass_count   = 0;
    finished     = FALSE;
    last_pass    = FALSE;
    dump_code    = FALSE;

    fHex  = open_output_file( hex_stdout,  hex_file_name);   /* Open required */
    fList = open_output_file(list_stdout, list_file_name);   /*  output files */
    fElf  = open_output_file( elf_stdout,  elf_file_name);
    fVerilog = open_output_file(verilog_stdout, verilog_file_name);

    if ((fList != NULL) && list_kmd) fprintf(fList, "KMD\n");   /* KMD marker */

    if ((fSource = fopen(input_file_name, "r")) == NULL)      /* Read file in */
      {
      fprintf(stderr,"Can't open %s\n", input_file_name);
      finished = TRUE;
      }
    else
      finished = FALSE;

    if (fVerilog != NULL)
      {
      int i;
      for (i = 0; i < verilog_mem_size; i++) Verilog_array[i] = 0;
      }

    while (!finished)
      {
      assembly_pointer         = 0;                                /* Default */
      data_pointer             = 0;
      entry_address            = 0;
      assembly_pointer_defined = TRUE;              /* ??? @@@@  Okay for us! */
      entry_address_defined    = FALSE;
      rv_variant          = 0;          /* Default to any RISC-V architecture */
      pass_errors         = 0;
      div_zero_this_pass  = FALSE;
      hex_address_defined = FALSE;
      elf_new_block       = TRUE;
      undefined_count     = 0;   /* Reads of undefined variables on this pass */
      defined_count       = 0;           /* Labels newly defined on this pass */
      redefined_count     = 0;     /* Labels with values changed on this pass */
      if_SP               = 0;
      if_stack[0]         = TRUE;
      elf_section_valid   = FALSE;                     /* No bytes dumped yet */
      elf_section         = 1;
      loc_lab_position    = NULL;
      size_record_current = size_record_list;          /* Go to front of list */
      size_changed_count  = 0;

      rewind(fSource);                             /* Ensure at start of file */

      code_pass(fSource, input_file_name);
                                                       /* no error checks @@@ */
/*
{     // Variable size record printout for debugging
size_record *pP;
int x;
pP = size_record_list;
x = 0;
while (pP != NULL)
  {
  printf("Record %2d  size %d\n", x, pP->size);
  x++;
  pP = pP->pNext;
  }
}
*/

      hex_dump_flush();                           /* Ensure buffer is cleared */

//    printf("Pass %2d complete.  ", pass_count);
//    printf("Label changes: defined %3d; ", defined_count);
//    printf("values changed %3d; ", redefined_count);
//    printf("read while undefined %3d;\n", undefined_count);
//printf("\n");
//printf("Pass %2d complete: %d sizes changed.\n", pass_count, size_changed_count);

      if (pass_errors != 0)
        {
        finished = TRUE;
        printf("Pass %2d: terminating due to %d error", pass_count,
                                                        pass_errors);
        if (pass_errors == 1) printf("\n"); else printf("s\n");
        }
      else
        {
        if (last_pass || (pass_count > MAX_PASSES)) finished = TRUE;
        else
          {
          if ((defined_count==0)&&(redefined_count==0)&&(undefined_count==0))
            {
            last_pass = TRUE;                            /* One more time ... */
            dump_code = !div_zero_this_pass;     /* If error don't plant code */
            }
          pass_count++;
          }
        }


      if (if_SP != 0)
        {
        printf("Pass completed with IF clause still open; terminating\n");
        finished = TRUE;
        }

// printf("Labels redefined: %d on pass %d\n", redefined_count, pass_count);
// sym_print_table(symbol_table, ALL, ALPHABETIC, TRUE, ""); Debug monitoring

      }                                                       /* End of WHILE */


    if (fSource != NULL) fclose(fSource);

    if ((fList != NULL) && list_sym) list_symbols(fList, symbol_table);
                                                    /* Symbols into list file */

    if (fVerilog != NULL)                    /* Dump memory image to hex file */
      {
      int i;		// Want some variants - e.g. separate bytes @@@
      for (i = 0; i < verilog_mem_size; i++)
        {
        if (verilog_bytes)
          fprintf(fVerilog, "%02X ", Verilog_array[i]);      /* Little endian */
        else
          fprintf(fVerilog, "%02X", Verilog_array[i^3]);        /* Big endian */
        if ((i & 3) == 3) fprintf(fVerilog, "\n");
        }
      }

    close_output_file(fList, list_file_name, pass_errors != 0);
    close_output_file(fHex,   hex_file_name, pass_errors != 0);
    close_output_file(fVerilog, verilog_file_name, pass_errors != 0);

    if (fElf != NULL) elf_dump_out(fElf, symbol_table); /* Organise & o/p ELF */

    if (pass_count > MAX_PASSES)
      {
      printf("Couldn't do it ... fed up!\n\n");
      printf("Undefined labels:\n");
      sym_print_table(symbol_table, UNDEFINED, ALPHABETIC, TRUE, "");
      }
    else
      {
      if (symbols_stdout || (symbols_file_name[0] != '\0'))
        {
        sym_print_table(symbol_table, ALL, symbols_order, symbols_stdout,
                                                       symbols_file_name);
        if (!symbols_stdout) printf("Symbols in file: %s\n", symbols_file_name);
        }

      if (pass_errors == 0)
        {
        if (list_file_name[0]!='\0') printf("List file in: %s\n",list_file_name);
        if (hex_file_name[0] !='\0') printf("Hex dump in: %s\n",  hex_file_name);
        if (elf_file_name[0] !='\0') printf("ELF file in: %s\n",  elf_file_name);
        if (verilog_file_name[0] !='\0')
          {
          printf("Verilog file in: %s", verilog_file_name);
          printf("  size: %X (decimal %d) words\n", verilog_mem_size,
                                                     verilog_mem_size);
          }
        }
      else printf("No output generated.\n");  /* Errors => trash output files */

      if (pass_count == 1) printf("\n1 pass performed.\n");
      else printf("\nComplete.  %d passes performed.\n", pass_count);
      }

    {                                                /* Free local label list */
    local_label *pTemp;
    while ((pTemp = loc_lab_list) != NULL)           /* Syntactically grubby! */
      {
      loc_lab_list = loc_lab_list->pNext;
      free(pTemp);
      }
    }

    {                                                       /* Free size list */
    size_record *pTemp;
    while ((pTemp = size_record_list) != NULL)       /* Syntactically grubby! */
      {
      size_record_list = size_record_list->pNext;     /* Cut out first record */
      free(pTemp);                                          /*  and delete it */
      }
    }

    {                                    /* Clear away lists of symbol tables */
    sym_table_item *p1, *p2;

    p1 = rv32i_mnemonic_list;
    while (p1 != NULL) { p2 = p1; p1 = p1->pNext; free(p2); }
    }

/*
symbols_file_name = "Wombat";
printf("Symbol name [%s]\n", symbols_file_name);
        sym_print_table(csr_table, ALL, VALUE, TRUE, symbols_file_name);
*/

    sym_delete_table(        symbol_table, FALSE);
    sym_delete_table(     directive_table, FALSE);
    sym_delete_table(rv32i_mnemonic_table, FALSE);
    sym_delete_table(           csr_table, FALSE);
    sym_delete_table(          arch_table, FALSE);
    sym_delete_table(      operator_table, FALSE);
    sym_delete_table(      register_table, FALSE);
    sym_delete_table(         shift_table, FALSE);
    }
  }
else
  printf("No input file specified\n");

if (pass_errors == 0) exit(0);
else                  exit(-1);
}

/*----------------------------------------------------------------------------*/
/*					// Allow omission of spaces? @@@@     */

boolean set_options(int argc, char *argv[])
{

  void file_option(int *std_out, char **filename, char *err_mss)
    {
    if (argc > 2)
      {
      if ((argv[1])[0] == '-') *std_out = TRUE;
      else { *filename = &(*argv[1]); argc --; argv++; }
      }
    else printf("%s filename omitted\n", err_mss);
    return;
    }

boolean okay;
char c;

okay = FALSE;

if (argc == 1)
  {
  printf("RISC V assembler v0.10 (22/8/22)\n");
  printf("Hacked from: ARM assembler v0.28 (23/03/15)\n");
  printf("Usage: %s <options> filename\n", argv[0]);
  printf("Options:    -e <filename>  specify ELF output file\n");
  printf("            -h <filename>  specify hex dump file\n");
  printf("            -l <filename>  specify list file\n");
  printf("                -ls appends symbol table\n");
  printf("                -lk produces a KMD file\n");
  printf("            -s <filename>  specify symbol table file\n");
  printf("                -sd gives symbols in order of definition\n");
  printf("                -sv gives symbols sorted by value\n");
  printf("                -sl includes local labels\n");
  printf("            -v <filename>  specify Verilog readmemh file\n");
  printf("                [<hex number>] following name sets file size\n");
  printf("                 :b in the brackets dumps as separate bytes\n");
  printf("                    default (max) = %08X\n", VERILOG_MAX);
  printf("                    (output aliases modulo this size)\n");
  printf("Omitting a filename (or using '-') directs to stdout\n");
  }
else
  {
  argv++;                                                     /* Next pointer */

//while ((argc > 1) && ((*argv)[0] == '-'))
				// Changed to increase argument ordering options
  while (argc > 1)
    {
    if ((*argv)[0] == '-')
      {
      c = (*argv)[1];
      switch (c)
        {
        case '\0': break;                    /* Can be used as a non-filename */

        case 'E':
        case 'e':
          file_option(&elf_stdout, &elf_file_name, "Elf file");
          break;

        case 'H':
        case 'h':
          file_option(&hex_stdout, &hex_file_name, "Hex dump");
          break;

        case 'L':
        case 'l':
          list_sym = ((((*argv)[2]&0xDF) == 'S') || (((*argv)[2]&0xDF) == 'K'));
                                              /* 'S' or 'K' dumps symbols too */
          list_kmd = (((*argv)[2] & 0xDF) == 'K');  /* K inserts "KMD" header */
          file_option(&list_stdout, &list_file_name, "List");
          break;

        case 'S':
        case 's':
          {
          int pos;
          pos = 2;
          switch ((*argv)[pos])
            {
            case 'v': case 'V': symbols_order = VALUE;      pos++; break;
            case 'd': case 'D': symbols_order = DEFINITION; pos++; break;
            default:            symbols_order = ALPHABETIC; break;
            }

          while ((*argv)[pos] != '\0')
            {
            if (((*argv)[pos]=='l')||((*argv)[pos]=='L')) sym_print_extras |= 1;
//          if (((*argv)[pos]=='p')||((*argv)[pos]=='P')) sym_print_extras |= 2;
            pos++;
            }
          file_option(&symbols_stdout, &symbols_file_name, "Symbol");
          }
          break;

        case 'V':
        case 'v':
          file_option(&verilog_stdout, &verilog_file_name,
                                       "Verilog memory format");
          if (argc > 2)
            {
            if ((argv[1])[0] == '[')
              {
              unsigned int x, p;
              x = 0;
              p = 1;

              if (get_num(argv[1], &p, &x, 16))                /* Input value */
                if (x < verilog_mem_size) verilog_mem_size = x;/* Clip to max.*/

              if ((argv[1])[p] == ':') p++;
              if (((argv[1])[p] | 0x20) == 'b')
                { verilog_bytes = TRUE; p++; }
              if ((argv[1])[p] != ']')
                printf("Please close the brackets!\n");

              argc--;
              argv++;
              }
            }
          break;

        default:
          printf("Unknown option %c\n", c);
          break;
        }
      }
    else
      {
      input_file_name = *argv;
      printf("Input file: %s\n", input_file_name);
      okay = TRUE;
      }
    argc--;                                    /* Remove parameter from count */
    argv++;                                                   /* Next pointer */
    }
/*   Snipped to increase argument ordering options
  if (argc > 1)
    {
    input_file_name = *argv;
    printf("Input file: %s\n", input_file_name);
    okay = TRUE;
    }
*/
  }
return okay;
}

/*----------------------------------------------------------------------------*/

void print_error(char *line, unsigned int line_no, unsigned int error_code,
                 char *filename, boolean last_pass)
{
unsigned int error, position;
int i;

if ((error_code & WARNING_ONLY) != 0)
  {
  if (!last_pass) return;                                            /* Barf! */
  else printf("Warning: ");
  }
else pass_errors++;                                   /* Don't tally warnings */

/*  The position on the line is in the bottom 8 bits; 0 indicates undefined.  */
position = error_code & 0x000000FF;

switch (error_code & 0xFFFFFF00)
  {
  case SYM_ERR_SYNTAX:      printf("Syntax error");                       break;
  case SYM_ERR_NO_MNEM:     printf("Mnemonic not found");                 break;
  case SYM_ERR_NO_EQU:      printf("Label missing");                      break;
  case SYM_BAD_REG:         printf("Bad register");                       break;
//case SYM_BAD_REG_COMB:    printf("Illegal register combination");       break;
//case SYM_NO_REGLIST:      printf("Register list required");             break;
  case SYM_NO_COMMA_LBR:    printf("'[' (or base register) expected");    break;
//case SYM_NO_RSQUIGGLE:    printf("Missing '}'");                        break;
  case SYM_OORANGE:         printf("Value out of range");                 break;
  case SYM_ENDLESS_STRING:  printf("String unterminated");                break;
  case SYM_DEF_TWICE:       printf("Label redefined");                    break;
  case SYM_NO_COMMA:        printf("',' expected");                       break;
  case SYM_GARBAGE:         printf("Garbage");                            break;
  case SYM_ERR_NO_EXPORT:   printf("Exported label not defined");         break;
  case SYM_INCONSISTENT:    printf("Label redefined inconsistently");     break;
  case SYM_ERR_NO_FILENAME: printf("Filename missing");                   break;
  case SYM_NO_LBR:          printf("'[' expected");                       break;
  case SYM_NO_RBR:          printf("']' expected");                       break;
  case SYM_ADDR_MODE_ERR:   printf("Error in addressing mode");           break;
//case SYM_ADDR_MODE_BAD:   printf("Illegal addressing mode");            break;
//case SYM_NO_LSQUIGGLE:    printf("'{' expected");                       break;
  case SYM_OFFSET_TOO_BIG:  printf("Offset out of range");                break;
//case SYM_BAD_COPRO:       printf("Coprocessor specifier expected");     break;
  case SYM_BAD_VARIANT:     printf("Instruction not available");          break;
//case SYM_NO_COND:         printf("Conditional execution forbidden");    break;
//case SYM_BAD_CP_OP:       printf("Bad coprocessor operation");          break;
  case SYM_NO_LABELS:       printf("No labels! Position uncertain");      break;
  case SYM_DOUBLE_ENTRY:    printf("Entry already defined");              break;
  case SYM_NO_INCLUDE:      printf("Include file missing");               break;
//case SYM_NO_BANG:         printf("'!' expected");                       break;
  case SYM_MISALIGNED:      printf("Offset misaligned");                  break;
  case SYM_OORANGE_BRANCH:  printf("Branch out of range");                break;
  case SYM_UNALIGNED_BRANCH:printf("Branch to misaligned target");        break;
  case SYM_VAR_INCONSISTENT:printf("Variable redefined inconsistently");  break;
  case SYM_NO_IDENTIFIER:   printf("Identifier expected");                break;
  case SYM_MANY_IFS:        printf("Too many nested IFs");                break;
  case SYM_MANY_FIS:        printf("ENDIF without an IF");                break;
  case SYM_LOST_ELSE:       printf("Floating ELSE");                      break;
  case SYM_NO_HASH:         printf("'#' expected");                       break;
  case SYM_NO_IMPORT:       printf("Import file missing");                break;
  case SYM_ADRL_PC:        printf("Only ADR allowed with destination PC");break;
  case SYM_ERR_NO_SHFT:     printf("Shift operator expected");            break;
  case SYM_NO_REG_HASH:     printf("'#' or register expected");           break;
  case eval_no_operand:     printf("Operand expected");                   break;
  case eval_no_operator:    printf("Operator expected");                  break;
  case eval_not_closebr:    printf("Missing ')'");                        break;
  case eval_not_openbr:     printf("Extra ')'");                          break;
  case eval_mathstack_limit:printf("Math stack overflow");                break;
  case eval_no_label:       printf("Label not found");                    break;
  case eval_label_undef:    printf("Label undefined");                    break;
  case eval_out_of_radix:   printf("Number out of radix");                break;
  case eval_div_by_zero:    printf("Division by zero");                   break;
  case eval_operand_error:  printf("Operand error");                      break;
  case eval_bad_loc_lab:    printf("Bad local label");                    break;
  case eval_no_label_yet:   printf("Label not defined before this point");break;

  default:                   printf("Strange error");                     break;
  }
printf(" on line %d of file: %s\n", line_no, filename);

/*printf(line); printf("\n");           /* This suppresses '%' characters :-( */
for (i = 0; line[i] != '\0'; i++) printf("%c", line[i]);printf("\n"); /* Yuk! */

if (position > 0)                           /* else position not well defined */
  {
  for (i = 0; i <= position-1; i++)            /* 1 space less than the posn. */
    if (line[i] == '\t') printf("\t"); else printf(" ");
  printf("^\n");        /* Mirrors TAB expansion (non-printing chars too? @@) */
  }

return;
}

/*----------------------------------------------------------------------------*/

boolean input_line(FILE *file, char *buffer, unsigned int max)
{
int i;
char c;

if (file != NULL)
  {
  i = 0;
  do
    {
    c = getc(file);
    if (!feof(file) && (i <= max - 1)) buffer[i++] = c;
    }
    while ((c != '\n') && (c != '\r') && !feof(file));

  buffer[i] = '\0';                                              /* Terminate */
  if ((i > 0) && ((buffer[i-1] == '\n') || (buffer[i-1] == '\r')))
    buffer[i-1] = '\0';                                           /* Strip LF */

  if (c == '\r') c = getc(file);             /* Strip off any silly DOS-iness */
  if (c != '\n') ungetc(c, file);         /* Yuk! In case there's -just- a CR */

  return TRUE;
  }
else return FALSE;                                          /* file not valid */
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

boolean parse_mnemonic_line(char *line, sym_table *a_table, sym_table *s_table,
                                        sym_table *d_table)
{
int i, j, k, okay;
unsigned int value, token;
sym_record *dummy;
char buffer[SYM_NAME_MAX + 5];  /* Largest suffix is 5 bytes, inc. terminator */
char *pCC;

i = skip_spc(line, 0);
j = 0;                                    /* Indicates end of `root' mnemonic */

if (!test_eol(line[i]))                    /* Something on line - not comment */
  {
  while (alpha_numeric(line[i]) && (j < SYM_NAME_MAX))
    buffer[j++] = line[i++];              /* Mnemonics may start with numeric */
  buffer[j] = '\0';                                         /* Add terminator */

  okay = get_num(line, &i, &value, 16);                     /* Get hex number */
/*  use evaluate() - mark real symbols for export and decimate before use @@@ */

  if (okay)
    {
    if ((value & 0xF0000000) == 0xF0000000)             /* Straight directive */
      sym_define_label(&buffer[0], value, 0, d_table, &dummy);
    else
    if ((value & 0x00000001) != 0)                             /* CSR address */
      sym_define_label(&buffer[0], value>>20, 0, s_table, &dummy);
    else
      {
      token = value;
      sym_define_label(&buffer[0], value, 0, a_table, &dummy);
      }
    }
  }
else okay = TRUE;                                    /* Blank line or comment */

return okay;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

unsigned int parse_source_line(char *line, sym_table_item *mnemonic_list,
                                           sym_table *symbols,
                                           int pass_count, boolean last_pass,
                                           char **include_name,
                                           char *include_file_path)
{
int pos, j;
own_label label_this_line;
sym_record *ptr;
label_type label;
char buffer[LINE_LENGTH];
unsigned int value, error_code;
boolean mnemonic, colon;

error_code             = SYM_NO_ERROR;				/* @@@@ */
mnemonic               = FALSE;
colon                  = FALSE;
label_this_line.sort   = NO_LABEL;
pos = skip_spc(line, 0);

if (last_pass && (fList != NULL)) list_start_line(assembly_pointer, FALSE);

if (!test_eol(line[pos]))                  /* Something on line - not comment */
  {
  if (get_num(line, &pos, &value, 10))  /* Look for a `local' (numeric) label */
    {
    pos = skip_spc(line, pos);

    if (pass_count == 0)
      {
      local_label *pTemp;

      pTemp = (local_label*) malloc(LOCAL_LABEL_SIZE);           /* New entry */
      pTemp->pNext = NULL;
      pTemp->pPrev = loc_lab_position;
      if (loc_lab_position == NULL) loc_lab_list = pTemp;      /* First entry */
      else               loc_lab_position->pNext = pTemp; /* Subsequent entry */
      loc_lab_position = pTemp;
      loc_lab_position->label = value;
      loc_lab_position->flags = 0;
      }
    else                                              /* After the first pass */
      {
      if (loc_lab_position==NULL) loc_lab_position = loc_lab_list;/* 1st entry*/
      else loc_lab_position = loc_lab_position->pNext;    /* Subsequent entry */
      }
    label_this_line.sort  = LOCAL_LABEL;
    label_this_line.local = loc_lab_position;
    }
  else
    {
    if ((j = get_identifier(line, pos, buffer, LINE_LENGTH)) != 0)
                                                           /* Element=>buffer */
      {
      pos = pos + j;                                 /* Move position in line */
      if (colon = (line[pos] == ':')) pos++;         /* Check for & strip ':' */

      if (colon || ((ptr = sym_find_label_list(buffer, mnemonic_list)) == NULL))
        {                                                   /* Not a mnemonic */
        if (sym_locate_label(buffer,         /* Pass in flag if in Thumb area */
                             0,
                             symbols, &(label_this_line.symbol)))
          label_this_line.sort = SYMBOL;                             /* Found */
        else
          {
          if (pass_count == 0)                                  /* First pass */
            label_this_line.sort = MAYBE_SYMBOL;  /* Could be reg. name, etc. */
          }
        }
      else
        mnemonic = TRUE;            /* Mnemonic first - no label on this line */
      }
    else
      error_code = pos | SYM_ERR_SYNTAX;   /* 1st char. on line is non-alpha. */
    }
                       /* If all is well, at this point the first item on the */
                       /*  line has been identified, classified and stripped. */

  if ((error_code == eval_okay) && !mnemonic)
                   /* Could check for other symbols (e.g. "=") first     @@@@ */
    {
    pos = skip_spc(line, pos);                      /* Find next item on line */
    if ((j = get_identifier(line, pos, buffer, LINE_LENGTH)) != 0)
      {                                          /* Possible identifier found */
      if ((ptr = sym_find_label_list(buffer, mnemonic_list)) == NULL)
        error_code = pos | SYM_ERR_NO_MNEM;			/*	//### */
      else
        {                                                   /* Mnemonic found */
        mnemonic = TRUE;
        pos = pos + j;                               /* Move position in line */
        }
      }
    else
      {                                 /* Nothing recognisable found on line */
      if (!test_eol(line[pos]))                             /* Effective EOL? */
        {                        /* Label(?) followed by something unexpected */
        error_code = pos | SYM_ERR_NO_MNEM;
        }
      else                                       /* Just a label on this line */
        {
        if (label_this_line.sort==MAYBE_SYMBOL) /* Uncertain only on 1st pass */
          sym_add_to_table(symbols, label_this_line.symbol);

        assemble_redef_label(assembly_pointer, assembly_pointer_defined,
                             &label_this_line, &error_code, 0,
                             pass_count, last_pass, line);
        }
      }
    }

  if ((error_code == eval_okay) && mnemonic)
    {
                 /* Check lower bits of token against current instruction set */
    if ((ptr->value & rv_variant & 0x00000FFF) != 0)
      error_code = SYM_BAD_VARIANT;      /* Disallowed in selected RV variant */
    else
      error_code = assemble_line(line, pos, ptr->value, &label_this_line,
                                 symbols, pass_count, last_pass,
                                 include_name, include_file_path);
    }
  }


if (last_pass)
  {
  if (fList != NULL) list_end_line(&line[0]);

  if ((label_this_line.sort == SYMBOL)
  && ((label_this_line.symbol->flags & SYM_REC_EQUATED) == 0))
    label_this_line.symbol->elf_section = elf_section;      /* Purely for ELF */
  }

return error_code;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Make/maintain list of variable size items.                                 */
/* Inputs: first_pass - build or use flag                                     */
/*         size - desired size of item                                        */
/* Returns: size allocated (may be larger than desired)                       */
/* Globals: size_record_list, size_record_current, size_changed_count         */

unsigned int variable_item_size(int first_pass, unsigned int size)
{
if (first_pass)                       /* Build list of variable size elements */
  {
  size_record *pTemp;

  pTemp = (size_record*) malloc(SIZE_RECORD_SIZE);       /* Append new record */
  pTemp->pNext = NULL;
  pTemp->size  = size;
  if (size_record_list == NULL) size_record_list = pTemp;    /* Link in first */
  else                size_record_current->pNext = pTemp;    /* or subsequent */
  size_record_current = pTemp;                     /* Pointer to last in list */
  }
else
  {                                  /* Check for changes in object code size */
  if (size_record_current->size != size)
    {
    if ((pass_count < SHRINK_STOP)                        /* Can still shrink */
     || (size_record_current->size < size))                       /*  or grow */
      {
      size_record_current->size = size;
      size_changed_count++;                               /* Superfluous? @@@ */
      }
    else
      size = size_record_current->size;                         /* Size fixed */
    }
  size_record_current = size_record_current->pNext;     /* Global ptr to next */
  }

return size;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* If an INCLUDE <file> is found a string is allocated and pointed to by      */
/* include_name.                                                              */

unsigned int assemble_line(char *line, unsigned int position, 
                                       unsigned int token,
                                       own_label *my_label,
                                       sym_table *symbol_table,
                                       int pass_count,
                                       boolean last_pass,
                                       char **include_name,
                                       char *include_file_path)
{
unsigned int operand, error_code;
unsigned int temp;
int first_pass, i;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  void assemble_define(int size)
    {                                       /* Elongated by string definition */
    boolean terminate, escape;
    int i;
    char delimiter, c;

    terminate = FALSE;

    while (!terminate)
      {
      position = skip_spc(line, position);
      if (((line[position] == '"') || (line[position] == '\'')  /*  String    */
        || (line[position] == '/') || (line[position] == '`'))) /* delimiters */
        {                                                     /* Input string */
        delimiter = line[position++];             /* Strip & record delimiter */
        while ((line[position] != delimiter) && !terminate)
          {
          c = line[position];
          if (escape = (c == '\\'))                    /* C-style escape code */
            c = line[++position];                       /* Get next character */

          if (c != '\0')
            {
            if (last_pass)
              {                                                 /* Bytes only */
              if (escape) c = c_char_esc(c);
              byte_dump(assembly_pointer + def_increment, c, line, size);
              }
            def_increment = def_increment + size;  /* Always one address here */
            position++;
            }
          else
            {                              /* Line finished before string did */
            error_code = SYM_ENDLESS_STRING;
            terminate = TRUE;
            }
          }
        if (!terminate) position=skip_spc(line, position+1);  /*Skip delimiter*/
        }
      else
        {
        error_code = evaluate(line, &position, &temp, symbol_table);
                                                          /* Parse expression */
        if ((error_code == eval_okay)
          || allow_error(error_code, first_pass, last_pass))
          {
          if ((error_code == eval_okay) && last_pass)    /* Plant, ltl endian */
            byte_dump(assembly_pointer + def_increment, temp, line, size);

          if (!last_pass) error_code = eval_okay;        /* Pretend it's okay */
          def_increment += size;          /* Continue, even if missing values */
          }
        else
          terminate = TRUE;
        }

      if (!terminate)
        {
        if (line[position] == ',') position++;            /* Another element? */
        else terminate = TRUE;
        }
      }                                                       /* End of WHILE */
//##
if (if_stack[if_SP])
    assembly_pointer += def_increment;               /* Add total size at end */
    return;
    }

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  void fill_space(unsigned int count)
    {                                                      /* Fill with value */
    unsigned int fill;
    int i;

    position++;                                                 /* Skip comma */
    error_code = evaluate(line, &position, &fill, symbol_table);
    if (allow_error(error_code, first_pass, last_pass))
      error_code = eval_okay;

    if (last_pass && (error_code == eval_okay))
      for (i = 0; i < operand; i++)
        byte_dump(assembly_pointer + i, fill & 0xFF, line, 1);

    return;
    }

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Parse an addressing mode                                                   */

// New (replacement) bit for RISC-V

// Returns: 00reg_0off or -1 for error

  int addr_mode(unsigned int op_code, unsigned int *prefix)
  {
  boolean in_range, global, jalr;
  int offset, reg;

  in_range = FALSE;
  global   = FALSE;
  jalr     = (token & 0x0FFF0000) == 0x01950000;   /* Disallows some pseudos. */

  if (!cmp_next_non_space(line, &position, 0, '['))                /* Offset? */
    {
    error_code = evaluate(line, &position, &offset, symbol_table);
    if (allow_error(error_code, first_pass, last_pass))
      error_code = eval_okay;
    if (error_code == eval_okay)
      {
      if (cmp_next_non_space(line, &position, 0, '[')) /* Base register next? */
        {
        if (((offset & 0xFFFFF800) == 0x00000000)                /* In range? */
         || ((offset & 0xFFFFF800) == 0xFFFFF800)) in_range = TRUE;
        }
      else
        {                         /* Chance of load/store 'global' pseudo-op. */
        global = TRUE;

        if (jalr) error_code = SYM_NO_LBR | position;       /* '[' compulsory */
        else
          {
          if ((token & 0x000F0000) == 0x00040000)               /* If a store */
            {                                    /* look for another register */
            if (!cmp_next_non_space(line, &position, 0, ','))
              error_code = SYM_NO_COMMA_LBR | position;  /* Failed to find it */
            else
              {
              if ((reg = get_reg(line, &position)) < 0)
                error_code = SYM_BAD_REG | position;
              }
            }
          else reg = (op_code >> 7) & 0x1F;      /* Extract base from op_code */
                                     /* For Loads this is the target register */

          offset = offset - assembly_pointer; /* These are done PC-relative (!) */
//@##@
          *prefix = 0x00000017 | (reg << 7);                    /* Set up AUIPC */
          if ((offset & 0x00000800) == 0)              /* with immediate (high) */
            *prefix = *prefix | (offset & 0xFFFFF000);
          else
            *prefix = *prefix | ((offset & 0xFFFFF000) + 0x00001000);
          }
        }                  /* 'reg' and 'offset' set up for (eventual) return */
      }
    }
  else {offset = 0x000; in_range = TRUE;}      /* Found '[' before expression */

  if (!global)             /* Only pick up base register here following a '[' */
    {
    if (error_code == eval_okay)
      {
      if ((reg = get_reg(line, &position)) >= 0)
        {
        if (!cmp_next_non_space(line, &position, 0, ']'))
          error_code = SYM_NO_RBR | position;
        else position++;
        }
      else error_code = SYM_ERR_SYNTAX | position;
      }

    if ((error_code == eval_okay) && !in_range)         /* Low priority fault */
      error_code = SYM_OORANGE;                            /* (Still !global) */
    }

  if (error_code == eval_okay) return (reg << 16) | (offset & 0x00000FFF);
  else                         return -1;          /* Pack return information */
  }

// End of ...

  void prefix_dump(unsigned int prefix)	// Hack - just to collect in one place now @@@
  {
  if (last_pass) byte_dump(assembly_pointer, prefix, line, 4);
  assembly_pointer += 4;
  return;
  }

  unsigned int b_offset()  /* Parse 12-bit offset to scrambled op-code fields */
  {
  unsigned int value, offset;

  error_code = evaluate(line, &position, &value, symbol_table);
  if (allow_error(error_code, first_pass, last_pass))
    error_code = eval_okay;
  value = value - assembly_pointer;
  if ((value & 1) != 0)                     /* Not halfword aligned */
          // Insert trap for word alignment if 16-bit instructions disabled @@@
    error_code = SYM_UNALIGNED_BRANCH;
  else
    if (((value & 0xFFFFF000) == 0x00000000)    /*  or out of range */
     || ((value & 0xFFFFF000) == 0xFFFFF000))
      offset = ((value & 0x1000) << 19) | ((value & 0x0800) >>  4)/* Really?! */
             | ((value & 0x07E0) << 20) | ((value & 0x001E) <<  7);
    else error_code = SYM_OORANGE_BRANCH;
  return offset;
  }


  unsigned int parse_csr()
  {
  unsigned int value;

  error_code = evaluate(line, &position, &value, symbol_table);
  if (allow_error(error_code, first_pass, last_pass))
    error_code = eval_okay;
  if (error_code == eval_okay)
    if ((value & 0xFFFFF000) != 0x00000000)                  /* Out of range? */
      error_code = SYM_OORANGE;
  return value << 20;
  }

  unsigned int parse_csr2(unsigned int token)                 /* Parse Rs/imm */
  {
  unsigned int value;
  int reg;

  if ((token & 0x10000000) == 0)
    {
    if ((reg = get_reg(line, &position)) < 0)                     /* Parse rs */
      error_code = SYM_BAD_REG | position;
    else value = reg;
    }
  else
    {
    error_code = evaluate(line, &position, &value, symbol_table);
    if (allow_error(error_code, first_pass, last_pass))
      error_code = eval_okay;
    if (error_code == eval_okay)
      if ((value & 0xFFFFFFE0) != 0x00000000)                /* Out of range? */
        error_code = SYM_OORANGE;
    }
  return value << 15;
  }

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  void rv32i_mnemonic()
    {                                 /* Instructions, rather than directives */
    unsigned int op_code, value, extras;
    int reg;

    extras = 0;                                     /* Instruction length - 4 */

    if (first_pass || last_pass || ((token & 0x00008000) != 0))     /* -skip- */
//                              || ((token & 0x000E0000) == 0x00040000))
//                            /* Also nasty Load/store overloading at the end */
      {/* Only do difficult stuff on first pass (syntax) and last pass (code
         dump) unless instruction may cause file length to vary (e.g. LI ###) */

      switch (token & 0x000F0000)
        {
        case 0x00000000:                                            /* U-type */
          op_code = ((token & 0x01F00000) >> 18) | 0x00000003;

          if ((reg = get_reg(line, &position)) >= 0)
            {
            op_code = op_code | (reg << 7);
            if (!cmp_next_non_space(line, &position, 0, ','))
              error_code = SYM_NO_COMMA | position;
            }
          else error_code = SYM_BAD_REG | position;

          if (error_code == eval_okay)                     /* Parse Immediate */
            {
            error_code = evaluate(line, &position, &value, symbol_table);
            if (allow_error(error_code, first_pass, last_pass))
              error_code = eval_okay;
            if ((value & 0x00000FFF) != 0)                 /* Not valid value */
              error_code = SYM_OORANGE;
            else
              op_code = op_code | value;
            }
          break;


        case 0x00010000:                                            /* J-type */
          op_code = ((token & 0x01F00000) >> 18) | 0x00000003;

          if ((reg = get_reg(line, &position)) >= 0)
            {
            op_code = op_code | (reg << 7);
            if (!cmp_next_non_space(line, &position, 0, ','))
              error_code = SYM_NO_COMMA | position;
            }
          else
            op_code = op_code | (1 << 7);                        /* assume X1 */
//        else error_code = SYM_BAD_REG | position;

          if (error_code == eval_okay)                  /* Now the offset ... */
            {
            error_code = evaluate(line, &position, &value, symbol_table);
            if (allow_error(error_code, first_pass, last_pass))
              error_code = eval_okay;
            value = value - assembly_pointer;
            if ((value & 1) != 0)                     /* Not halfword aligned */
          // Insert trap for word alignment if 16-bit instructions disabled @@@
              error_code = SYM_UNALIGNED_BRANCH;
            else
              if (((value & 0xFFF00000) == 0x00000000)    /*  or out of range */
               || ((value & 0xFFF00000) == 0xFFF00000))
                op_code = op_code | ((value & 0x100000) << 11)    /* Really?! */
                                  |  (value & 0x0FF000)
                                  | ((value & 0x000800) << 9)
                                  | ((value & 0x0007FE) << 20);
              else error_code = SYM_OORANGE_BRANCH;
            }
          break;

  
        case 0x00020000:                                            /* I-type */
          op_code = ((token & 0x01F00000) >> 18)
                  | ((token & 0x0E000000) >> 13) | 0x00000003;

          if ((reg = get_reg(line, &position)) >= 0)
            {
            op_code = op_code | (reg << 7);
            if (!cmp_next_non_space(line, &position, 0, ','))
              error_code = SYM_NO_COMMA | position;
            }
          else
            error_code = SYM_BAD_REG | position;

          if (error_code == eval_okay)
            {
            if ((reg = get_reg(line, &position)) >= 0)
              {
              op_code = op_code | (reg << 15);
              if (!cmp_next_non_space(line, &position, 0, ','))
                error_code = SYM_NO_COMMA | position;
              }
            else
              error_code = SYM_BAD_REG | position;
            }

          if (error_code == eval_okay)                     /* Parse Immediate */
            {
            error_code = evaluate(line, &position, &value, symbol_table);
            if (allow_error(error_code, first_pass, last_pass))
              error_code = eval_okay;
            if ((token & 0x10000000) != 0x00000000) value = -value; /* 'SUBI' */

            if ((token & 0x06000000) == 0x02000000)       /* Immediate shifts */
              {
              if ((value & 0xFFFFFFE0) == 0x00000000)
                op_code = op_code | ((token & 0x00007F00) << 17)
                                  | ((value & 0x0000001F) << 20);
                                 // Note '1F' mask needs to be '3F' for RV64 @@@
              else
                error_code = SYM_OORANGE;                  /* Not valid value */
              }
            else
              {
              if (((value & 0xFFFFF800) == 0x00000000)
               || ((value & 0xFFFFF800) == 0xFFFFF800))
                op_code = op_code | ((value & 0x00000FFF) << 20);
              else
                error_code = SYM_OORANGE;                  /* Not valid value */
              }
            }
          break;


        case 0x00030000:                                            /* B-type */
          {
          boolean pseudo;

          pseudo = (token & 0xF0000000) != 0;
          op_code = ((token & 0x01F00000) >> 18)
                  | ((token & 0x0E000000) >> 13) | 0x00000003;

          if ((reg = get_reg(line, &position)) >= 0)
            {
            if (pseudo) op_code = op_code | (reg << 20);
            else        op_code = op_code | (reg << 15);
            if (!cmp_next_non_space(line, &position, 0, ','))
              error_code = SYM_NO_COMMA | position;
            }
          else
            error_code = SYM_BAD_REG | position;

          if (error_code == eval_okay)
            {
            if ((reg = get_reg(line, &position)) >= 0)
              {
              if (pseudo) op_code = op_code | (reg << 15);
              else        op_code = op_code | (reg << 20);
              if (!cmp_next_non_space(line, &position, 0, ','))
                error_code = SYM_NO_COMMA | position;
              }
            else
              error_code = SYM_BAD_REG | position;
            }

          if (error_code == eval_okay)                  /* Now the offset ... */
            op_code = op_code | b_offset();
          }
          break;


        case 0x00040000:                                            /* S-type */
          {
          unsigned int prefix;                  /* Any additional instruction */

          prefix  = 0x00000000;                              /* Illegal value */
          op_code = ((token & 0x01F00000) >> 18)
                  | ((token & 0x0E000000) >> 13) | 0x00000003;

          if ((reg = get_reg(line, &position)) >= 0)
            {
            op_code = op_code | (reg << 20);
            if (!cmp_next_non_space(line, &position, 0, ','))
              error_code = SYM_NO_COMMA | position;
            }
          else
            error_code = SYM_BAD_REG | position;

          if (error_code == eval_okay) value = addr_mode(op_code, &prefix);
          if (value >= 0)
            {
            op_code = op_code | ((value & 0xFE0) << 20)             /* Offset */
                              | ((value & 0x01F) <<  7)
                              | ((value & 0x001F0000) >> 1);      /* Register */
            }

          if (prefix != 0) prefix_dump(prefix);      /* A bit bodgy still @@@ */
          }
          break;


        case 0x00050000:                                             /* Loads */
          {                                                      /*  and JALR */
          boolean pseudo_jalr;
          unsigned int prefix;                  /* Any additional instruction */

          pseudo_jalr = FALSE;
          prefix      = 0x00000000;                          /* Illegal value */
          op_code = ((token & 0x01F00000) >> 18)
                  | ((token & 0x0E000000) >> 13) | 0x00000003;

          if ((reg = get_reg(line, &position)) >= 0)
            {
            if (cmp_next_non_space(line, &position, 0, ','))
              op_code = op_code | (reg << 7);
            else
              if ((token & 0x0FF00000) == 0x01900000)      /* JALR pseudo-op? */
                {                                /* (Lacks explicit register) */
                pseudo_jalr = TRUE;
                op_code = op_code | (reg << 15) | 0x080;      /* Link with x1 */
                }
              else
                error_code = SYM_NO_COMMA | position;
            }
          else
            error_code = SYM_BAD_REG | position;
          if (error_code == eval_okay)
            {
            if (!pseudo_jalr)                       /* Deal with other fields */
              {
              value = addr_mode(op_code, &prefix);
              if (value >= 0)
                {
                op_code = op_code | ((value & 0xFFF) << 20)         /* Offset */
                                  | ((value & 0x001F0000) >> 1);  /* Register */
                }
              }
            }

          if (prefix != 0) prefix_dump(prefix);      /* A bit bodgy still @@@ */
          }
          break;


        case 0x00060000:                                            /* R-type */
          op_code = ((token & 0x01F00000) >> 18)
                  | ((token & 0x0E000000) >> 13)
                  | ((token & 0x00007F00) << 17) | 0x00000003;
          if ((reg = get_reg(line, &position)) >= 0)              /* Parse rd */
            {
            op_code = op_code | (reg << 7);
            if (!cmp_next_non_space(line, &position, 0, ','))
              error_code = SYM_NO_COMMA | position;
            }
          else
            error_code = SYM_BAD_REG | position;

          if (error_code == eval_okay)                           /* Parse rs1 */
            {
            if ((reg = get_reg(line, &position)) >= 0)
              {
              op_code = op_code | (reg << 15);
              if (!cmp_next_non_space(line, &position, 0, ','))
                error_code = SYM_NO_COMMA | position;
              }
            else
              error_code = SYM_BAD_REG | position;
            }

          if (error_code == eval_okay)                           /* Parse rs2 */
            {
            if ((reg = get_reg(line, &position)) >= 0)
              {
              op_code = op_code | (reg << 20);
              }
            else
              error_code = SYM_BAD_REG | position;
            }
          break;


        case 0x00070000:                                       /* Environment */
          op_code = (token & 0xFFF00000) | 0x00000073;
          break;


        case 0x00080000:                                             /* FENCE */
          { 
          unsigned int fields;

          unsigned int hedge()
            {          /* Well, it's the fence around the field ... kinda :-} */
            unsigned int fields;
            boolean done;

            fields = 0;
            done = FALSE;
            while (!done)
              {
              position = skip_spc(line, position);
              switch (line[position] & 0xDF)
                {
                case 'I': fields |= 8; position++; break;
                case 'O': fields |= 4; position++; break;
                case 'R': fields |= 2; position++; break;
                case 'W': fields |= 1; position++; break;
                default: done = TRUE; break;
                }
              }
            return fields;
            }

          op_code = 0x0000000F | ((token & 0x0E000000) >> 13);
          if ((token & 0x0E000000) == 0x00000000)            /* Plain 'FENCE' */
            {
            fields = hedge();
            if (fields == 0)
              op_code = op_code | 0x0FF00000;           /* Nothing means all! */
            else
              {
              op_code = op_code | fields << 24;
              if (!cmp_next_non_space(line, &position, 0, ','))
                error_code = SYM_NO_COMMA | position;
              else
                {
                fields = hedge();
                if (fields == 0)
                  error_code = SYM_ERR_SYNTAX | position;
						// Better message pending
                else
                  op_code = op_code | fields << 20;
                }
              }
            }
          break;
          }

        case 0x00090000:                                        /* Pseudo-ops */
          {
          switch(token & 0x00F00000)
            {
            case  0x00000000:
              switch(token & 0xF0000000)
                {
                case  0x00000000: op_code = 0x00000013; break;         /* NOP */
                case  0x10000000: op_code = 0x00008067; break;         /* RET */
                case  0x20000000: op_code = 0x00000000; break;     /* ILLEGAL */
                default: op_code = 0x00000013; break;
                }
              break;

            case  0x00100000:
              if ((reg = get_reg(line, &position)) < 0)                 /* JR */
                error_code = SYM_BAD_REG | position;
              else
                {
                switch(token & 0xF00000000)
                  {
                  case  0x00000000: op_code = 0x00000067 | reg << 15; break;
                  default: op_code = 0x00000013; break;
                  }
                }
              break;

            case  0x00200000:                                            /* J */
              error_code = evaluate(line, &position, &value, symbol_table);
              if (allow_error(error_code, first_pass, last_pass))
                error_code = eval_okay;
              value = value - assembly_pointer;
              if (((value & 0xFFF00000) == 0x00000000)       /* Out of range? */
               || ((value & 0xFFF00000) == 0xFFF00000))
                op_code = ((value & 0x100000) << 11)              /* Really?! */
                        |  (value & 0x0FF000)
                        | ((value & 0x000800) << 9)
                        | ((value & 0x0007FE) << 20);
              else error_code = SYM_OORANGE_BRANCH;

              switch(token & 0xF00000000)
                {
                case  0x00000000: op_code = op_code | 0x0000006F; break; /* J */
                default: op_code = 0x00000013; break;
                }
              break;

            case  0x00300000:                                 /* MV, NOT etc. */
              if ((reg = get_reg(line, &position)) < 0)
                error_code = SYM_BAD_REG | position;
              else
                if (!cmp_next_non_space(line, &position, 0, ','))
                  error_code = SYM_NO_COMMA | position;
//printf("Token: %08X\n", token);
              if (error_code == eval_okay)
                {
                op_code = reg << 7;                          /* Start with Rd */
                if ((reg = get_reg(line, &position)) < 0)         /* Parse Rs */
                  error_code = SYM_BAD_REG | position;
                else
                  {
                  op_code = op_code | reg << 15 | reg << 20;   /* BOTH fields */
                  switch (token & 0xF0000000)
                    {
                    case 0x00000000:                                    /* MV */
                      op_code = op_code & 0x000FFF80 | 0x00000013; break;
                    case 0x10000000:                                   /* NOT */
                      op_code = op_code & 0x000FFF80 | 0xFFF04013; break;
                    case 0x20000000:                                   /* NEG */
                      op_code = op_code & 0x01F07F80 | 0x40000033; break;
                    case 0x30000000:                                  /* NEGW */
                      op_code = op_code & 0x01F07F80 | 0x4000003B; break;
                    case 0x40000000:                                /* SEXT.W */
                      op_code = op_code & 0x000FFF80 | 0x0000001B; break;
                    case 0x50000000:                                  /* SEQZ */
                      op_code = op_code & 0x000FFF80 | 0x00103013; break;
                    case 0x60000000:                                  /* SNEZ */
                      op_code = op_code & 0x01F07F80 | 0x00003033; break;
                    case 0x70000000:                                  /* SLTZ */
                      op_code = op_code & 0x000FFF80 | 0x00002033; break;
                    case 0x80000000:                                  /* SGTZ */
                      op_code = op_code & 0x01F07F80 | 0x00002033; break;
                    default: op_code = 0x00000013; break;
                    }
                  }
                }
              break;

            case  0x00400000:                                     /* Branches */
              if ((reg = get_reg(line, &position)) < 0)
                error_code = SYM_BAD_REG | position;
              else
                if (!cmp_next_non_space(line, &position, 0, ','))
                  error_code = SYM_NO_COMMA | position;
//printf("Token: %08X\n", token);
              if (error_code == eval_okay)
                op_code = 0x00000063 | (token & 0x0E000000) >> 13 | b_offset();

              if (error_code == eval_okay)     /* Code may have changed above */
                if ((token & 0x01000000) != 0)
                  op_code = op_code | reg << 20;
                else
                  op_code = op_code | reg << 15;
              break;

            case  0x00500000:                                         /* CSRs */
              op_code = ((token & 0x0E000000) >> 13) |  0x00000073;
              if ((token & 0x20000000) != 0)
                {
                if ((reg = get_reg(line, &position)) < 0)         /* Parse rd */
                  error_code = SYM_BAD_REG | position;
                else
                  {
                  op_code = op_code | reg << 7;
                  if (!cmp_next_non_space(line, &position, 0, ','))
                    error_code = SYM_NO_COMMA | position;
                  }

                if (error_code == eval_okay)                     /* Parse CSR */
                  op_code = op_code | parse_csr();
                }
              else
                {
                op_code = op_code | parse_csr();              /* Parse Rs/imm */
                if (error_code == eval_okay)
                  {
                  if (!cmp_next_non_space(line, &position, 0, ','))
                    error_code = SYM_NO_COMMA | position;
                  else
                    op_code = op_code | parse_csr2(token);
                  }
                }
              break;

            case  0x00600000:                       /* Explicit counter reads */
              op_code = (token & 0xFF000000) | 0x00000073
                     | ((token & 0x0000F000) << 8)
                     | ((token & 0x00000700) << 4);
              if ((reg = get_reg(line, &position)) < 0)   /* Parse first reg. */
                error_code = SYM_BAD_REG | position;
              else
                if ((token & 0x00000700) == 0x00000200)
                  op_code = op_code | reg << 7;                   /* Rd field */
                else
                  {
                  if (!cmp_next_non_space(line, &position, 0, ','))
                    op_code = op_code | reg << 15;                /* Rs field */
                  else
                    {
                    op_code = op_code | reg << 7;                 /* Rd field */
                    if ((reg = get_reg(line, &position)) < 0)
                      error_code = SYM_BAD_REG | position;
                    else
                      op_code = op_code | reg << 15;              /* Rs field */
                    }
                  }
              break;

            default: op_code = 0x00000013; break;
            }
          break;
          }


        case 0x000A0000:                                         /* CSR stuff */
          op_code = ((token & 0x01F00000) >> 18)
                  | ((token & 0x0E000000) >> 13) | 0x00000003;
          if ((reg = get_reg(line, &position)) >= 0)              /* Parse rd */
            {
            op_code = op_code | (reg << 7);
            if (!cmp_next_non_space(line, &position, 0, ','))
              error_code = SYM_NO_COMMA | position;
            }
          else
            error_code = SYM_BAD_REG | position;

          if (error_code == eval_okay)                           /* Parse CSR */
            op_code = op_code | parse_csr();

          if (error_code == eval_okay)
            if (!cmp_next_non_space(line, &position, 0, ','))
              error_code = SYM_NO_COMMA | position;

          if (error_code == eval_okay)                        /* Parse Rs/imm */
            op_code = op_code | parse_csr2(token);

          break;


        case 0x000B0000:                   /* Long/variable length pseudo-ops */
          {
          boolean flag;                 /* Will require two instructions flag */
          unsigned int prefix;                  /* Any additional instruction */

          if ((token & 0x00002000) == 0)
            {
            if ((reg = get_reg(line, &position)) >= 0)            /* Parse Rd */
              {
              prefix  = (reg << 7);
              op_code = (reg << 7) | (reg << 15);
              if (!cmp_next_non_space(line, &position, 0, ','))
                error_code = SYM_NO_COMMA | position;
              }
            else
              error_code = SYM_BAD_REG | position;
            }
          else
            {
            prefix  =  (token & 0x01F00000) >> 13;
            op_code = ((token & 0x01F00000) >> 5)|((token & 0x3E000000) >> 18);
            }

          if (error_code == eval_okay)                           /* Immediate */
            {
            flag = TRUE;                            /* 'Needs two words' flag */
            error_code = evaluate(line, &position, &value, symbol_table);
            if (allow_error(error_code, first_pass, last_pass))
              error_code = eval_okay;

            if ((token & 0x00007000) != 0x00000000)         /* If PC relative */
              value = value - assembly_pointer;              /* (Yucky hack.) */

            prefix = prefix | ((value + 0x00000800) & 0xFFFFF000); /* Default */
            op_code = op_code | ((value << 20) & 0xFFF00000);

            switch (token & 0x00007000)
              {
              case 0x00000000:                                       /* LI    */
                prefix  = prefix  | 0x00000037;                      /* LUI   */
                op_code = op_code | 0x00000013;                      /* ADDI  */
                if (((value + 0x800) & 0xFFFFF000) == 0)            /* Range? */
                  {
                  flag = FALSE;                /* Optimise to one instruction */
                  op_code = op_code & ~0x000F8000;          /* Zero RS1 field */
                  }
                else if ((value & 0x00000FFF) == 0)          /* Just LUI form */
                  {
                  flag = FALSE;                /* Optimise to one instruction */
                  op_code = (prefix & 0x00000FFF) | value;        /* LUI only */
                  }
                break;

              case 0x00001000:                                       /* LA    */
                prefix  = prefix  | 0x00000017;                      /* AUIPC */
                op_code = op_code | 0x00000013;                      /* ADDI  */
                break;

              case 0x00002000:                                       /* CALL  */
                if ((value & 1) != 0)                 /* Not halfword aligned */
          // Insert trap for word alignment if 16-bit instructions disabled @@@
                  error_code = SYM_UNALIGNED_BRANCH;
                else
                  {
                  if (((value + 0x100000) & 0xFFE00000) != 0)        /* Range?*/
                    {                                                /* Long  */
                    prefix  = prefix  | 0x00000017;                  /* AUIPC */
                    op_code = op_code | 0x00000067;                  /* JALR  */
                    }
                  else
                    {
                    op_code = 0x000000EF | imm_jal(value);          /* JAL X1 */
                    flag = FALSE;              /* Optimise to one instruction */
                    }
                  }
                break;
              default: break;
              }
            }

//printf("Token %08X  Value %08X  Flag %d\n", token, value, flag);
          if (variable_item_size(first_pass, flag ? 8 : 4) == 8) flag = TRUE;

          if (flag)                              /* I.e. not been 'optimised' */
            {
            if (last_pass) byte_dump(assembly_pointer, prefix, line, 4);
            assembly_pointer += 4;
            }
          }
          break;


        case 0x000C0000:                                           /* Atomics */
          op_code = ((token & 0x07F00000) << 5)
                  | ((token & 0x08000000) >> 15) | 0x0000202F;
          if ((reg = get_reg(line, &position)) >= 0)              /* Parse Rd */
            {
            op_code = op_code | (reg << 7);
            if (!cmp_next_non_space(line, &position, 0, ','))
              error_code = SYM_NO_COMMA | position;
            }
          else
            error_code = SYM_BAD_REG | position;

          if (((token &0x07C00000) != 0x00800000) && (error_code == eval_okay))
            {                                         /* Excludes LR variants */
            if ((reg = get_reg(line, &position)) >= 0)           /* Parse Rs2 */
              {
              op_code = op_code | (reg << 20);
              if (!cmp_next_non_space(line, &position, 0, ','))
                error_code = SYM_NO_COMMA | position;
              }
            else
              error_code = SYM_BAD_REG | position;
            }

          if (error_code == eval_okay)
            {
            if (!cmp_next_non_space(line, &position, 0, '['))
              error_code = SYM_NO_LBR | position;
            else
              if ((reg = get_reg(line, &position)) >= 0)         /* Parse Rs1 */
                {
                op_code = op_code | (reg << 15);
                if (!cmp_next_non_space(line, &position, 0, ']'))
                error_code = SYM_NO_RBR | position;
                }
              else
                error_code = SYM_BAD_REG | position;
            }
          break;


        default:  
          printf("Unprocessable opcode!\n");
          break;
        }

      }                                                      /* end of -skip- */

    if (error_code == eval_okay)
      {
      if (last_pass) byte_dump(assembly_pointer + extras, op_code, line, 4);
      }
    else
      {
      if (!last_pass)
        {
        if (allow_error(error_code, first_pass, last_pass))
          error_code = eval_okay;                       /* Pretend we're okay */
        }
      else                                             /* Error on final pass */
        byte_dump(assembly_pointer + extras, 0, line, 4);
                                              /* Dump 0x00000000 place holder */
      }
//##
if (if_stack[if_SP])
    assembly_pointer = assembly_pointer + 4 + extras;

    return;
    }

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Separated out so other functions (e.g. "ARM") can call it too.             */

  void do_align()
    {
    int fill;

    error_code = evaluate(line, &position, &operand, symbol_table);
    if ((error_code & 0xFFFFFF00) == eval_no_operand)      /* Code - position */
      {
      error_code = eval_okay;
      operand = 4;
      }

    if (error_code == eval_okay)
      {
      if (operand != 0)
        {                                          /* (ALIGN 0 has no effect) */
        temp = (assembly_pointer - 1) % operand;
        operand = operand - (temp + 1);     /* No. of elements to skip(/fill) */
        }

      if (fill = (line[position] == ','))     /*Note where any label should go*/
        fill_space(operand);                           /* Fill with value (?) */
      else                                /* Start new section if leaving gap */
        {
        if (fList != NULL) list_start_line(assembly_pointer+operand, FALSE);
                                                  /* Revise list file address */

        if (operand != 0) elf_new_section_maybe(); /* Only reorigin in needed */
        }
      }

    if (error_code == eval_okay)                                 /* Still OK? */
      {
      if (fill)                               /* Any label is at source point */
        assemble_redef_label(assembly_pointer,
                           assembly_pointer_defined,
                        my_label, &error_code, 0, pass_count, last_pass, line);
      else                                    /* Any label is after alignment */
        assemble_redef_label(assembly_pointer + operand,
                           assembly_pointer_defined,
                        my_label, &error_code, 0, pass_count, last_pass, line);
//##
if (if_stack[if_SP])
      assembly_pointer = assembly_pointer + operand;
      }

    return;
    }

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

error_code = eval_okay;        /* #DEFINE ??   @@@@ */

first_pass = (pass_count == 0);
evaluate_own_label = my_label;                                  /* Yuk! @@@@@ */
                        /* Global, to pass local label definition to evaluate */

if (((token & 0xF4000000) != 0xF4000000) && (my_label->sort == MAYBE_SYMBOL))
  sym_add_to_table(symbol_table, my_label->symbol);        /* Must be a label */

if (((token & 0xF8000000) != 0xF8000000) && (my_label->sort != NO_LABEL))
           /* Redefine label if present/required unless directive such as EQU */
    assemble_redef_label(assembly_pointer, assembly_pointer_defined, my_label,
                        &error_code, 0, pass_count, last_pass, line);

def_increment = 0;                  /* Default to first position/item on line */

if (error_code == eval_okay)
  {	//###	Check this error trap correct.

  if ((token & 0xF0000000) == 0xF0000000)
    {                                                            /* Directive */
    switch (token)
      {                                                      /* Defining code */
      case 0xF0000000: assemble_define(1); break;                     /* DEFB */
      case 0xF0010000: assemble_define(2); break;                     /* DEFH */
      case 0xF0020000: assemble_define(4); break;                     /* DEFW */
      case 0xF0030000:                                                /* DEFS */
        error_code = evaluate(line, &position, &operand, symbol_table);
        if (allow_error(error_code, first_pass, last_pass))
          error_code = eval_okay;

        if (error_code == eval_okay)
          {
          if (line[position] == ',') fill_space(operand);/*Fill with value (?)*/
          else                        /* Reorigin ELF by starting new section */
            if (operand != 0) elf_new_section_maybe();  /* Reorigin in needed */

//##
if (if_stack[if_SP])
          if (error_code == eval_okay) assembly_pointer += operand;/*Still OK?*/
          }
        break;

      case 0xF0040000:                                              /* EXPORT */
        {
        boolean terminate;
        int i;
        char ident[LINE_LENGTH];
        sym_record *symbol;

        terminate = FALSE;

        while (!terminate)
          {
          position = skip_spc(line, position);
          if ((i = get_identifier(line, position, ident, LINE_LENGTH)) > 0)
            {
            if (last_pass)                /* Only care when about to complete */
              {
              if ((i == 3) && ((ident[0] & 0xDF) == 'A')        /* Bodgy test */
              && ((ident[1] & 0xDF) == 'L') && ((ident[2] & 0xDF) == 'L'))
                symbol_table->flags |= SYM_TAB_EXPORT_FLAG; /*Mark whole table*/
              else
                {
                if ((symbol = sym_find_label(ident, symbol_table)) != NULL)
                  symbol->flags |= SYM_REC_EXPORT_FLAG;
                else
                  error_code = SYM_ERR_NO_EXPORT;
                }
              }
            position = position + i;
            }
          else
            {
            error_code = SYM_ERR_SYNTAX | position;
            terminate  = TRUE;
            }
          if (!cmp_next_non_space(line, &position, 0, ',')) terminate = TRUE;
          }
        }
        break;

      case 0xF0050000:                                             /* INCLUDE */
        {			// Allow " " around name?  @@@
        int i;

        position = skip_spc(line, position);
        if (test_eol(line[position]))                       /* Effective EOL? */
          {
          error_code = SYM_ERR_NO_FILENAME | position;    /* Filename missing */
          }
        else
          {                                 /* Got name; make and fill buffer */
          *include_name = (char*) malloc(LINE_LENGTH+1); /* Overkill-so what? */
          i = 0;
          while (!test_eol(line[position])
                       && (line[position] != ' ') && (line[position] != '\t'))
            (*include_name)[i++] = line[position++];
          (*include_name)[i] = '\0';             /* Terminate filename string */
          }
        }
        break;

      case 0xF0070000:                                                /* ARCH */
        {
        int arch;
        arch = get_thing(line, &position, arch_table);
        if (arch >= 0) rv_variant = arch;
        else           error_code = SYM_ERR_SYNTAX | position;
        }
        break;

      case 0xF0080000:                                               /* ENTRY */
        if (!entry_address_defined)
         {
         entry_address = assembly_pointer;
         entry_address_defined = TRUE;
         }
        else error_code = SYM_DOUBLE_ENTRY;
        break;

      case 0xF0090000:                                                 /* ARM */
//      instruction_set = ARM;
//      do_align();      /* Automatic realignment (correct choice of action?) */
        break;	// Kept in case want to have 'compressed' flag/mode @@@

      case 0xF00A0000:                                               /* THUMB */
//      instruction_set = THUMB;
        break;

      case 0xF00B0000:                                    /* <option removed> */
        break;


      case 0xF00C0000:                                                  /* IF */
        {
        char name[SYM_NAME_MAX];	// @@@ Size of buffer (also below)
        int j, condition;

        if (if_SP >= IF_STACK_SIZE) error_code = SYM_MANY_IFS;
        else
          {
          error_code = evaluate(line, &position, &condition, symbol_table);
          if (error_code != eval_okay)
            {
//## printf("IF: error %08X\n", error_code);
            condition = -1;
            if ((error_code & 0xFFFFFF00) == eval_no_label)  /* Improve error */
              error_code = eval_no_label_yet | (error_code & 0xFF);/* message */
            }
          if_stack[++if_SP] = condition;
//## printf("IF: Push value %08X\n", condition);
//            }
//          else
//            error_code = SYM_NO_IDENTIFIER | position;
          }
        }
        break;

      case 0xF00D0000:                                                  /* FI */
        if (if_SP > 0) if_SP--; else error_code = SYM_MANY_FIS;
//## printf("IF: Pop value\n");
        break;

      case 0xF00E0000:                                                /* ELSE */
        if (if_SP > 0) if_stack[if_SP] = ~if_stack[if_SP];
        else error_code = SYM_LOST_ELSE;
//## printf("IF: flip value %08X\n", if_stack[if_SP]);
        break;

      case 0xF00F0000:                                              /* IMPORT */
        {			// Allow " " around name?  @@@
        int  i;
        char *import_name;                 /* Name extracted from source file */
        char *import_full_name;                   /* Path to file from 'here' */
        FILE *import_handle;
        char byte;

        position = skip_spc(line, position);
        if (test_eol(line[position]))                       /* Effective EOL? */
          {
          error_code = SYM_ERR_NO_FILENAME | position;    /* Filename missing */
          }
        else
          {                                 /* Got name; make and fill buffer */
          import_name = (char*) malloc(LINE_LENGTH+1);   /* Overkill-so what? */
          i = 0;
          while (!test_eol(line[position])
                       && (line[position] != ' ') && (line[position] != '\t'))
            import_name[i++] = line[position++];
          import_name[i] = '\0';                 /* Terminate filename string */

          if (import_name[0] == '/')               /* Absolute path specified */
            import_full_name = import_name;
          else                           /* Add path from invocation position */
            import_full_name = pathname(include_file_path, import_name);

          if (import_name[0] != '\0')
            {
            import_handle = fopen(import_full_name, "r");// Ignores errors if any  @@@
            if (import_handle != NULL)
              {
              while (!feof(import_handle))
                {
                byte = getc(import_handle);

                if (!feof(import_handle))
                  {
                  if (last_pass)
                    byte_dump(assembly_pointer + def_increment, byte, line, 1);
                  def_increment++;
                  }
                }

              assembly_pointer += def_increment;     /* Add total size at end */
              fclose(import_handle);
              }
            else
              {
              error_code = SYM_NO_IMPORT;                 /* Filename missing */
              }
            }
          if (import_full_name != import_name) free(import_full_name);
          free(import_name);
          }
        }
        break;


                                                            /* Defining label */
      case 0xF8000000:                                                 /* EQU */
      case 0xF8030000:                                                 /* DEF */
        error_code = evaluate(line, &position, &temp, symbol_table);
        if (my_label->symbol != NULL)
          {
          if (token == 0xF8000000)
            assemble_redef_label(temp, TRUE, my_label, &error_code,
                                 SYM_REC_EQU_FLAG, pass_count, last_pass, line);
          else
            assemble_redef_label(temp, TRUE, my_label, &error_code,
                                 SYM_REC_USR_FLAG, pass_count, last_pass, line);
          }
        else
          error_code = SYM_ERR_NO_EQU;

        break;

      case 0xF8010000:                                                 /* ORG */
        error_code = evaluate(line, &position, &assembly_pointer, symbol_table);
        assembly_pointer_defined = (error_code == eval_okay);
                                                 /* Result may be `undefined' */
        if (allow_error(error_code, first_pass, last_pass))
          error_code = SYM_NO_ERROR;/* ORG undefined -itself- is not an error */
        if (fList != NULL) list_start_line(assembly_pointer, FALSE);
                                                  /* Revise list file address */
        assemble_redef_label(assembly_pointer, assembly_pointer_defined,
                         my_label, &error_code, 0, pass_count, last_pass, line);

        elf_new_section_maybe();       /* else reuse previous (unused) number */
        break;

      case 0xF8020000:                                               /* ALIGN */
        do_align();
        break;

      case 0xF8040000:                                              /* RECORD */
        error_code = evaluate(line, &position, &temp, symbol_table);
        if (((error_code & 0xFFFFFF00) == eval_no_operand)
           || allow_error(error_code, first_pass, last_pass))
          {
          temp = 0;             /* If no operand found then assume zero start */
          error_code = eval_okay;
          }

        if (error_code == eval_okay)
          {
          data_pointer = temp;
          assemble_redef_label(data_pointer, TRUE, my_label, &error_code,
                               SYM_REC_DATA_FLAG, pass_count, last_pass, line);
          }
        break;

      case 0xF8050000:                                           /* REC_ALIGN */
        error_code = evaluate(line, &position, &temp, symbol_table);
        if (((error_code & 0xFFFFFF00) == eval_no_operand)
           || allow_error(error_code, first_pass, last_pass))
          {
          temp = 4;
          error_code = eval_okay;
          }

        if (error_code == eval_okay)
          {
          if (temp != 0) data_pointer = data_pointer-(data_pointer%temp)+temp;
                                              /* Any label is after alignment */
          assemble_redef_label(data_pointer, TRUE, my_label, &error_code,
                                SYM_REC_DATA_FLAG, pass_count, last_pass, line);
          }
        break;

      case 0xF8100000:                                            /*    ALIAS */
      case 0xF8110000:                                            /*     BYTE */
      case 0xF8120000:                                            /* HALFWORD */
      case 0xF8140000:                                            /*     WORD */
      case 0xF8180000:                                            /*   DOUBLE */
        {
        unsigned int size;

        size = (token >> 16) & 0xF;                    /* Size of one element */
        error_code = evaluate(line, &position, &temp, symbol_table);
        if (((error_code & 0xFFFFFF00) == eval_no_operand)
           || allow_error(error_code, first_pass, last_pass))
          {
          temp = 1;            /* If no operand found then assume one element */
          error_code = eval_okay;
          }

        if (error_code == eval_okay)
          {
          assemble_redef_label(data_pointer, TRUE, my_label, &error_code,
                               SYM_REC_DATA_FLAG, pass_count, last_pass, line);
          data_pointer = data_pointer + (temp * size);
          }

        }
        break;

      case 0xF4000000:                                                  /* RN */
        if (first_pass)                     /* Must be resolved on first pass */
          {
          if (my_label->sort == MAYBE_SYMBOL)
            {
            if ((my_label->symbol->value = get_reg(line, &position)) >= 0)
              redefine_symbol(line, my_label->symbol, register_table);
            else
              error_code = SYM_BAD_REG | position;
            }
          else error_code = SYM_ERR_NO_EQU;
          }
        break;


      default:
        error_code = SYM_ERR_BROKEN;
        break;
      }
    }
  else
    {
    rv32i_mnemonic();
    }

  }//  ###
if (first_pass && (error_code == eval_okay))
  {                   /* Check that nothing remains on line (first pass only) */
  position = skip_spc(line, position);
  if (!test_eol(line[position])) error_code = SYM_GARBAGE | position;
  }

return error_code;
}

/*----------------------------------------------------------------------------*/
/* Fully parameterised utility routines                                       */

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Look up a value in a symbol table.                                         */
/* Intended to recover positive values only; returns -1 if not found.         */

#define THING_BUFFER_LENGTH  16

int get_thing(char *line, unsigned int *pos, sym_table *table)
{
int i, result;
char buffer[THING_BUFFER_LENGTH];
sym_record *ptr;

*pos = skip_spc(line, *pos);
result = -1;                                                /* Not found code */

if ((i = get_identifier(line, *pos, buffer, THING_BUFFER_LENGTH)) > 0)
  {                                                        /* Something taken */
  if ((ptr = sym_find_label(buffer, table)) != NULL)
    {                                                    /* Symbol recognised */
    result = ptr->value;
    *pos += i;
    }
  }

return result;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

int get_reg(char *line, unsigned int *pos)	/* Expand into code? @@@@ */
{ return get_thing(line, pos, register_table); }

/*----------------------------------------------------------------------------*/

unsigned int imm_jal(unsigned int value)
{ return ((value & 0x100000) << 11) |  (value & 0x0FF000)
       | ((value & 0x000800) << 9)  | ((value & 0x0007FE) << 20); }

/*----------------------------------------------------------------------------*/
/* Refetch first identifier from source line (in case it was truncated) and   */
/* re-hash into old symbol with appropriate rules.                            */

void redefine_symbol(char *line, sym_record *record, sym_table *table)
{
char ident[LINE_LENGTH];

get_identifier(line, skip_spc(line, 0), ident, LINE_LENGTH);
sym_string_copy(ident, record, table->flags);
sym_add_to_table(table, record);

return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Redefine a label on the current line                                       */

void assemble_redef_label(unsigned int value, int defined, own_label *my_label,
                         unsigned int *error_code, int type_change,
                         int pass_count, boolean last_pass, char *line)
{
int value_defined;                                  /* Genuine value supplied */
unsigned int old_value;
int flags;

if (my_label->sort != NO_LABEL)              /* Reject cases that don't apply */
  {
  if ((my_label->sort == SYMBOL) || (my_label->sort == MAYBE_SYMBOL))
    {                            /* Symbolic (ordinary) label on current line */
    old_value = my_label->symbol->value;
    flags     = my_label->symbol->flags;
    }
  else if (my_label->sort == LOCAL_LABEL)
    {                                          /* Local label on current line */
    old_value = my_label->local->value;
    flags     = my_label->local->flags;
    }

  value_defined = (*error_code == eval_okay) && defined; // Clumsy - 2 parameters @@

  if ((pass_count != (flags & 0xFF))            /* First encounter this pass? */
   || ((flags & SYM_REC_DEF_FLAG) == 0))                 /*  or was undefined */
    {                                               /* Treat as new encounter */
    if (value_defined)                          /* New value is defined value */
      {
      if (((flags & SYM_REC_DEF_FLAG) == 0)                  /* Undefined ... */
        || (old_value != value))                            /*  ...or changed */
        {
        if ((flags & SYM_REC_DEF_FLAG) == 0) defined_count++;    /* Undefined */
        else                               redefined_count++;
                                                        /* Note what was done */
        flags |= SYM_REC_DEF_FLAG;                         /* Mark as defined */
        }
      }
    else
      {
      flags &= ~SYM_REC_DEF_FLAG;                  /* Mark label as undefined */

      if (allow_error(*error_code, pass_count==0, last_pass))
        *error_code = SYM_NO_ERROR; 	          // But flag up something @@@@
      }
    }
  else
    {                                  /* Repeat encounter with defined label */
    if (!value_defined || (value != old_value))                 /* Different! */
      *error_code = SYM_INCONSISTENT;
    }

  flags = (flags & 0xFFFFFF00) | pass_count;

  if ((my_label->sort == SYMBOL) || (my_label->sort == MAYBE_SYMBOL))
    {                            /* Symbolic (ordinary) label on current line */
    my_label->symbol->value = value;
    my_label->symbol->flags = flags;
    }
  else if (my_label->sort == LOCAL_LABEL)
    {                                          /* Local label on current line */
    my_label->local->value = value;
    my_label->local->flags = flags;
    }

  if (type_change != 0)                        /* Maybe want to override type */
    {
    if (my_label->sort == LOCAL_LABEL)
      my_label->local->flags  |= type_change;             /* `type' indicator */
    else
      my_label->symbol->flags |= type_change;             /* `type' indicator */
    }

  }

return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void byte_dump(unsigned int address, unsigned int value, char *line, int size)
{
int i;

//## printf("Should I? : %d %08X %08X %d\n", if_SP, if_stack[if_SP], value, size);

if (dump_code && if_stack[if_SP])
  {
  if (fList != NULL) list_mid_line(value, line, size);

  for (i = 0; i < size; i++)
    {
    if (fHex != NULL) hex_dump(address + i, (value >> (8*i)) & 0xFF);
    if (fElf != NULL) elf_dump(address + i, (value >> (8*i)) & 0xFF);
    if (fVerilog != NULL)
      Verilog_array[(address + i) % VERILOG_MAX] = (value >> (8*i)) & 0xFF;
    }

  }
return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

FILE *open_output_file(int std, char *filename)
{
if (std) return stdout;
else
  if (filename[0] != '\0') return fopen(filename, "w");	// Ignores errors if any  @@@
  else return NULL;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void close_output_file(FILE *handle, char *filename, int errors)
{
if ((handle != stdout) && (handle != NULL))
  {
  fclose(handle);
  if (errors) remove(filename);
  }
return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void hex_dump(unsigned int address, char value)
{
int i;

if (hex_address_defined && (address == hex_address))
  {                                                       /* Expected address */
  list_hex(value, 2, &hex_buffer[HEX_LINE_ADDRESS+3*(address%HEX_BYTE_COUNT)]);
  hex_address++;
  }
else
  {                                              /* New or unexpected address */
  if (hex_address_defined) fprintf(fHex, "%s\n", hex_buffer);

  for (i = 0; i < HEX_LINE_LENGTH - 1; i++) hex_buffer[i] = ' ';
  hex_buffer[i] = '\0';
  list_hex(address, 8, &hex_buffer[0]);
  hex_buffer[8] = ':';

  list_hex(value, 2, &hex_buffer[HEX_LINE_ADDRESS+3*(address%HEX_BYTE_COUNT)]);

  hex_address = address + 1;
  hex_address_defined = TRUE;
  }

if ((hex_address % HEX_BYTE_COUNT) == 0)              /* If end of line, dump */
  {
  fprintf(fHex, "%s\n", hex_buffer);
  hex_address_defined = FALSE;
  }

return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void hex_dump_flush(void)
{
if (hex_address_defined) fprintf(fHex, "%s\n", hex_buffer);
hex_address_defined = FALSE;
return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void elf_dump(unsigned int address, char value)
{
elf_temp *pTemp;

elf_section_valid = TRUE;    /* Note that we've dumped -something- in section */

if (elf_new_block || (elf_section != elf_section_old))
  {                                              /* New or unexpected address */
  pTemp = (elf_temp*) malloc(ELF_TEMP_SIZE);                /* Allocate block */
  pTemp->pNext        = NULL;                        /* Initialise new record */
  pTemp->continuation = (elf_section == elf_section_old);
  pTemp->section      = elf_section;
  pTemp->address      = address;
  pTemp->count        = 0;

  if (current_elf_record == NULL)
    {                                                         /* First record */
    elf_record_list = pTemp;
    pTemp->continuation = FALSE;
    }
  else current_elf_record->pNext = pTemp;

  current_elf_record = pTemp;                                      /* Move on */
  }

current_elf_record->data[(current_elf_record->count)++] = value;

elf_section_old = elf_section;

elf_new_block = (current_elf_record->count >= ELF_TEMP_LENGTH);

return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Start a new ELF section if one is already in use                           */

void elf_new_section_maybe(void)
{
if (elf_section_valid) { elf_section++; elf_section_valid = FALSE; }
return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void elf_dump_out(FILE *fElf, sym_table *table)
{

  void elf_dump_word(FILE *fElf, unsigned int word)
    {
    fprintf(fElf, "%c%c%c%c", word & 0xFF, (word>>8) & 0xFF,
                             (word>>16) & 0xFF, (word>>24) & 0xFF);
    return;
    }

  void elf_dump_SH(FILE *fElf, unsigned int name, unsigned int type,
    unsigned int flags, unsigned int addr, unsigned int pos, unsigned int size,
    unsigned int link, unsigned int info, unsigned int align, unsigned int size2)
    {
    elf_dump_word(fElf, name);   elf_dump_word(fElf, type);
    elf_dump_word(fElf, flags);  elf_dump_word(fElf, addr);
    elf_dump_word(fElf, pos);    elf_dump_word(fElf, size);
    elf_dump_word(fElf, link);   elf_dump_word(fElf, info);
    elf_dump_word(fElf, align);  elf_dump_word(fElf, size2);
    return;
    }


elf_temp *pTemp, *pTemp_old;
elf_info *pInfo_head, *pInfo, *pInfo_new;
unsigned int fragments, total, pad_to_align, i, j, temp;
unsigned int symtab_count, symtab_length, strtab_length, shstrtab_length;
unsigned int symtab_local_count;
unsigned int prog_start;
unsigned int code_SHstr_offset,   sym_SHstr_offset;
unsigned int  str_SHstr_offset, SHstr_SHstr_offset;
char *strings, *SHstrings;
sym_record *head, *ptr;

char *sym_sectionname = "symtab";                 /* Predefined section names */
char *str_sectionname = "strtab";
char *shs_sectionname = "shstrtab";


pInfo      = NULL;
pInfo_head = NULL;    /* Needed in case there's nothing to write (e.g. error) */
pTemp      = elf_record_list;
fragments  = 0;                                             /* Number of ORGs */
total      = 0;                                   /* Length of all code bytes */

while (pTemp != NULL)                     /* Make an code output section list */
  {
  if (!pTemp->continuation)
    {
    pInfo_new = (elf_info*) malloc(ELF_INFO_SIZE);          /* Allocate block */
    pInfo_new->pNext    = NULL;
    pInfo_new->address  = pTemp->address;
    pInfo_new->position = total;
    if (pInfo == NULL) pInfo_head   = pInfo_new;        /* Start ...          */
    else               pInfo->pNext = pInfo_new;        /* ... or add to list */

    pInfo = pInfo_new;
    pInfo->size = 0;
    fragments++;
    }
  pInfo->size  = pInfo->size + pTemp->count;
  total = total + pTemp->count;
  pTemp = pTemp->pNext;
  }

head = sym_sort_symbols(table, ALL, FOR_ELF);       /* Make temp. symbol list */

prog_start         = ELF_EHSIZE;
symtab_count       = sym_count_symbols(table, ALL) + 1;  /* Number of entries */
                                           /* " + 1" is for dummy first entry */
symtab_local_count = symtab_count - sym_count_symbols(table, EXPORTED);
strings            = sym_strtab(head, symtab_count, &strtab_length);

shstrtab_length = 1;                              /* Build shstrtab in memory */
i = 0; do { shstrtab_length++; } while (elf_file_name[i++]   != '\0');
i = 0; do { shstrtab_length++; } while (sym_sectionname[i++] != '\0');
i = 0; do { shstrtab_length++; } while (str_sectionname[i++] != '\0');
i = 0; do { shstrtab_length++; } while (shs_sectionname[i++] != '\0');
shstrtab_length = (shstrtab_length + 3) & 0xFFFFFFFC;

SHstrings = (char*) malloc(shstrtab_length);                /* Allocate block */
j = 0;                                                          /* Fill block */
SHstrings[j++]  = '\0';
code_SHstr_offset = j;   i = 0; 
do {SHstrings[j++] = elf_file_name[i];}   while (elf_file_name[i++]   != '\0');
sym_SHstr_offset = j;    i = 0;
do {SHstrings[j++] = sym_sectionname[i];} while (sym_sectionname[i++] != '\0');
str_SHstr_offset = j;    i = 0;
do {SHstrings[j++] = str_sectionname[i];} while (str_sectionname[i++] != '\0');
SHstr_SHstr_offset = j;  i = 0;
do {SHstrings[j++] = shs_sectionname[i];} while (shs_sectionname[i++] != '\0');
while ((j & 3)!=0) SHstrings[j++]='\0';                  /* Pad to word align */

pad_to_align    = -(total % 4) & 3;
total           = total + pad_to_align;                         /* Word align */
strtab_length   = (strtab_length + 3) & 0xFFFFFFFC;             /* Word align */
symtab_length   = 16 * symtab_count;                           /* True length */

elf_dump_word(fElf, 0x7F | ('E'<<8) | ('L'<<16) | ('F'<<24));  /* File header */
elf_dump_word(fElf, 0x00010101);
elf_dump_word(fElf, 0);
elf_dump_word(fElf, 0);
elf_dump_word(fElf, 2 + (ELF_MACHINE << 16));
elf_dump_word(fElf, 1);
elf_dump_word(fElf, entry_address);
elf_dump_word(fElf, prog_start + total + symtab_length + strtab_length
                    + shstrtab_length);
elf_dump_word(fElf, prog_start + total + symtab_length + strtab_length
                    + shstrtab_length + (ELF_PHENTSIZE * fragments));
elf_dump_word(fElf, 0);			// Flags @@@
elf_dump_word(fElf, ELF_EHSIZE      +   (ELF_PHENTSIZE << 16));
elf_dump_word(fElf, fragments       +   (ELF_SHENTSIZE << 16));
elf_dump_word(fElf, (fragments + 4) + ((fragments + 3) << 16));	// @@@

pTemp = elf_record_list;

while (pTemp != NULL)                               /* Dump the code sections */
  {
  for (i = 0; i < pTemp->count; i++) { fprintf(fElf, "%c", pTemp->data[i]); }
  pTemp = pTemp->pNext;
  }
for (i = 0; i < pad_to_align; i++) fprintf(fElf, "%c", 0);           /* Align */

                                             /* Symbol table - values et alia */
for (i = 0; i < 4; i++) elf_dump_word(fElf, 0);         /* Dummy first symbol */

ptr = head;
j   = 0;
for (i = 1; i < symtab_count; i++)
  {
  while (strings[j++] != '\0');                     /* Point beyond next '\0' */
  elf_dump_word(fElf, j);
  elf_dump_word(fElf, ptr->value);
  elf_dump_word(fElf, 0x00000000);

  if ((ptr->flags & SYM_REC_EXPORT_FLAG) == 0) temp = 0x00; else temp = 0x10;
                                                                   /* Binding */
  if ((ptr->flags & SYM_REC_EQUATED) == 0)
    elf_dump_word(fElf, (ptr->elf_section << 16) | temp);
  else                                          /* EQU, so regard as absolute */
    elf_dump_word(fElf, (ELF_SHN_ABS      << 16) | temp);
  ptr = ptr->pNext;
  }

sym_delete_record_list(&head, FALSE);               /* Destroy temporary list */

//printf("SHStrtab start:  %08X\n", ELF_EHSIZE+total+symtab_length+strtab_length);

for (i = 0; i < strtab_length; i++)   fprintf(fElf, "%c", strings[i]);
for (i = 0; i < shstrtab_length; i++) fprintf(fElf, "%c", SHstrings[i]);

pInfo = pInfo_head;

for (j = 0; j < fragments; j++)				// PHeader	@@@@@
  {          // Should one -segment- magically cover all these -sections- ?@@@
  elf_dump_word(fElf, 1);
  elf_dump_word(fElf, ELF_EHSIZE + pInfo->position);
  elf_dump_word(fElf, pInfo->address);
  elf_dump_word(fElf, pInfo->address);
  elf_dump_word(fElf, pInfo->size);
  elf_dump_word(fElf, pInfo->size);
  elf_dump_word(fElf, 0x00000000);
  elf_dump_word(fElf, 1);
  pInfo = pInfo->pNext;
  }

elf_dump_SH(fElf, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);        /* Dump section table */

pInfo = pInfo_head;
for (i = 0; i < fragments; i++)
  {
  elf_dump_SH(fElf, code_SHstr_offset, 1, 0x07, pInfo->address, 
              ELF_EHSIZE + pInfo->position, pInfo->size, 0, 0, 0, 0);
  pInfo = pInfo->pNext;
  }

elf_dump_SH(fElf, sym_SHstr_offset, 2, 0, 0,ELF_EHSIZE + total,
               symtab_length, fragments+2, symtab_local_count, 0, 16);	// @@@

elf_dump_SH(fElf, str_SHstr_offset, 3, 0, 0,ELF_EHSIZE + total + symtab_length,
               strtab_length, 0, 0, 0, 0);

elf_dump_SH(fElf, SHstr_SHstr_offset, 3, 0, 0,
               ELF_EHSIZE + total + symtab_length + strtab_length,
               shstrtab_length, 0, 0, 0, 0);

close_output_file(fElf, elf_file_name, pass_errors != 0);

                                           /* Trash temporary data structures */
pInfo = pInfo_head;

while (pInfo != NULL) { pInfo_new=pInfo->pNext; free(pInfo); pInfo=pInfo_new; }

pTemp = elf_record_list;
while (pTemp != NULL) { pTemp_old=pTemp; pTemp=pTemp->pNext; free(pTemp_old); }

free(strings);
free(SHstrings);

return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Prepare the list output buffer at the start of a line.                     */

void list_start_line(unsigned int address, int cont)
{
list_byte = 0;
if (!cont) list_line_position = 0;
list_address = address;
return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
// Parameterisation a bit dubious; due for re-analysis & revision @@@@

void list_mid_line(unsigned int value, char *line, int size)
{
if (list_byte == 0)                                 /* At start of first line */
  list_buffer_init(line, list_byte, TRUE);              /* Zero output buffer */
else
  if (((list_byte % 4) == 0)                         /* Start of another line */
  || (((list_byte % 4) + size) > 4))                 /*  or about to overflow */
    {
    if (list_byte != 0) list_file_out();                   /* Dump buffer (?) */
    list_buffer_init(line, list_byte, TRUE);            /* Zero output buffer */
    }

list_hex(value, 2 * size, &list_buffer[10 + 3 * (list_byte % 4)]);
list_byte = list_byte + size;
return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void list_end_line(char *line)
{
int i;

if (list_byte == 0) list_buffer_init(line, list_byte, TRUE);
                                                      /* No bytes were dumped */
list_file_out();

while (line[list_line_position] != '\0')  /* Deal with any continuation lines */
  {
  list_buffer_init(line, 0, FALSE);
  list_file_out();
  }

return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Listing line dumped to appropriate file                                    */
/* Excessive use of globals (?? @@)                                           */

void list_file_out(void)
{
if (dump_code && (fList != NULL)) fprintf(fList, "%s\n", list_buffer);
return;              /* Shouldn't reach here unless there -is- an output file */
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Add the symbol table to the list file                                      */

void list_symbols(FILE *fList, sym_table *table)
{
unsigned int sym_count;
sym_record *sorted_list, *pSym;
int i;

sym_count = sym_count_symbols(table, ALL);
if (sym_count > 0) fprintf(fList, "\nSymbol Table: %s\n", table->name);
  {
  sorted_list = sym_sort_symbols(table, ALL, DEFINITION);
                                                      /* Generate record list */
  pSym = sorted_list;
  while (pSym != NULL)
    {
    fprintf(fList, ": ");
    for (i = 0; i < SYM_NAME_MAX; i++)
      {
      if (i < pSym->count) fprintf(fList, "%c", pSym->name[i]);
      else                 fprintf(fList, " ");
      }
    if ((pSym->flags & SYM_REC_DEF_FLAG) != 0)
      fprintf(fList, "  %08X",     pSym->value);
    else
      fprintf(fList, "  00000000");

    if      ((pSym->flags&SYM_REC_EQU_FLAG)   !=0) fprintf(fList,"  Value");
    else if ((pSym->flags&SYM_REC_USR_FLAG)   !=0) fprintf(fList,"  Constant");
    else if ((pSym->flags&SYM_REC_DATA_FLAG)  !=0) fprintf(fList,"  Offset");
    else if ((pSym->flags&SYM_REC_DEF_FLAG)   ==0) fprintf(fList,"  Undefined");
    else
      {
      if ((pSym->flags&SYM_REC_EXPORT_FLAG)!=0) fprintf(fList,"  Global");
      else                                      fprintf(fList,"  Label");
/*    if ((pSym->flags&SYM_REC_EXPORT_FLAG)!=0) fprintf(fList,"  Global -");
      else                                      fprintf(fList,"  Local --");
      if ((pSym->flags&SYM_REC_THUMB_FLAG)==0)  fprintf(fList, " ARM");
      else                                      fprintf(fList, " Thumb");
*/    }
    fprintf(fList, "\n");
    pSym = pSym->pNext;
    }

  sym_delete_record_list(&sorted_list, FALSE);      /* Destroy temporary list */
  }
return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void list_buffer_init(char *line, unsigned int offset, int do_address)
{
int i;

for (i = 0; i < LIST_BYTE_FIELD; i++) list_buffer[i] = ' ';
if (do_address)
  {
  list_hex(list_address + offset, 8, &list_buffer[0]);
  list_buffer[8]  = ':';
  }
list_buffer[LIST_BYTE_FIELD - 2] = ';';

for (i = 0; (i < LIST_LINE_LIST) && (line[list_line_position] != '\0');
             i++, list_line_position++)
  {
  if (line[list_line_position] != '\t')                          /* Not a TAB */
    list_buffer[LIST_BYTE_FIELD + i] = line[list_line_position];
  else
    {     /* Expand TAB into list line (space to column # next multiple of 8) */
    do                                /* "DO" to guarantee at least one space */
      { list_buffer[LIST_BYTE_FIELD + i] = ' '; i++; }
      while (((i % 8) != 0) && (i < LIST_LINE_LIST));
    i--;    /* DO loop post-increments; so does surrounding FOR, so step back */
    }
  }

list_buffer[LIST_BYTE_FIELD + i] = '\0';                  /* Terminate buffer */

return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void list_hex(unsigned int number, unsigned int length, char *destination)
{
int i, digit;

for (i = 0; i < length; i++)
  {
  digit = (number >> ( 4 * (length - i - 1)) ) & 0xF;
  if (digit < 10) destination[i] = '0' + digit;
  else            destination[i] = 'A' + digit - 10;
  }
return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Create a new, empty symbol table with given attributes.                    */
/* On input: name is a zero terminated ASCII string, given to the table       */
/*           flags contain the associated attributes                          */
/* Returns:  pointer (handle) for the table (NULL on failure)                 */

sym_table *sym_create_table(char *name, unsigned int flags)
{
sym_table *new_table;
int i;

for (i = 0; name[i] != '\0'; i++);/* Find length of string (excl. terminator) */

new_table =  (sym_table*) malloc(SYM_TABLE_SIZE);          /* Allocate header */
if (new_table != NULL)
  {
  new_table->name = (char*) malloc(i+1);              /* Allocate name string */
  if (new_table->name == NULL)
    {                                          /* Problem - tidy up and leave */
    free(new_table);
    new_table = NULL;
    }
  else
    {
    new_table->symbol_number = 0;        /* Next unique identifier for record */
    while (i >= 0) {new_table->name[i] = name[i]; i--;}/* Includes terminator */
    new_table->flags = flags;
    for (i = 0; i < SYM_TAB_LIST_COUNT; i++)       /* Initialise linked lists */
      new_table->pList[i] = NULL;
    }
  }
return new_table;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Delete a symbol table, including all its contents, unless records are both */
/* wanted and marked for export.                                              */
/* On input: old_table is the symbol table for destruction                    */
/*           export is a Boolean - TRUE allows records marked for export to   */
/*             retained                                                       */
/* Returns:  Boolean - TRUE if some of the table remains                      */

int sym_delete_table(sym_table *old_table, boolean export)
{
int i;
boolean some_kept;

some_kept = export && ((old_table->flags & SYM_TAB_EXPORT_FLAG) != 0);

if (!some_kept)                                  /* Not exporting whole table */
  for (i=0; i<SYM_TAB_LIST_COUNT; i++)  /* Chain down lists, deleting records */
    if (sym_delete_record_list(&(old_table->pList[i]), export))
      some_kept = TRUE;

if (!some_kept) { free(old_table->name); free(old_table); } /* Free, if poss. */

return some_kept;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Define a label with the given name, value and attributes in the specified  */
/* symbol table.  Allocates memory as appropriate (linked into symbol table). */
/* On input: *name points to a string which is the label name                 */
/*           value holds the value for definition                             */
/*           flags holds the attributes                                       */
/*           table points to an existing symbol table                         */
/*           **record defines a pointer for a return value                    */
/* Returns:  enumerated type indicating the action taken                      */
/*           pointer to the appropriate record in var. specified by **record  */

defn_return sym_define_label(char *name, unsigned int value,
                                         unsigned int flags,
                                         sym_table *table,
                                         sym_record **record)
{
sym_record *ptr1, *ptr2;
defn_return result;

ptr1 = sym_create_record(name, value, flags | SYM_REC_DEF_FLAG, table->flags);

if ((table == NULL) || (ptr1 == NULL)) result = SYM_REC_ERROR;      /* Oooer! */
else
  {
  if ((ptr2 = sym_find_record(table, ptr1)) == NULL) /* Label already exists? */
    {
    sym_add_to_table(table, ptr1);                /*  No - add the new record */
    *record = ptr1;                                    /* Point at new record */
    result = SYM_REC_ADDED;
    }
else
    {
    if ((ptr2->flags & SYM_REC_DEF_FLAG) == 0)                  /* Undefined? */
      {
      ptr2->flags |= SYM_REC_DEF_FLAG;  /* First definition of existing label */
      ptr2->value = ptr1->value;                              /* Update value */
      result = SYM_REC_DEFINED;    
      }
    else
      if (ptr2->value != ptr1->value)                     /* Value different? */
        {
        ptr2->value = ptr1->value;                            /* Update value */
        result = SYM_REC_REDEFINED;
        }
      else
        result = SYM_REC_UNCHANGED;

    *record = ptr2;                             /* Point at discovered record */
    sym_delete_record(ptr1);                        /* Trash temporary record */
    }
  }

return result;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Locate a label with the given name and attributes in the specified symbol  */
/* table.  Creates the entry if it wasn't there.  The value is undefined.     */
/* Allocates memory as appropriate (linked into symbol table).                */
/* On input: *name points to a string which is the label name                 */
/*           flags holds the attributes                                       */
/*           table points to an existing symbol table                         */
/*           **record defines a pointer for a return value                    */
/* Returns:  TRUE if the record was found (previously existed)                */
/*           pointer to the appropriate record in var. specified by **record  */

int sym_locate_label(char *name, unsigned int flags,
                                 sym_table   *table,
                                 sym_record **record)
{
sym_record *ptr1, *ptr2;
boolean result;
//defn_return result;

ptr1 = sym_create_record(name, 0, flags & ~SYM_REC_DEF_FLAG, table->flags);

if ((ptr2 = sym_find_record(table, ptr1)) == NULL)   /* Label already exists? */
  {
  *record = ptr1;                                      /* Point at new record */
  result = FALSE;
  }
else
  {
  sym_delete_record(ptr1);                          /* Trash temporary record */
  *record = ptr2;                               /* Point at discovered record */
  result = TRUE;
  }

return result;
}				// Errors?  (If allocation fails?)  @@@@@@

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Find a label (by name) in the designated list of tables.                   */
/* On input: name points to a string which is the label name                  */
/*           table points to an existing list of symbol tables                */
/* Returns:  pointer to record (NULL if not found)                            */

sym_record *sym_find_label_list(char *name, sym_table_item *item)
{
sym_table  *table;
sym_record *result;

result = NULL;                                     /* In case nothing in list */

while ((item != NULL) && (result == NULL))    /* Terminate if EOList or found */
  {
  table = item->pTable;
  if (table != NULL) result = sym_find_label(name, table);
  item = item->pNext;
  }

return result;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Find a label (by name) in the designated table.                            */
/* On input: name points to a string which is the label name                  */
/*           table points to an existing symbol table                         */
/* Returns:  pointer to record (NULL if not found)                            */

sym_record *sym_find_label(char *name, sym_table *table)
{
sym_record *temp, *result;

temp   = sym_create_record(name, 0, 0, table->flags);            /* Hash name */
result = sym_find_record(table, temp);                              /* Search */
sym_delete_record(temp);                             /* Lose temporary record */
return result;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Create a new symbol record, complete with hashing etc.                     */
/* On input: name is the label name (ASCII string)                            */
/*           value is the initial value for the record                        */
/*           flags define other aspects of the record                         */
/*           global_flags define the symbol table properties                  */
/* Returns:  pointer to allocated record (NULL if this failed)                */

sym_record *sym_create_record(char *name, unsigned int value,
                                          unsigned int flags,
                                          unsigned int global_flags)
{
sym_record *new_record;

new_record = (sym_record*) malloc(SYM_RECORD_SIZE);        /* Allocate record */

if (new_record != NULL)
  {
  sym_string_copy(name, new_record, global_flags);
  new_record->pNext = NULL;
  new_record->value = value;
  new_record->flags = flags;
  }

return new_record;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Delete a single linked list of symbol records.                             */
/* Will retain records marked for export if requested to.                     */
/* On input: ptr1 is the adddress of the start of list pointer                */
/*           export is a Boolean indicating that records may be retained      */
/* Returns:  TRUE if any records have been kept                               */

int sym_delete_record_list(sym_record **ptr1, int export)
{
boolean some_kept;
sym_record *ptr2, *ptr3; /* Current and next records; ptr1 => current pointer */

some_kept = FALSE;                              /* Default to keeping nothing */

while (*ptr1 != NULL)                            /* While not end of list ... */
  {
  ptr2 = *ptr1;                                   /* Record for consideration */

  if (!export || ((ptr2->flags & SYM_REC_EXPORT_FLAG) == 0))
    {                                                        /* Delete record */
    ptr3  =  ptr2->pNext;                      /* Salvage link to next record */
    *ptr1 =  ptr3;                 /* Point previous link past current record */
    sym_delete_record(ptr2);                          /* Trash current record */
    }
  else
    {
    ptr1 = &ptr2->pNext;                                       /* Move on ... */
    some_kept = TRUE;                   /* Noting that something was retained */
    }
  }

return some_kept;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Delete a single symbol record                                              */

void sym_delete_record(sym_record *old_record)
{               /* Can deallocate strings etc. if such have been allocated @@ */
free(old_record);
return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Add record to appropriate part of table (front of list)                    */

int sym_add_to_table(sym_table *table, sym_record *record)
{
unsigned int list;                         /* Which data substructure is used */

if (table != NULL)
  {
  list = record->hash & SYM_TAB_LIST_MASK;

  record->identifier = table->symbol_number++;   /* Allocate unique record No */
  record->pNext      = table->pList[list];
  table->pList[list] = record;

  return SYM_NO_ERROR;
  }
else
  return SYM_NO_TABLE;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Search for record's twin in specified table and return a pointer.          */
/* Returns NULL if not found.                                                 */

sym_record *sym_find_record(sym_table *table, sym_record *record)
{
sym_record *ptr;
boolean found;
int i;

if (table != NULL)
  {
  ptr = table->pList[record->hash & SYM_TAB_LIST_MASK]; /* Correct list start */
  found = FALSE;

  while ((ptr != NULL) && !found)
    {
    if ((ptr->hash == record->hash) && (ptr->count == record->count))
      {
      i = 0;
      found = TRUE;                                /* Speculation, at present */
      while ((i < ptr->count) && found)                        /* Scan string */
        {
        found = (ptr->name[i] == record->name[i]);    /* Not found after all? */
        i++;
        }
      }
    if (!found) ptr = ptr->pNext;                  /* If not found, try again */
    }
  return ptr;
  }
else
  return NULL;                       /* If table pointer not valid, not found */
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*  Copy a string into a specified record, including case conversion,         */
/* generating hash functions, etc.                                            */

void sym_string_copy(char *string, sym_record *record, unsigned int table_flags)
{
unsigned int hash, count;
int case_insensitive;
char c;

case_insensitive = ((table_flags & SYM_TAB_CASE_FLAG) != 0);

count = 0;
hash  = 0;
while ((c = string[count]) != '\0')
  {
  if (case_insensitive && (c >= 'a') && (c <= 'z')) c = c&0xDF; /* Case conv? */
  if (count < SYM_NAME_MAX) record->name[count] = c;
                                     /* Keep characters whilst there is space */

  hash = (((hash<<5) ^ (hash>>11)) + c);            /* Crude but spreads LSBs */
  count++;
  }                                            /* Doesn't copy the terminator */

record->count = count;
record->hash  = hash;

return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Make up an array of all the strings in the symbol table                    */
/* Used for ELF symbol table output                                           */

char *sym_strtab(sym_record *start, unsigned int count, unsigned int *length)
{
unsigned int index, i;
char *array;
sym_record *ptr;

ptr     = start;
*length = 1;                                      /* For null string at start */

while (ptr != NULL)                       /* Measure space for symbol strings */
  {
  *length = *length + ptr->count + 1;
  ptr = ptr->pNext;
  }
*length = (*length + 3) & 0xFFFFFFFC;                           /* Word align */

array = (char*) malloc(*length);                     /* Allocate buffer space */
						// No error checking @@@
index = 0;
array[index++] = '\0';                                         /* "" at start */

ptr = start;
while (ptr != NULL)
  {
  for (i = 0; i < ptr->count; i++) array[index++] = ptr->name[i];
  array[index++] = '\0';                                        /* Terminator */
  ptr = ptr->pNext;
  }

while ((index & 3) != 0) array[index++] = '\0';                /* Pad to word */

return array;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Count entries of a specified type in a specified symbol table              */

unsigned int sym_count_symbols(sym_table *table, label_category what)
{
unsigned int count, i;
sym_record *ptr;

count = 0;

for (i = 0; i < SYM_TAB_LIST_COUNT; i++)                /* For all structures */
  {
  ptr = table->pList[i];                                     /* Start of list */
  while (ptr != NULL)
    {
    if ((what != EXPORTED) || ((ptr->flags & SYM_REC_EXPORT_FLAG) != 0))
      count++;
    ptr = ptr->pNext;
    }
  }

return count;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Returns a newly created (allocated) linked list of symbol records copied   */
/* from the designated table and sorted as specified.                         */

sym_record *sym_sort_symbols(sym_table *table, label_category what,
                                               label_sort how)
{

  void sym_dup_record(sym_record *old_record, sym_record *new_record)
  {
  unsigned int i, j;

  new_record->count       = old_record->count;
  new_record->hash        = old_record->hash;
  new_record->flags       = old_record->flags;
  new_record->identifier  = old_record->identifier;
  new_record->value       = old_record->value;
  new_record->elf_section = old_record->elf_section;
  j = old_record->count;
  if (j > SYM_NAME_MAX) j = SYM_NAME_MAX;
  for (i = 0; i < j; i++)
    new_record->name[i] = old_record->name[i];

  return;
  }


sym_record *temp_record, *sorted_list, *ptr1, *ptr2, **pptr;
int i, j, min, after;
boolean found;
unsigned int flag_mask, flag_match;

switch (what)                                  /* Class of records to include */
  {
  case ALL: flag_mask = 0; flag_match = 0; break;
  case EXPORTED:
    flag_mask  = SYM_REC_EXPORT_FLAG;
    flag_match = SYM_REC_EXPORT_FLAG;
    break;
  case DEFINED:
    flag_mask  = SYM_REC_DEF_FLAG;
    flag_match = SYM_REC_DEF_FLAG;
    break;
  case UNDEFINED:
    flag_mask  = SYM_REC_DEF_FLAG;
    flag_match = 0;
    break;
  default: flag_mask = 0; flag_match = 0; break;
  }

sorted_list = NULL;
for (i = 0; i < SYM_TAB_LIST_COUNT; i++)
  {
  ptr1 = table->pList[i];
  while (ptr1 != NULL)
    {
    if ((ptr1->flags & flag_mask) == flag_match)       /* Criteria for output */
      {
      temp_record = (sym_record*) malloc(SYM_RECORD_SIZE);
      sym_dup_record(ptr1, temp_record);

      if ((table->flags & SYM_TAB_EXPORT_FLAG) != 0)
        temp_record->flags |= SYM_REC_EXPORT_FLAG;    /* Global => local flag */

      pptr = &sorted_list;                      /* Linked list insertion sort */
      ptr2 =  sorted_list;
      found = FALSE;
      while ((ptr2 != NULL) && !found)
        {
        switch (how)                                /* Field used for sorting */
          {
          case ALPHABETIC:                             /* Sort alphabetically */
            if (temp_record->count < ptr2->count) min = temp_record->count;
            else                                  min = ptr2->count;
            if (min > SYM_NAME_MAX) min = SYM_NAME_MAX;
                                                      /* Clip to field length */
            j = 0;
            while ((temp_record->name[j] == ptr2->name[j]) && (j < min)) j++;
            after = (temp_record->name[j] > ptr2->name[j])/* After candidate? */
             || ((j >= min) && (temp_record->count > j));   /* New string > ? */
            break;

          case VALUE:                                     /* Sort numerically */
            after = (temp_record->value > ptr2->value)
                && ((temp_record->flags & SYM_REC_DEF_FLAG) != 0);
            break;

          case DEFINITION:                          /* In order of definition */
            after = (temp_record->identifier > ptr2->identifier);
            break;

          case FOR_ELF:               /* In order of definition, locals first */
            after = ((((temp_record->flags & SYM_REC_EXPORT_FLAG) != 0)
                          && ((ptr2->flags & SYM_REC_EXPORT_FLAG) == 0))
                    || (temp_record->identifier > ptr2->identifier));
            break;

          }

        if (after)
          {
          pptr = &(ptr2->pNext);
          ptr2 = ptr2->pNext;                            /* Lower, keep going */
          }
        else
          found = TRUE;

//        DUPLICATES  ???

        }
      temp_record->pNext = ptr2;                     /* Insert created record */
      *pptr = temp_record;
      }

    ptr1 = ptr1->pNext;                                 /* Next source record */
    }
  }
return sorted_list;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Print out symbols in table.  Tedious, because symbols need sorting.        */

void sym_print_table(sym_table *table, label_category what, label_sort how,
                     int std_out, char *file)
{
sym_record *sorted_list, *ptr;
int i;
FILE *handle;

if (std_out) handle=stdout; else handle=fopen(file,"w");  /* Open output file */

if (file == NULL)
  fprintf(stderr, "Can't open symbol file: %s\n", file);
else
  {
  sorted_list = sym_sort_symbols(table, what, how);   /* Generate record list */

  fprintf(handle, "\nSymbol table: %s\n", table->name);
  fprintf(handle, "Label");
  for (i = 0; i < SYM_NAME_MAX - 2; i++) fprintf(handle, " ");
//fprintf(handle, "  ID      Length     Hash     Value    Type\n");
  fprintf(handle, "  ID      Value    Type\n");

  ptr = sorted_list;
  while (ptr != NULL)
    {
    for (i = 0; i < SYM_NAME_MAX; i++)
      {
      if (i < ptr->count) fprintf(handle, "%c", ptr->name[i]);
      else if (i > ptr->count + 1) fprintf(handle, ".");
      else fprintf(handle, " ");
      }
    fprintf(handle, "  %08X", ptr->identifier);
//  fprintf(handle, "  %08X", ptr->count);
//  fprintf(handle, "  %08X", ptr->hash);
    if ((ptr->flags & SYM_REC_DEF_FLAG) != 0)
      fprintf(handle, "  %08X", ptr->value);
    else
      fprintf(handle, " Undefined");

    if ((ptr->flags & SYM_REC_USR_FLAG)   != 0) fprintf(handle, "  Constant   ");
    else
      if ((ptr->flags & SYM_REC_EQU_FLAG) != 0) fprintf(handle, "  Value      ");
      else
        if ((ptr->flags&SYM_REC_DATA_FLAG)!= 0) fprintf(handle, "  Offset     ");
        else
          {
          if ((ptr->flags&SYM_REC_THUMB_FLAG)==0) fprintf(handle, "  RV32 label  ");
          else                                    fprintf(handle, "  Thumb label");
          }

    if ((ptr->flags & SYM_REC_EXPORT_FLAG) != 0) /* Table flags in temp recd. */
      fprintf(handle, " (exported)");
    fprintf(handle, "\n");
    ptr = ptr->pNext;
    }

  sym_delete_record_list(&sorted_list, FALSE);      /* Destroy temporary list */

  if ((sym_print_extras & 1) != 0) local_label_dump(loc_lab_list, handle);

  if (handle != stdout) fclose(handle);
  }

return;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void local_label_dump(local_label *pTable, FILE *handle)
{
if (pTable != NULL)
  {
  fprintf(handle, "\nLocal (labels in order of definition):\n");
  fprintf(handle, "           Local Label     Value\n");
  while (pTable != NULL)
    {
    fprintf(handle, "%22d:  %08X\n", pTable->label, pTable->value);
    pTable = pTable->pNext;
    }
  }

return;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/* Evaluate - modulo current word length                                      */
/* On entry: *string points to a pointer to the input string                  */
/*            *pos points to an offset in the string                          */
/*            *value points to the location for the result value              */
/*            *symbol_table points to the symbol table to search [@@ extend]  */
/* On exit:  the pointer at *pos is adjusted to the end of the expression     */
/*           the value at *value contains the result, assuming no error       */
/*           the return value is the error status                             */

unsigned int evaluate(char *string, unsigned int *pos, int *value,
                      sym_table *symbol_table)
{
unsigned int math_stack[MATHSTACK_SIZE];
unsigned int math_SP, error, first_error;

  void Eval_inner(int priority, int *value)
  {                                        /* Main function shares stack etc. */
  boolean done, bracket;
  unsigned int operator, operand, unary;

  done = FALSE;                                      /* Termination indicator */

  math_stack[math_SP] = priority;
  math_SP = math_SP + 1;                              /* Stack `start' marker */

  while (!done)
    {
    error = get_variable(string, pos, &operand, &unary, &bracket, symbol_table);

    if ((error & ALL_EXCEPT_LAST_PASS) != 0)     /* Error not instantly fatal */
      {
      if (first_error == eval_okay) first_error = error;/* Keep note of error */
      error = eval_okay;             /*  and pretend everything is still okay */
      }

    if (error == eval_okay)
      {
      if (bracket) Eval_inner(1, &operand);               /* May return error */
      if (error == eval_okay)
        {
        switch (unary)               /* Can now apply unary to returned value */
          {
          case PLUS:                      break;
          case MINUS: operand = -operand; break;
          case NOT:   operand = ~operand; break;
          case LOG:                              /* Truncated log2 of operand */
            {unsigned int i;i=operand;operand=-1;while(i>0){operand++;i=i>>1;}}
            break;
          }

        if ((error = get_operator(string,pos,&operator,&priority)) == eval_okay)
          {
          while ((priority <= math_stack[math_SP - 1])
                          && (math_stack[math_SP - 1] > 1))
            { /* If priority decreasing and previous a real operator, OPERATE */
            switch (math_stack[math_SP - 2])
              {
              case PLUS:
                operand = math_stack[math_SP - 3] + operand;
                break;
              case MINUS:
                operand = math_stack[math_SP - 3] - operand;
                break;
              case MULTIPLY:
                operand = math_stack[math_SP - 3] * operand;
                break;
              case DIVIDE:
                if (operand != 0)
                  operand = math_stack[math_SP - 3] / operand;
                else
                  {
                  operand = -1;
                  if ((error == eval_okay) && (first_error == eval_okay))
                    error = eval_div_by_zero;
                  div_zero_this_pass = TRUE;
                  }
                break;
              case MODULUS:
                if (operand != 0)                      /* else leave it alone */
                  operand = math_stack[math_SP - 3] % operand;
                break;
              case LEFT_SHIFT:
                operand = math_stack[math_SP - 3] << operand;
                break;
              case RIGHT_SHIFT:
                operand = math_stack[math_SP - 3] >> operand;
                break;
              case AND:
                operand = math_stack[math_SP - 3] & operand;
                break;
              case OR:
                operand = math_stack[math_SP - 3] | operand;
                break;
              case XOR:
                operand = math_stack[math_SP - 3] ^ operand;
                break;
              case EQUALS:
                if (math_stack[math_SP - 3] == operand) operand = -1;
                else                                    operand =  0;
                break;
              case NOT_EQUAL:
                if (math_stack[math_SP - 3] != operand) operand = -1;
                else                                    operand =  0;
                break;
              case LOWER_THAN:
                if (math_stack[math_SP - 3] <  operand) operand = -1;
                else                                    operand =  0;
                break;
              case LOWER_EQUAL:
                if (math_stack[math_SP - 3] <= operand) operand = -1;
                else                                    operand =  0;
                break;
              case HIGHER_THAN:
                if (math_stack[math_SP - 3] >  operand) operand = -1;
                else                                    operand =  0;
                break;
              case HIGHER_EQUAL:
                if (math_stack[math_SP - 3] >= operand) operand = -1;
                else                                    operand =  0;
                break;
              case LESS_THAN:
                if ((int)math_stack[math_SP - 3] <  (int)operand) operand = -1;
                else                                              operand =  0;
                break;
              case LESS_EQUAL:
                if ((int)math_stack[math_SP - 3] <= (int)operand) operand = -1;
                else                                              operand =  0;
                break;
              case GREATER_THAN:
                if ((int)math_stack[math_SP - 3] >  (int)operand) operand = -1;
                else                                              operand =  0;
                break;
              case GREATER_EQUAL:
                if ((int)math_stack[math_SP - 3] >= (int)operand) operand = -1;
                else                                              operand =  0;
                break;

              default: break;
              }
            math_SP = math_SP - 3;
            }
          done = (priority <= 1);               /* Next operator a ")" or end */

          if (!done)
            {                                  /* Priority must be increasing */
            if ((math_SP + 3) <= MATHSTACK_SIZE)                      /* PUSH */
              {
              math_stack[math_SP]     = operand;
              math_stack[math_SP + 1] = operator;
              math_stack[math_SP + 2] = priority;
              math_SP = math_SP + 3;
              }
            else
              error = eval_mathstack_limit;           /* Don't overflow stack */
            }
          else
            {                      /* Now bracketed by terminators.  Matched? */
            if (priority == math_stack[math_SP - 1]) math_SP = math_SP - 1;
            else if (priority == 0) error = eval_not_closebr;       /* Errors */
            else                    error = eval_not_openbr;
            }
          }
        }
      }
    if (error != eval_okay)
      {
      done = TRUE;                       /* Terminate on error whatever else */
      if (error == eval_not_openbr)   /* Include position on line (if poss.) */
        error = error | (*pos - 1);            /* Has stepped over extra ')' */
      else
        if (error != eval_div_by_zero)   /* Arithmetic error will occur late */
          error = error | *pos;                  /* Include position on line */
      }
    }

  *value = operand;

  return;
  }

error       = eval_okay;       /* "Evaluate" initialised and called from here */
first_error = eval_okay;            /* Used to note if labels undefined, etc. */
math_SP     = 0;
Eval_inner(0, value);                /* Potentially recursive evaluation code */

if (error == eval_okay) return first_error;  /* Signal any problems held over */
else                    return error;
}

/*----------------------------------------------------------------------------*/
/* Get a quantity from the front of the passed ASCII input string, stripping  */
/* the value in the process.                                                  */
/* On entry: *input points to the input string                                */
/*            *pos points to the offset in this string                        */
/*            *value points to the location for the result value              */
/*            *unary points to the location for the result's unary indicator  */
/*            *bracket points to the location of a Boolean "found '(' signal  */
/*            *symbol_table points to the symbol table to search [@@ extend]  */
/* On exit:  the position at *pos is adjusted to the end of the variable      */
/*           the value at *value contains the result, assuming no error       */
/*           the value at *unary contains a unary code, assuming no error     */
/*           the value at *bracket contains a "'(' found instead" indicator   */
/*           the return value is the error status                             */

int get_variable(char *input, unsigned int *pos, int *value, int *unary,
                              boolean *bracket, sym_table *symbol_table)
{
int status, radix;
unsigned int ii;

status = eval_no_operand;                            /* In case nothing found */
radix  = -1;                         /* Indicates no numeric constant spotted */
*pos = skip_spc(input, *pos);      /* In case of error want this at next item */
ii = *pos;                                   /* String pointer within routine */

*unary   = PLUS;                                                   /* Default */
*bracket = FALSE;                                    /* Default - no brackets */
*value   = 0;

/* Deal with unary operators */
if      (input[ii] == '+') {                 ii = skip_spc(input, ii + 1);}
else if (input[ii] == '-') { *unary = MINUS; ii = skip_spc(input, ii + 1);}
else if (input[ii] == '~') { *unary = NOT;   ii = skip_spc(input, ii + 1);}
else if (input[ii] == '|') { *unary = LOG;   ii = skip_spc(input, ii + 1);}

if (input[ii] == '(')
  {                                         /* Open brackets instead of value */
  *bracket = TRUE;
  ii++;                                                       /* Skip bracket */
  status = eval_okay;                                         /* Legal syntax */
  }
else
  {
  int i;
  char ident[LINE_LENGTH];
  sym_record *symbol;

  if ((i = get_identifier(input, ii, ident, LINE_LENGTH)) > 0)
    {                                                      /* Something taken */
    if ((symbol = sym_find_label(ident, symbol_table)) != NULL)
      {
      if ((symbol->flags & SYM_REC_DEF_FLAG) != 0)
        {                             /* Label present and with a valid value */
        *value = symbol->value;
        status = eval_okay;
        }
      else
        {                                    /* Label found but value invalid */
        status = eval_label_undef | ii;
        undefined_count++;                       /* Increment global variable */
        }
      }
    else
      {                                                    /* Label not found */
//printf("Tables: %08X %08X %s\n", copro_table, csr_table, ident);
      if ((symbol = sym_find_label(ident, csr_table)) != NULL)	//***!!!@@@
        {                                                /* CSR address found */
        *value = symbol->value;
//      status = eval_label_undef | ii;
        status = eval_okay;
        }
      else
        {
        status = eval_no_label | ii;
        }
      }
    ii = ii + i;                                           /* Step pointer on */
    }                                               /* End of label gathering */
  else
    {
    if (input[ii] == '\%')
      {
      local_label *pStart, *pTemp;
      char c;
      int directions;                      /* Bit flags for search directions */
      unsigned int label;

      c = input[ii + 1] & 0xDF;
      if      (c == 'B') { directions = 1; ii = ii + 2; }        /* Backwards */
      else if (c == 'F') { directions = 2; ii = ii + 2; }        /* Forwards  */
      else               { directions = 3; ii = ii + 1; }        /* Both ways */

      if ((evaluate_own_label->sort != LOCAL_LABEL) && ((directions & 1) == 0))
        {      /* If searching forwards only and no local label on this line */
        if (loc_lab_position == NULL) pStart = loc_lab_list;/* Start of list */
        else                          pStart = loc_lab_position->pNext;
        }
      else      /* If searching backwards, own label will be present already */
        pStart = loc_lab_position;
      if (!get_num(input, &ii, &label, 10)) status = eval_bad_loc_lab;
      else
        {
        boolean found;

        found = FALSE;

        if ((directions & 1) != 0)                        /* Seach backwards */
          {
          pTemp = pStart;
          while ((pTemp != NULL) && !found)
            if (!(found = (label == pTemp->label))) pTemp = pTemp->pPrev;
          }

        if (!found && ((directions & 2) != 0))             /* Seach forwards */
          {
          pTemp = pStart;
          while ((pTemp != NULL) && !found)
            if (!(found = (label == pTemp->label))) pTemp = pTemp->pNext;
          }

        if (found) { status = eval_okay; *value = pTemp->value; }
        else         status = eval_no_label;
        }
      }
    else
      {
      if (input[ii] == '\'')                            /* Character constant */
        {
        ii++;

        if (input[ii] != '\\')                        /* 'Escaped' character? */
//   ;  if (TRUE)                               /* Insert if escapes unwanted */
          {                                                         /* No ... */
          if ((input[ii]!='\0') && (input[ii]!='\n') && (input[ii+1]=='\''))
            { *value = input[ii]; ii += 2; status = eval_okay; }
          else
            status = eval_operand_error | ii;
          }
        else
          {                                     /* C-style escaped characters */
          ii++;                                                   /* Skip '\' */
          if ((input[ii]!='\0') && (input[ii]!='\n') && (input[ii+1]=='\''))
            { *value = c_char_esc(input[ii]); ii += 2; status = eval_okay; }
          else
            status = eval_operand_error | ii;
          }
        }
      else
        {
        if (input[ii] == '.')
          {
          if (assembly_pointer_defined)
            { *value = assembly_pointer + def_increment; status = eval_okay; }
          else status = eval_label_undef | ii;
          ii++;
          }
        else
          {                                               /* Try for a number */
          if (input[ii] == '0')                /* 'orrible 'ex prefices, etc. */
            {
            if      ((input[ii+1] & 0xDF) == 'X') { ii+=2; radix = 16; }
            else if ((input[ii+1] & 0xDF) == 'B') { ii+=2; radix =  2; }
            }
          if (radix < 0)                                /* Not yet identified */
            {
            if ((input[ii] >= '0') && (input[ii] <= '9'))  radix = 10;
            else if (input[ii] == '$')            { ii++;  radix = 16; }
//          else if (input[ii] == '&')            { ii++;  radix = 16; }
            else if (input[ii] == ':')            { ii++;  radix =  2; }
            else if (input[ii] == '@')            { ii++;  radix =  8; }
            }
          if (radix > 0)
            {
            if (get_num(input, &ii, value, radix)) status = eval_okay;
            else                                   status = eval_out_of_radix;
            }
          }
        }
      }
    }
  }

if ((status == eval_okay) || ((status & ALL_EXCEPT_LAST_PASS) != 0))
  *pos = ii;             /* Move input pointer if successful (in some degree) */
return status;                                           /* Return error code */
}

/*----------------------------------------------------------------------------*/
/* Get an operator from the front of the passed ASCII input string, stripping */
/* it in the process.  Returns the token and the priority.                    */
/* On entry: *input points to the input string                                */
/*            *pos points to the offset in this string                        */
/*            *operator points to the location for the operator code          */
/*            *priority points to the location for the priority code          */
/*                   0 is the lowest priority and is reserved for terminators */
/*                   priority 1 is reserved for brackets                      */
/* On exit:  the pointer at *pos is adjusted to the end of the expression     */
/*           the value at *operator contains the operator code                */
/*           the value at *priority contains the operator priority            */
/*           the return value is the error status                             */

int get_operator(char *input, unsigned int *pos, int *operator, int *priority)
{
int status;
char *temp;
unsigned int ii;

*pos = skip_spc(input, *pos);      /* In case of error want this at next item */
ii = *pos;                                   /* String pointer within routine */

status = eval_no_operator;
                   /* in case no operator was found, this will be the default */
switch (input[ii])
  {
  case '\0':                                              /* Terminator cases */
  case ',':
  case ';':
  case '[':
  case ']':
  case '}':
  case '\n': *operator = END;              status = eval_okay; break;
  case '+':  *operator = PLUS;      ii++;  status = eval_okay; break;
  case '-':  *operator = MINUS;     ii++;  status = eval_okay; break;
  case '*':  *operator = MULTIPLY;  ii++;  status = eval_okay; break;
  case '/':  *operator = DIVIDE;    ii++;  status = eval_okay; break;
  case '\\': *operator = MODULUS;   ii++;  status = eval_okay; break;
  case ')':  *operator = CLOSEBR;   ii++;  status = eval_okay; break;
  case '|':  *operator = OR;        ii++;  status = eval_okay; break;
  case '&':  *operator = AND;       ii++;  status = eval_okay; break;
  case '^':  *operator = XOR;       ii++;  status = eval_okay; break;
  case '=':  *operator = EQUALS;    ii++;  status = eval_okay; break;
  case '!':  if (input[ii+1] == '=')
               { *operator = NOT_EQUAL; ii += 2; status = eval_okay; }
             break;
  case '<': switch (input[ii+1])
              {
              case '<': *operator = LEFT_SHIFT;  ii += 2; break;
              case '>': *operator = NOT_EQUAL;   ii += 2; break;
              case '=': *operator = LOWER_EQUAL; ii += 2; break;
              default:  *operator = LOWER_THAN;  ii += 1; break;
              }
              status = eval_okay;
              break;
  case '>': switch (input[ii+1])
              {
              case '>': *operator = RIGHT_SHIFT;   ii += 2; break;
              case '=': *operator = HIGHER_EQUAL;  ii += 2; break;
              default:  *operator = HIGHER_THAN;   ii += 1; break;
              }
              status = eval_okay;
              break;

  default:
    {                          /* Have a go at symbolically defined operators */
    int i;
    char buffer[SYM_NAME_MAX];
    sym_record *ptr;

    if ((i = get_identifier(input, ii, buffer, SYM_NAME_MAX)) > 0)
      {                                                    /* Something taken */
      if ((ptr = sym_find_label(buffer, operator_table)) != NULL)
        {                                                /* Symbol recognised */
        *operator = ptr->value;
        ii += i;
        status = eval_okay;
        }
      }
    }
  }

switch (*operator)                                      /* Priority "look up" */
  {                                     /* The first two priorities are fixed */
  case END:           *priority = 0; break;
  case CLOSEBR:       *priority = 1; break;
  case PLUS:          *priority = 3; break;
  case MINUS:         *priority = 3; break;
  case MULTIPLY:      *priority = 4; break;
  case DIVIDE:        *priority = 4; break;
  case MODULUS:       *priority = 4; break;
  case LEFT_SHIFT:    *priority = 7; break;
  case RIGHT_SHIFT:   *priority = 7; break;
  case AND:           *priority = 6; break;
  case OR:            *priority = 5; break;
  case XOR:           *priority = 5; break;
  case EQUALS:        *priority = 2; break;
  case NOT_EQUAL:     *priority = 2; break;
  case LOWER_THAN:    *priority = 2; break;
  case LOWER_EQUAL:   *priority = 2; break;
  case HIGHER_THAN:   *priority = 2; break;
  case HIGHER_EQUAL:  *priority = 2; break;
  case LESS_THAN:     *priority = 2; break;
  case LESS_EQUAL:    *priority = 2; break;
  case GREATER_THAN:  *priority = 2; break;
  case GREATER_EQUAL: *priority = 2; break;
  }

if (status == eval_okay) *pos = ii;       /* Move input pointer if successful */
return status;                                           /* Return error code */
}

/*----------------------------------------------------------------------------*/

int skip_spc(char *line, int position)
{
while ((line[position] == ' ') || (line[position] == '\t')) position++;
return position;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Makes a copy of "filename" with the last element stripped back to '/'      */
/* Returns null string if no '/' present.                                     */
/* Allocates space for new string.                                            */

char *file_path(char *filename)
{
char *pPath;
int position;

position = strlen(filename);              /* Find back-end of original string */
while ((position > 0) && (filename[position - 1] != '/')) position--;

pPath = (char*) malloc(position+1);         /* Allocate space for path + '\0' */

pPath[position] = '\0';                       /* Insert terminator, step back */
while (position > 0)                                   /* Copy rest of string */
  {
  position = position - 1;
  pPath[position] = filename[position];
  }

return pPath;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Concatenates source strings and returns newly allocated string.            */

char *pathname(char *name1, char *name2)
{
char *pResult;

pResult = (char*) malloc(strlen(name1) + strlen(name2) + 1);     /* Make room */
strcpy(pResult, name1);                               /* Copy in first string */
strcat(pResult, name2);                               /* Append second string */

return pResult;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Returns Boolean - true if `character' is found                             */
/* If character is found it is stripped                                       */

boolean cmp_next_non_space(char *line, int *pPos, int offset, char character)
{
boolean result;

*pPos = skip_spc(line, *pPos +  offset);    /* Strip, possibly skipping first */
result = (line[*pPos] == character);
if (result) (*pPos)++;                                /* Strip test character */
return result;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Returns Boolean - true if `character' is valid end of statement            */

boolean test_eol(char character)
{ return (character == '\0') || (character == ';') || (character == '\n'); }

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

unsigned int get_identifier(char *line, unsigned int position, char *buffer,
                            unsigned int max_length)
{
unsigned int i;

i = 0;
if (alphabetic(line[position]))
  while ((alpha_numeric(line[position])) && (i < max_length - 1))
    buffer[i++] = line[position++];       /* Truncates if too long for buffer */

buffer[i] = '\0';
return i;                               /* Length of symbol (sans terminator) */
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

boolean alpha_numeric(char c)                                       /* Crude! */
{
return (((c >= '0') && (c <= '9')) || alphabetic(c)) || (c == '.');
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

boolean alphabetic(char c)                                          /* Crude! */
{
return ((c == '_') || ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z')));
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* If recognised, translate a C-style 'escaped' character code                */
/* E.g. 'n' will become '\n'                                                  */

unsigned char c_char_esc(unsigned char c)
{
switch (c)
  {
  case '0':  c = '\0'; break;
  case '\"': c = '\"'; break;
  case '\'': c = '\''; break;
  case '\?': c = '\?'; break;
  case '\\': c = '\\'; break;
  case 'a':  c = '\a'; break;
  case 'b':  c = '\b'; break;
  case 'f':  c = '\f'; break;
  case 'n':  c = '\n'; break;
  case 'r':  c = '\r'; break;
  case 't':  c = '\t'; break;
  case 'v':  c = '\v'; break;
  default: break;
  }

return c;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Read number of specified radix into variable indicated by *value           */
/* Return flag to say number read (value at pointer).                         */

int get_num(char *line, int *position, int *value, unsigned int radix)
{

  int num_char(char *line, int *pos, unsigned int radix)
    {                          /* Return value(c) if in radix  else return -1 */
    char c;

    while ((c = line[*pos]) == '_') (*pos)++;          /* Allow & ignore  '_' */
    if (c < '0') return -1;
    if (c >= 'a') c = c & 0xDF;                         /* Upper case convert */
    if (c <= '9') { if (c < '0'+ radix) return c - '0';     /* Number < radix */
                    else                return -1; }        /* Number > radix */
    else          { if (c < 'A')                   return -1;   /* Not letter */
                    else if (c < 'A' + radix - 10) return c - 'A' + 10;
                         else                      return -1; }
    }

int i, new_digit;
boolean found;

i = skip_spc(line, *position);
*value = 0;
found  = FALSE;

while ((new_digit = num_char(line, &i, radix)) >= 0)
  {
  *value = (*value * radix) + new_digit;
  found = TRUE;
  i++;
  }

if (found) *position = i;                                     /* Move pointer */
return found;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Test abstracted for convenience of use                                     */

int allow_error(unsigned int error_code, boolean first_pass, boolean last_pass)
{
return (!last_pass && ((error_code & ALLOW_ON_INTER_PASS) != 0))
    || (first_pass && ((error_code & ALLOW_ON_FIRST_PASS) != 0));
}

/*============================================================================*/
