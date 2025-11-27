/**
 * Aurora OS - PE/COFF Binary Loader
 * 
 * Support for loading Windows PE (Portable Executable) format binaries
 */

#ifndef PE_LOADER_H
#define PE_LOADER_H

#include <stdint.h>

/* DOS Header */
#define PE_DOS_SIGNATURE 0x5A4D  /* "MZ" */

typedef struct {
    uint16_t e_magic;      /* Magic number (MZ) */
    uint16_t e_cblp;       /* Bytes on last page of file */
    uint16_t e_cp;         /* Pages in file */
    uint16_t e_crlc;       /* Relocations */
    uint16_t e_cparhdr;    /* Size of header in paragraphs */
    uint16_t e_minalloc;   /* Minimum extra paragraphs needed */
    uint16_t e_maxalloc;   /* Maximum extra paragraphs needed */
    uint16_t e_ss;         /* Initial (relative) SS value */
    uint16_t e_sp;         /* Initial SP value */
    uint16_t e_csum;       /* Checksum */
    uint16_t e_ip;         /* Initial IP value */
    uint16_t e_cs;         /* Initial (relative) CS value */
    uint16_t e_lfarlc;     /* File address of relocation table */
    uint16_t e_ovno;       /* Overlay number */
    uint16_t e_res[4];     /* Reserved words */
    uint16_t e_oemid;      /* OEM identifier */
    uint16_t e_oeminfo;    /* OEM information */
    uint16_t e_res2[10];   /* Reserved words */
    uint32_t e_lfanew;     /* File address of new exe header */
} __attribute__((packed)) pe_dos_header_t;

/* PE Signature */
#define PE_NT_SIGNATURE 0x00004550  /* "PE\0\0" */

/* COFF File Header */
#define PE_MACHINE_I386  0x014C
#define PE_MACHINE_AMD64 0x8664

typedef struct {
    uint16_t Machine;              /* Target machine */
    uint16_t NumberOfSections;     /* Number of sections */
    uint32_t TimeDateStamp;        /* Time date stamp */
    uint32_t PointerToSymbolTable; /* File pointer to symbol table */
    uint32_t NumberOfSymbols;      /* Number of symbols */
    uint16_t SizeOfOptionalHeader; /* Size of optional header */
    uint16_t Characteristics;      /* Characteristics */
} __attribute__((packed)) pe_coff_header_t;

/* Optional Header - PE32 */
#define PE_OPTIONAL_HDR_MAGIC_PE32     0x010B
#define PE_OPTIONAL_HDR_MAGIC_PE32PLUS 0x020B

typedef struct {
    uint16_t Magic;
    uint8_t  MajorLinkerVersion;
    uint8_t  MinorLinkerVersion;
    uint32_t SizeOfCode;
    uint32_t SizeOfInitializedData;
    uint32_t SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;
    uint32_t BaseOfCode;
    uint32_t BaseOfData;
    uint32_t ImageBase;
    uint32_t SectionAlignment;
    uint32_t FileAlignment;
    uint16_t MajorOperatingSystemVersion;
    uint16_t MinorOperatingSystemVersion;
    uint16_t MajorImageVersion;
    uint16_t MinorImageVersion;
    uint16_t MajorSubsystemVersion;
    uint16_t MinorSubsystemVersion;
    uint32_t Win32VersionValue;
    uint32_t SizeOfImage;
    uint32_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem;
    uint16_t DllCharacteristics;
    uint32_t SizeOfStackReserve;
    uint32_t SizeOfStackCommit;
    uint32_t SizeOfHeapReserve;
    uint32_t SizeOfHeapCommit;
    uint32_t LoaderFlags;
    uint32_t NumberOfRvaAndSizes;
} __attribute__((packed)) pe_optional_header32_t;

/* Data Directory */
#define PE_DIRECTORY_EXPORT         0
#define PE_DIRECTORY_IMPORT         1
#define PE_DIRECTORY_RESOURCE       2
#define PE_DIRECTORY_EXCEPTION      3
#define PE_DIRECTORY_SECURITY       4
#define PE_DIRECTORY_BASERELOC      5
#define PE_DIRECTORY_DEBUG          6
#define PE_DIRECTORY_ARCHITECTURE   7
#define PE_DIRECTORY_GLOBALPTR      8
#define PE_DIRECTORY_TLS            9
#define PE_DIRECTORY_LOAD_CONFIG   10
#define PE_DIRECTORY_BOUND_IMPORT  11
#define PE_DIRECTORY_IAT           12
#define PE_DIRECTORY_DELAY_IMPORT  13
#define PE_DIRECTORY_COM_DESCRIPTOR 14

typedef struct {
    uint32_t VirtualAddress;
    uint32_t Size;
} __attribute__((packed)) pe_data_directory_t;

/* Section Header */
#define PE_SECTION_NAME_SIZE 8

#define PE_SCN_CNT_CODE               0x00000020
#define PE_SCN_CNT_INITIALIZED_DATA   0x00000040
#define PE_SCN_CNT_UNINITIALIZED_DATA 0x00000080
#define PE_SCN_MEM_EXECUTE            0x20000000
#define PE_SCN_MEM_READ               0x40000000
#define PE_SCN_MEM_WRITE              0x80000000

typedef struct {
    uint8_t  Name[PE_SECTION_NAME_SIZE];
    uint32_t VirtualSize;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLinenumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLinenumbers;
    uint32_t Characteristics;
} __attribute__((packed)) pe_section_header_t;

/* Import Directory Entry */
typedef struct {
    uint32_t ImportLookupTableRVA;
    uint32_t TimeDateStamp;
    uint32_t ForwarderChain;
    uint32_t NameRVA;
    uint32_t ImportAddressTableRVA;
} __attribute__((packed)) pe_import_descriptor_t;

/* Export Directory */
typedef struct {
    uint32_t Characteristics;
    uint32_t TimeDateStamp;
    uint16_t MajorVersion;
    uint16_t MinorVersion;
    uint32_t Name;
    uint32_t Base;
    uint32_t NumberOfFunctions;
    uint32_t NumberOfNames;
    uint32_t AddressOfFunctions;
    uint32_t AddressOfNames;
    uint32_t AddressOfNameOrdinals;
} __attribute__((packed)) pe_export_directory_t;

/* PE Image Structure */
typedef struct {
    pe_dos_header_t*      dos_header;
    pe_coff_header_t*     coff_header;
    pe_optional_header32_t* optional_header;
    pe_data_directory_t*  data_directories;
    pe_section_header_t*  section_headers;
    void*                 image_base;
    uint32_t              image_size;
    uint32_t              entry_point;
} pe_image_t;

/* PE Loader Functions */

/**
 * Initialize PE loader subsystem
 */
void pe_loader_init(void);

/**
 * Validate PE file format
 * @param data Pointer to PE file data
 * @param size Size of PE file data
 * @return 1 if valid, 0 otherwise
 */
int pe_validate(const void* data, uint32_t size);

/**
 * Load PE executable into memory
 * @param data Pointer to PE file data
 * @param size Size of PE file data
 * @param image Output PE image structure
 * @return 0 on success, -1 on error
 */
int pe_load(const void* data, uint32_t size, pe_image_t* image);

/**
 * Unload PE image from memory
 * @param image PE image structure
 */
void pe_unload(pe_image_t* image);

/**
 * Get PE entry point address
 * @param image PE image structure
 * @return Entry point address
 */
void* pe_get_entry_point(pe_image_t* image);

/**
 * Resolve imports in PE image
 * @param image PE image structure
 * @return 0 on success, -1 on error
 */
int pe_resolve_imports(pe_image_t* image);

/**
 * Process base relocations for PE image
 * @param image PE image structure
 * @param delta Difference between actual and preferred load address
 * @return 0 on success, -1 on error
 */
int pe_apply_relocations(pe_image_t* image, int32_t delta);

/**
 * Get export address from PE image by name
 * @param image PE image structure
 * @param name Export function name
 * @return Function address or NULL if not found
 */
void* pe_get_export_by_name(pe_image_t* image, const char* name);

/**
 * Get export address from PE image by ordinal
 * @param image PE image structure
 * @param ordinal Export ordinal number
 * @return Function address or NULL if not found
 */
void* pe_get_export_by_ordinal(pe_image_t* image, uint16_t ordinal);

/**
 * Get number of exports in PE image
 * @param image PE image structure
 * @return Number of exports
 */
uint32_t pe_get_export_count(pe_image_t* image);

/**
 * Get export name by index
 * @param image PE image structure
 * @param index Export index
 * @return Export name or NULL
 */
const char* pe_get_export_name(pe_image_t* image, uint32_t index);

/* Base relocation types */
#define IMAGE_REL_BASED_ABSOLUTE       0
#define IMAGE_REL_BASED_HIGH           1
#define IMAGE_REL_BASED_LOW            2
#define IMAGE_REL_BASED_HIGHLOW        3
#define IMAGE_REL_BASED_HIGHADJ        4
#define IMAGE_REL_BASED_DIR64          10

/* Base relocation block header */
typedef struct {
    uint32_t VirtualAddress;
    uint32_t SizeOfBlock;
} __attribute__((packed)) pe_base_reloc_block_t;

/* Import lookup table entry (for 32-bit PE) */
typedef struct {
    union {
        uint32_t ForwarderString;
        uint32_t Function;
        uint32_t Ordinal;
        uint32_t AddressOfData;
    };
} __attribute__((packed)) pe_thunk_data32_t;

/* Import hint/name entry */
typedef struct {
    uint16_t Hint;
    char     Name[1];  /* Variable length name follows */
} __attribute__((packed)) pe_import_by_name_t;

#define IMAGE_ORDINAL_FLAG32 0x80000000

#endif /* PE_LOADER_H */
