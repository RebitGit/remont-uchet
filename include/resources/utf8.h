#pragma once
#include <wx/string.h>

namespace remont::utf8 {

inline wxString U(const char* s) {
    return wxString::FromUTF8(s);
}

}