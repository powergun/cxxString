//
// Created by wein on 4/11/18.
//

#include <cassert>
#include <map>
#include <string>
#include <fstream>

using std::map;
using std::string;

void RunTinyTests();

class Configure {
public:
    explicit Configure(const string& filePath)
        : m_store() {
        std::ifstream ifs{filePath};
        string line, key, value;
        if (! ifs.good()) {
            return;
        }
        while (ifs.good()) {
            std::getline(ifs, line);
            size_t pos = line.find_first_of(": ");
            if (pos == string::npos) {
                continue;
            }
            key = line.substr(0, pos);
            value = line.substr(pos + 2);
            m_store[key] = value;
        }
    }
    string Get(const string& k) {
        return m_store[k];
    }
private:
    map<string, string> m_store;
};

void setUp() {
    std::ofstream ofs;
    ofs.open("/tmp/column_separated_text");
    const char* text = R"iddqd(
doom: 1992
doom2: 1993
doom3: 2003
    )iddqd";
    ofs << text;
}

void test_readConfFile() {
    auto filePath = "/tmp/column_separated_text";
    Configure c{filePath};
    assert(c.Get("doom4").empty());
    assert(string{"1992"} == c.Get("doom"));
    assert(string{"1993"} == c.Get("doom2"));
}

int main() {
    RunTinyTests();
    return 0;
}