#include <string>

namespace GlobalAlignment {
    std::string GetDiff(const char *first_filename, const char *second_filename);

    std::string RestoreByDiff(const char *filename, const char *diff_filename);
}