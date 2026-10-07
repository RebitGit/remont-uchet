#pragma once
#include <wx/string.h>
#include <string>

namespace remont::utf8 {

inline wxString U(const char* s) {
    return wxString::FromUTF8(s);
}

inline std::string toUtf8(const wxString& s) {
    return std::string(s.ToUTF8().data());
}

}