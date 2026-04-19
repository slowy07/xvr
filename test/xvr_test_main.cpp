#define CATCH_CONFIG_RUNNER

#include <catch2/catch_all.hpp>
#include <cstdlib>

int main(int argc, char* argv[]) {
    Catch::Session session;

    int returnCode = session.applyCommandLine(argc, argv);
    if (returnCode != 0) {
        return returnCode;
    }

    return session.run();
}
