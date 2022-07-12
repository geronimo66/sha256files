/****************************************************************************************************************************
 * Copyleft (c) 2022 by Marco Gerodetti                                                                                     *
 *                                                                                                                          *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public        *
 * License as published by the Free Software Foundation, version 2. This program is distributed WITHOUT ANY WARRANTY;       *
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public     *
 * License for more details <http://www.gnu.org/licenses/gpl-2.0.html>.                                                     *
 *                                                                                                                          *
 *   written in simple C++, POSIX compatibility for Environmental requirements                                              *
 *   using c++ ISO/IEC 14882:2011, POSIX c-libs IEEE Std 1003.1, https://pubs.opengroup.org/onlinepubs/9699919799/).        *
 *                                                                                                                          *
 * build instruction on UNIX/LINUX:                                                                                         *
 *    $  g++ sha256filesMain.cpp -osha256file -std=c++17 -s -Ofast                                                          *
 *                                                                                                                          *
 * design paradigms:                                                                                                        *
 *   POSIX libs & environment using              => high compatibility, no 3rd party libs                                   *
 *   Few Sourcefile & libs                       => easy to handle, few complexity                                          *
 ****************************************************************************************************************************/

 //  Unpublished Version, NOT To USE

#include <time.h>
#include <string.h>
#include "../lib/io.hpp"

#if defined _WIN32
    #include <io.h>
    #include <fileapi.h>
    #define path_separator '\\'
    #define access _access
#define F_OK 0
    enum FileType { DT_UNKNOWN, DT_FIFO, DT_LNK, DT_CHR, DT_DIR, DT_BLK, DT_REG, DT_SOCK, DT_WHT };
    #else // UNIX / LINUX
    #include <dirent.h>
    #include <string.h>
    #include <unistd.h>
    #include <sys/stat.h>
    #define path_separator '/'
    typedef u8 FileType;
    #define INVALID_HANDLE_VALUE NULL
#endif
// _WIN32

enum fileModes { modeRead = 'r', modeWrite = 'w', modeOverride = 'o' };

constexpr DString&
operator << (DString& p_s, const tm* t) noexcept {
    p_s << Num<4>(1900 + t->tm_year) << Num<2>(t->tm_mon) << Num<2>(t->tm_mday)
        << Num<2>(t->tm_hour) << Num<2>(t->tm_min) << Num<2>(t->tm_sec); return p_s;
}



struct timer {
    time_t start = time(nullptr);
    void reset(tm &t) {
        start = time(nullptr);
    }
    double runtime() {
        return difftime(time(nullptr), start);
    }
};

typedef  Pair< const u8, const CString> DtPair;

constexpr CStringInstance cFIFO ("FIFO");
constexpr CStringInstance cLNK  ("LNK ");
constexpr CStringInstance cCHR  ("CHR ");
constexpr CStringInstance cDIR  ("DIR ");
constexpr CStringInstance cBLK  ("BLK ");
constexpr CStringInstance cFILE ("FILE");
constexpr CStringInstance cSOCK ("SOCK");
constexpr CStringInstance cWHT  ("WHT ");
constexpr CStringInstance cUNKNOWN  ("??? ");

constexpr DtPair rDT_UNKNOWN  ();

constexpr DtPair fileTypeList[] = {{ DT_FIFO, cFIFO}, {DT_LNK , cLNK}, {DT_CHR , cCHR}, {DT_DIR , cDIR}, {DT_BLK , cBLK}, {DT_REG , cFILE}, {DT_SOCK, cSOCK}, {DT_WHT , cWHT}};

constexpr  KeyPairs< const u8, const CString> fileTypName ( {fileTypeList,  { DT_UNKNOWN, cUNKNOWN }});
const static KeyPairs< u16, FileType> posixFileTyps ({ { S_IFDIR, DT_DIR}, { S_IFLNK, DT_LNK}, { S_IFBLK, DT_BLK}, { S_IFREG, DT_REG}}, {0, DT_UNKNOWN});


void searchDir(const char* p_root_dir, const char* p_file_name, FileType type = DT_UNKNOWN) {
#if defined _WIN32
    HANDLE dir_handle = nullptr;
    WIN32_FIND_DATAA ep = { 0 };
#else
    DIR *dir_handle = NULL;
    struct dirent *ep;
#endif
    DStringContainer<PATH_MAX> this_path_container;
    this_path_container.reset() << p_root_dir;
    if (*p_file_name) {
        if (( strlen( p_root_dir) == 1 ) && ( *p_root_dir == path_separator ))
            this_path_container << p_file_name;
        else 
            this_path_container << path_separator << p_file_name;
    }
    auto this_path = this_path_container.reader();
#ifndef _WIN32
    i32 file_mode = -1;
    struct stat sb;
    if ( lstat( this_path.begin(), &sb) != -1){
      //  if (type == DT_UNKNOWN){
        type =  sb.st_mode >> 8 ;
        type = posixFileTyps.valueOf( sb.st_mode >> 8);
    }
    file_mode = sb.st_mode & 0xff;
#endif

    if (type == DT_DIR || type == DT_UNKNOWN) {
#if defined _WIN32
        DStringContainer<PATH_MAX> dir_name;
        dir_name << this_path << path_separator << "*.*";
        dir_handle = FindFirstFileA(dir_name.begin(), &ep);
#else
        dir_handle = opendir(this_path.begin());
#endif
    }
    if (type == DT_UNKNOWN)
        type = (dir_handle == INVALID_HANDLE_VALUE) ? DT_REG : DT_DIR;
    
    {
        DStringContainer<2048> outStr;
        CString tp = fileTypName.valueOf( type);
        outStr.reset() << tp << '|';
    #ifndef _WIN32
        if ( file_mode < 0)
            outStr << "    ";
        else{
            outStr << Scalar< 4,  8, '0'>( file_mode);
        }
        outStr << '|';
    #endif
        switch (type) {
            case DT_REG: {
                File this_file;
                this_file.open( this_path.begin(), "r", false);
                static Sha256 sha_gen;
                if ( this_file.is_open()) {
                    sha_gen.reset() << this_file;
                    u64  size = this_file.tell();  // current file pointer as size
                    outStr << Num<12, ' '>( size) << '|';
                    if (size > 0 )
                        outStr  << sha_gen << '|';
                    else
                        outStr << "                                                                |";
                }
                else
                    outStr << "#" << Num<5>(errno) << " error|                                                                |";
            } break;
            default: {
                outStr << "           0|                                                                |";
            } break;
        }
        fputs( outStr.begin(), stdout);
        fputs( p_root_dir, stdout);
        fputs("/|", stdout);
        fputs( p_file_name, stdout);
        fputs("\n", stdout);
    }
    switch (type) {
    case DT_DIR: {
#if defined _WIN32
        do {
            const FileType ft = (ep.dwFileAttributes & 0x10) ? DT_DIR : DT_REG;
            const char* ep_name = ep.cFileName;
#else
        while ((dir_handle != NULL) && (ep = readdir(dir_handle)) != NULL) {
            FileType ft = ep->d_type;
            char* ep_name = ep->d_name;
#endif
            DStringContainer<3> name_firstchars;
            name_firstchars.reset() << ep_name;
            if (!name_firstchars.reader().equals( CStringInstance(".")) && !name_firstchars.reader().equals( CStringInstance(".."))) {
                DStringContainer< PATH_MAX + 2> entry_path;
                searchDir( this_path.begin(), ep_name, ft);
            }
        }
#if defined _WIN32
        while (dir_handle != nullptr && FindNextFileA(dir_handle, &ep));
#endif
        } break;
    }

    if (dir_handle)
#if defined _WIN32
        FindClose(dir_handle);
#else
        closedir(dir_handle);
#endif
    }

struct ArgStr : public ArraySpan<char> {
    ArgStr( char* arg) : ArraySpan<char>( Span< char* const> (arg, arg + strlen(arg))) {}
};

/* description:    Program exit point
   return value:   none
   error:          errorNr                                                                                          */
static void
exitProgram(void) {
    fflush(nullptr);
}

/* description:    Program entry point
   return value:   errorNr                                                                                          */
int main(int argc, char **argv)
{
    atexit(&exitProgram);
    DStringContainer<1024> text_buffer;

    try {
        if (argc == 2) {
            char& last = argv[1][strlen(argv[1]) - 1];
            if ( strlen(argv[1]) > 2 && last == path_separator)
                last = 0;
            searchDir(argv[1], "");
            fputs("*DONE*", stdout);
        }
        else {
            char* progName = strrchr(argv[0], path_separator) ? strrchr(argv[0], path_separator) + 1 : argv[0];
            printf("%s V0.1.0.3 by M. Gerodetti - compute the sha256 hash of each file in the path tree\n", argv[0]);
            printf("syntax: %s <path>\n", progName);
        }
    }
    catch (const Exception ex) {
        text_buffer.reset() << ex;
        fputs(text_buffer.reader().begin(), stderr);
    }
}
