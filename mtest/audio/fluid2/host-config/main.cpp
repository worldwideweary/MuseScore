#include <fluidsynth.h>
#include <cstdio>
#include <cstring>

int main()
{
    std::puts(fluid_version_str());
    return std::strcmp(fluid_version_str(), "2.3.4");
}
