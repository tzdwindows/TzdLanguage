// TzdTools.cpp : CLI entry point for TzdLanguage.
// Links dynamically to tzd.dll via tzd.lib and delegates execution to tzd_run_main().
//

#include <iostream>
#include "tzd.h"

int main(int argc, char* argv[]) {
    return tzd_run_main(argc, argv);
}
