#include <map>

using namespace std;

map<string, int> TYPE_STR_TO_INT = {
    {"A", 1},
    {"NS", 2},
    {"CNAME", 5},
    {"SOA", 6},
    {"MX", 15},
    {"TXT", 16},
    {"AAAA", 28}
};
map<int, string> TYPE_INT_TO_STR = {
    {1, "A"},
    {2, "NS"},
    {5, "CNAME"},
    {6, "SOA"},
    {15, "MX"},
    {16, "TXT"},
    {28, "AAAA"}
};

map<string, int> CLASS_STR_TO_INT = {
    {"IN", 1},
    {"CS", 2},
    {"CH", 3},
    {"HS", 4}
};

map<int, string> CLASS_INT_TO_STR = {
    {1, "IN"},
    {2, "CS"},
    {3, "CH"},
    {4, "HS"}
};
