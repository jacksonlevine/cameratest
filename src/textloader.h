#include <string>
#include <istream>
#include <fstream>
#include <sstream>

int load_text(const char *fp, std::string &out);

#ifdef TEXT_LOADER_IMPLEMENTATION
int load_text (const char *fp, std::string &out) {
    std::ifstream text(fp);
    if(!text.is_open()) { return -1; }
    std::stringstream textstrm;
    textstrm << text.rdbuf();
    text.close();
    out = textstrm.str();
    return 1;
}
#endif