/*
 * iotest.cc
 *
 *  Created on: Feb 17, 2017
 *      Author: tiago
 */

#include <stdio.h>
#include <fcntl.h>

#if defined(__unix__) || defined (__CYGWIN__)
    #include <unistd.h>
#else
    #include <io.h>
#endif

#ifndef O_BINARY
    #define O_BINARY 0
#endif

#include <chrono>
#include <iostream>
#include <functional>
#include <fstream>
#include <map>
#include <string>
#include <vector>

using namespace std::chrono;

struct measure
{
    template<typename F, typename ...Args>
    static std::chrono::milliseconds::rep ms(F func, Args&&... args)
    {
        auto start = system_clock::now();
        func(std::forward<Args>(args)...);
        auto stop = system_clock::now();

        return duration_cast<milliseconds>(stop - start).count();
    }
};

void testCFileIO(const char* inFile, const char* outFile, std::vector<char>& inBuffer)
{
    FILE* in = ::fopen(inFile, "rb");
    if (!in)
    {
        std::cout << "Can't open input file: " << inFile << std::endl;
        return;
    }

    FILE* out = ::fopen(outFile, "wb");
    if (!out)
    {
        std::cout << "Can't open output file: " << outFile << std::endl;
        return;
    }

    fseek(in, 0, SEEK_END);
    size_t inFileSize = ::ftell(in);
    fseek(in, 0, SEEK_SET);

    for (size_t bytesLeft = inFileSize, chunk = inBuffer.size(); bytesLeft > 0; bytesLeft -= chunk)
    {
        if (bytesLeft < chunk)
        {
            chunk = bytesLeft;
        }

        ::fread(&inBuffer[0], 1, chunk, in);
        ::fwrite(&inBuffer[0], 1, chunk, out);
    }

    ::fclose(out);
    ::fclose(in);
}

void testCppIO(const char* inFile, const char* outFile, std::vector<char>& inBuffer)
{
    std::ifstream in(inFile, std::ifstream::binary);
    if (!in.is_open())
    {
        std::cout << "Can't open input file: " << inFile << std::endl;
        return;
    }

    std::ofstream out(outFile, std::ofstream::binary);
    if (!out.is_open())
    {
        std::cout << "Can't open output file: " << outFile << std::endl;
        return;
    }

    in.seekg(0, std::ifstream::end);
    size_t inFileSize = in.tellg();
    in.seekg(0, std::ifstream::beg);

    for (size_t bytesLeft = inFileSize, chunk = inBuffer.size(); bytesLeft > 0; bytesLeft -= chunk)
    {
        if (bytesLeft < chunk)
        {
            chunk = bytesLeft;
        }

        in.read(&inBuffer[0], chunk);
        out.write(&inBuffer[0], chunk);
    }
}

void testPosixIO(const char* inFile, const char* outFile, std::vector<char>& inBuffer)
{
    int in = ::open(inFile, O_RDONLY | O_BINARY);
    if (in < 0)
    {
        std::cout << "Can't open input file: " << inFile << std::endl;
        return;
    }

    int out = ::open(outFile, O_CREAT | O_WRONLY | O_BINARY, 0666);
    if (out < 0)
    {
        std::cout << "Can't open output file: " << outFile << std::endl;
        return;
    }

    size_t inFileSize = ::lseek(in, 0, SEEK_END);
    ::lseek(in, 0, SEEK_SET);

    for (size_t bytesLeft = inFileSize, chunk = inBuffer.size(); bytesLeft > 0; bytesLeft -= chunk)
    {
        if (bytesLeft < chunk)
        {
            chunk = bytesLeft;
        }

        ::read(in, &inBuffer[0], chunk);
        ::write(out, &inBuffer[0], chunk);
    }

    ::close(out);
    ::close(in);
}

int main(int argc, char* argv[])
{
    std::vector<std::string> args(argv, argv + argc);
    if (args.size() != 4)
    {
        std::cout << "Usage: " << args[0] << " copy_method (c, posix, c++) in_file number_of_times" << std::endl;
        return 1;
    }

    typedef std::map<std::string, std::function<void (const char*, const char*, std::vector<char>&)>> FuncMap;
    FuncMap funcMap { {"c", testCFileIO}, {"posix", testPosixIO}, {"c++", testCppIO}};

    auto it = funcMap.find(args[1]);
    if (it != funcMap.end())
    {
        std::vector<char> inBuffer(1024 * 1024);

        auto dest = args[2] + ".copy";
        const auto times = std::stoul(args[3]);

        milliseconds::rep total = 0;
        for (unsigned int i = 0; i < times; ++i)
        {
            total += measure::ms(it->second, args[2].c_str(), dest.c_str(), inBuffer);
            ::unlink(dest.c_str());
        }
        std::cout << "Average " << args[1] << " I/O took: " << total / double(times) << "ms" << std::endl;
    }
    else
    {
        std::cout << "Not supported copy method: " << args[1] << std::endl;
    }
}
