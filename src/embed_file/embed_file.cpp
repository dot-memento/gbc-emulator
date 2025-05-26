#include <array>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cerr << "USAGE: " << argv[0] << " {sym} {rsrc}\n\n"
        "  Creates {sym}.c from the contents of {rsrc}" << std::endl;
        return EXIT_FAILURE;
    }

    std::ifstream in_file(argv[2], std::ios::binary);
    if (!in_file.is_open())
    {
        std::cerr << "Couldn't open '" << argv[2] << "'." << std::endl;
        return EXIT_FAILURE;
    }

    const char* sym = argv[1];
    std::string out_file_name{sym};
    out_file_name.append(".h");
    std::ofstream out_file(out_file_name);
    if (!out_file.is_open())
    {
        std::cerr << "Couldn't open '" << out_file_name << "'." << std::endl;
        return EXIT_FAILURE;
    }

    out_file << "#include <stdlib.h>\n\n"
        "unsigned char " << argv[1] << "[] = {\n";

    std::array<char, 256> buf;
    size_t linecount = 0;

    while (!in_file.eof())
    {
        in_file.read(buf.data(), buf.size());
        const std::streamsize nread = in_file.gcount();
        for (std::streamsize i = 0; i < nread; ++i)
        {
            out_file << "0x" << std::setw(2) << std::setfill('0') << std::hex
                     << +buf[static_cast<decltype(buf)::size_type>(i)] << ", ";
            if (++linecount >= 16)
            {
                out_file << '\n';
                linecount = 0;
            }
        }
    }

    if (linecount > 0)
        out_file << '\n';

    out_file << "};\nconst size_t " << sym << "_len = sizeof(" << sym << ");\n";

    return EXIT_SUCCESS;
}