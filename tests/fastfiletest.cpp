
#include "../source/cppimplementation/fastfile.cpp"

std::string encoding(const char* value, const int, const char* ) {
    return std::string{value};
}

std::string decoding(std::string value) {
    return value;
}

// g++ -o main.exe fastfiletest.cpp -g -ggdb --std=c++11 && ./main.exe
int main(int argc, char const *argv[])
{
    FastFile<std::string> fastfile( "./sample.txt" );
    fprintf( stderr, "call a: %s\n", fastfile.call( encoding ).c_str() );
    fprintf( stderr, "call b: %s\n", fastfile.call( encoding ).c_str() );
    fastfile.resetlines();
    fprintf( stderr, "call c: %s\n", fastfile.call( encoding ).c_str() );
    fprintf( stderr, "call d: %s\n", fastfile.call( encoding ).c_str() );
    fprintf( stderr, "call e: %s\n", fastfile.call( encoding ).c_str() );
    fprintf( stderr, "call f: %s\n", fastfile.call( encoding ).c_str() );
    fprintf( stderr, "call g: '%s'\n", fastfile.call( encoding ).c_str() );
    fprintf( stderr, "call h: '%s'\n", fastfile.call( encoding ).c_str() );
}

