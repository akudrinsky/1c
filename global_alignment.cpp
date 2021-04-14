#include <deque>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <system_error>
#include <cstdio>

enum EditOperation
{
    DEL,
    SUB,
    INS,
    UNKNOWN,
};

namespace GlobalAlignment
{

    char *recursivePart(char *align_string, std::pair<const char *, const char *> data, std::pair<size_t, size_t> sizes);
    char *align_crossing(char *align_string, std::pair<const char *, const char *> data, std::pair<size_t, size_t> sizes);
    void reverse_memory(void *ptr, size_t by_size);
    void new_left_cost(std::vector<int> &counter, std::pair<const char *, const char *> data, std::pair<size_t, size_t> sizes);
    void new_right_cost(std::vector<int> &counter, std::pair<const char *, const char *> data, std::pair<size_t, size_t> sizes);
    EditOperation best_operation(int cost[3], char first, char second);

    size_t GetFilesize(const char *filename)
    {
        struct stat fileStat;
        int res = lstat(filename, &fileStat);
        if (res != 0)
        {
            throw std::logic_error("Lstat did not work properly. ");
        }
        return fileStat.st_size;
    }

    // what if zero? insert / delete?
    int levenshtein_distance(char first, char second)
    {
        return first != second;
    }

    // Just Needleman-Wunsh algorithm
    // TODO: second is smaller
    std::string GetDiff(const char *first_filename, const char *second_filename)
    {
        std::pair<FILE *, FILE *> files;
        std::pair<size_t, size_t> sizes;

        sizes.first = GetFilesize(first_filename);
        sizes.second = GetFilesize(second_filename);

        files.first = fopen(first_filename, "rb");
        files.second = fopen(second_filename, "rb");

        std::pair<char *, char *> data;
        data.first = new char[sizes.first + 1];
        data.second = new char[sizes.second + 1];

        fread(data.first, sizeof(char), sizes.first, files.first);
        fread(data.second, sizeof(char), sizes.second, files.second);

        data.first[sizes.first] = '\0';
        data.second[sizes.second] = '\0';

        char *align_string = new char[sizes.first + sizes.second + 1];

        if (sizes.first > sizes.second)
        {
            char *pos = recursivePart(align_string, data, sizes);
            for (char *tmp = align_string; *tmp != '\0'; ++tmp)
            {
                switch (*tmp)
                {
                case '+':
                    *tmp = '-';
                    break;
                case '-':
                    *tmp = '+';
                    break;
                }
            }
        }
        else
        {
            char *pos = recursivePart(align_string, data, sizes);
        }

        std::string diff;

        int size = strlen(align_string);
        int n_equals = 0;
        int n_plus = 0;
        int n_minus = 0;
        for (int i = 0; i < size; ++i) {
            if (align_string[i] == '=') {
                ++n_equals;
                diff += align_string[i];
            }
            if (align_string[i] == '+') {
                ++n_plus;
                diff += align_string[i];
            }
            if (align_string[i] == '-') {
                ++n_minus;
                diff += align_string[i];
            }
        }

        delete[] data.first;
        delete[] data.second;

        delete [] align_string;

        return diff;
    }

    char *recursivePart(char *align_string, std::pair<const char *, const char *> data, std::pair<size_t, size_t> sizes)
    {
        //printf("\n> rec %s | %s\n", data.first, data.second);
        if (sizes.second > 1)
        {
            std::vector<int> left(sizes.first + 1, 0);
            std::vector<int> right(sizes.first + 1, 0);
            std::vector<int> middle(sizes.first + 1, 0);

            std::pair<size_t, size_t> mid_indexes(0, sizes.second / 2);

            for (int i = 0; i < sizes.first + 1; ++i)
            {
                //printf("%d %d %d\n", left[i], right[i], middle[i]);
            }

            new_left_cost(
                left,
                data,
                std::make_pair(sizes.first, mid_indexes.second));

            new_right_cost(
                right,
                std::make_pair(data.first, data.second + mid_indexes.second),
                std::make_pair(sizes.first, sizes.second - mid_indexes.second));

            for (int i = 0; i < sizes.first + 1; ++i)
            {
                //printf("%d %d %d\n", left[i], right[i], middle[i]);
            }

            for (int i = 0; i <= sizes.first; ++i)
            {
                middle[i] = left[i] + right[i];
                if (middle[i] < middle[mid_indexes.first])
                {
                    mid_indexes.first = i;
                }
            }

            //printf("\n1 mid %d | %d\n", mid_indexes.first, mid_indexes.second);
            align_string = recursivePart(
                align_string,
                data,
                mid_indexes);

            //printf("\n2 mid %d | %d\n", mid_indexes.first, mid_indexes.second);
            align_string = recursivePart(
                align_string,
                std::make_pair(data.first + mid_indexes.first, data.second + mid_indexes.second),
                std::make_pair(sizes.first - mid_indexes.first, sizes.second - mid_indexes.second));

            //printf("\n< rec %s | %s\n", data.first, data.second);
            return align_string;
        }
        else
        {
            //printf("\n< rec %s | %s\n", data.first, data.second);
            return align_crossing(align_string, data, sizes);
        }
    }

    char *align_crossing(char *align_string, std::pair<const char *, const char *> data, std::pair<size_t, size_t> sizes)
    {
        //printf("> align %s | %s\n", data.first, data.second);
        // TODO heap
        int dyn_prog_matrix[sizes.first + 1][sizes.second + 1];

        dyn_prog_matrix[0][0] = 0;
        for (int i = 1; i <= sizes.first; i++)
            dyn_prog_matrix[i][0] = dyn_prog_matrix[i - 1][0] + levenshtein_distance(data.first[i - 1], 0);
        for (int j = 1; j <= sizes.second; j++)
            dyn_prog_matrix[0][j] = dyn_prog_matrix[0][j - 1] + levenshtein_distance(0, data.second[j - 1]);
        for (int j = 1; j <= sizes.second; j++)
        {
            for (int i = 1; i <= sizes.first; i++)
            {
                int cost[3] = {dyn_prog_matrix[i - 1][j], dyn_prog_matrix[i - 1][j - 1], dyn_prog_matrix[i][j - 1]};
                dyn_prog_matrix[i][j] = cost[best_operation(cost, data.first[i - 1], data.second[j - 1])];
            }
        }
        char *tmp_pointer = align_string;

        size_t i = sizes.first, j = sizes.second;

        while (i > 0 && j > 0)
        {
            int cost[3] = {dyn_prog_matrix[i - 1][j], dyn_prog_matrix[i - 1][j - 1], dyn_prog_matrix[i][j - 1]};
            switch (best_operation(cost, data.first[i - 1], data.second[j - 1]))
            {
            case DEL:
                *tmp_pointer++ = '-';
                i--;
                break;
            case SUB:
                *tmp_pointer++ = data.first[i - 1] == data.second[j - 1] ? '=' : '!';
                i--;
                j--;
                break;
            case INS:
                *tmp_pointer++ = '+';
                j--;
                break;
            default:
                throw std::logic_error("Unknown operand in enum");
            }
        }
        for (; i > 0; i--)
            *tmp_pointer++ = '-';
        for (; j > 0; j--)
            *tmp_pointer++ = '+';

        *tmp_pointer = '\0';
        reverse_memory(align_string, tmp_pointer - align_string);
        //printf("< align %s | %s\n", data.first, data.second);
        return tmp_pointer;
    }

    void new_left_cost(std::vector<int> &counter, std::pair<const char *, const char *> data, std::pair<size_t, size_t> sizes)
    {
        //printf("> left %s (%d) | %s (%d)\n", data.first, sizes.first, data.second, sizes.second);
        int ss = 0, tmp = 0;

        counter[0] = 0;
        for (int i = 1; i <= sizes.first; i++)
        {
            counter[i] = counter[i - 1] + levenshtein_distance(data.first[i - 1], 0);
            //printf("counter[%d] = %d\n", i, counter[i]);
        }
        for (int j = 1; j <= sizes.second; j++)
        {
            ss = counter[0];
            counter[0] += levenshtein_distance(0, data.second[j - 1]);
            //printf("counter[%d] = %d\n", 0, counter[0]);
            for (int i = 1; i <= sizes.first; i++)
            {
                int cost[3] = {counter[i - 1], ss, counter[i]};
                tmp = cost[best_operation(cost, data.first[i - 1], data.second[j - 1])];
                ss = counter[i];
                counter[i] = tmp;
                //printf("counter[%d] = %d\n", i, counter[i]);
            }
        }
        //printf("< left %s | %s\n", data.first, data.second);
    }

    void new_right_cost(std::vector<int> &counter, std::pair<const char *, const char *> data, std::pair<size_t, size_t> sizes)
    {
        //printf("> right %s | %s\n", data.first, data.second);
        int ss = 0, tmp = 0;

        counter[sizes.first] = 0;
        for (int i = sizes.first - 1; i >= 0; --i)
            counter[i] = counter[i + 1] + levenshtein_distance(data.first[i], 0);

        for (int j = sizes.second - 1; j >= 0; --j)
        {
            ss = counter[sizes.first];
            counter[sizes.first] += levenshtein_distance(0, data.second[j]);
            for (int i = sizes.first - 1; i >= 0; --i)
            {
                int cost[3] = {counter[i + 1], ss, counter[i]};
                tmp = cost[best_operation(cost, data.first[i], data.second[j])];
                ss = counter[i];
                counter[i] = tmp;
            }
        }
        //printf("< right %s | %s\n", data.first, data.second);
    }

    void reverse_memory(void *ptr, size_t by_size)
    {
        char *p = (char *)ptr;

        //printf("> reverse %s\n", p);
        for (int i = 0; i < by_size / 2; i++)
        {
            char tmp = p[i];
            p[i] = p[by_size - 1 - i];
            p[by_size - 1 - i] = tmp;
        }
        //printf("< reverse %s\n", p);
    }

    // nwmin -> best_operation
    EditOperation best_operation(int cost[3], char first, char second)
    {
        EditOperation i;

        //printf("best_operation: %c, %c\n", first, second);

        cost[DEL] += levenshtein_distance(first, 0);
        cost[SUB] += levenshtein_distance(first, second);
        cost[INS] += levenshtein_distance(0, second);
        i = cost[DEL] < cost[SUB] ? DEL : SUB;
        i = cost[i] < cost[INS] ? i : INS;
        return i;
    }

}