// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#include "master_layout.h"
#include <cstdlib>
#include <iostream>

int main()
      {
      const std::size_t enabled = masterSizeEnabled(), disabled = masterSizeDisabled();
      if (enabled != disabled || masterAlignmentEnabled() != masterAlignmentDisabled()) {
            std::cerr << "MasterSynthesizer layout differs across backend defines: enabled="
                      << enabled << ", disabled=" << disabled << '\n';
            return EXIT_FAILURE;
            }
      std::cout << "MasterSynthesizer layout agrees across backend defines: " << enabled << " bytes\n";
      }
