// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#include "audio/midi/fluid2/channelrouting.h"

#include <cstdlib>
#include <iostream>

using Ms::Fluid2ChannelRouting;

static void require(bool condition, const char* message)
      {
      if (!condition) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
            }
      }

int main()
      {
      Fluid2ChannelRouting routing;
      for (int channel = 0; channel < 256; ++channel)
            require(routing.source(channel) == Fluid2ChannelRouting::Unknown, "stale initial owner");
      require(!routing.setSource(-1, 0) && !routing.setSource(256, 0), "invalid channel accepted");
      require(routing.source(-1) == Fluid2ChannelRouting::Unknown
              && routing.source(256) == Fluid2ChannelRouting::Unknown, "invalid channel lookup");
      require(routing.groupOwner(0, 0) == Fluid2ChannelRouting::Unknown
              && routing.groupOwner(0, 129) == Fluid2ChannelRouting::Unknown
              && routing.groupOwner(16, 16) == Fluid2ChannelRouting::Unknown, "invalid group accepted");

      // All 256 logical channels remain distinct metadata values. With only
      // 128 raw outputs, their different owners must not silently alias.
      for (int channel = 0; channel < 256; ++channel) {
            require(routing.setSource(channel, channel), "identity source rejected");
            require(routing.source(channel) == channel, "identity source changed");
            require(routing.bankGroupOwner(channel / 128, channel % 128) == channel,
                    "bank output lost its distinct logical owner");
            }
      for (unsigned groups = 1; groups <= 128; ++groups)
            for (unsigned group = 0; group < groups; ++group)
                  require(routing.groupOwner(group, groups) == Fluid2ChannelRouting::Mixed,
                          "modulo collision mistaken for an independent owner");

      routing.clear();
      require(routing.setSource(5, 5) && routing.setSource(255, 5), "high bend lane rejected");
      require(routing.groupOwner(5, 128) == 5 && routing.groupOwner(127, 128) == 5,
              "bend lane lost its source across output groups");
      require(routing.bankGroupOwner(0, 5) == 5 && routing.bankGroupOwner(1, 127) == 5,
              "bend lane lost its source across banks");
      require(routing.bankGroupOwner(2, 0) == Fluid2ChannelRouting::Unknown
              && routing.bankGroupOwner(0, 128) == Fluid2ChannelRouting::Unknown, "invalid bank output accepted");
      require(routing.groupOwner(0, 128) == Fluid2ChannelRouting::Unknown, "unused output acquired an owner");
      require(routing.setSource(128, 0) && routing.setSource(0, 0), "same-owner group rejected");
      require(routing.groupOwner(0, 128) == 0, "same-owner modulo group marked mixed");

      // Do not redirect old release tails if a channel is reused by a different
      // owner. Conflicts remain sticky until the affected voices are killed.
      require(!routing.setSource(128, 128), "live ownership reassignment accepted");
      require(routing.source(128) == Fluid2ChannelRouting::Mixed
              && routing.groupOwner(0, 128) == Fluid2ChannelRouting::Mixed, "ownership conflict hidden");
      require(!routing.setSource(128, 0), "ambiguous owner silently recovered");
      routing.clear(128);
      require(routing.source(5) == 5 && routing.source(255) == 5, "channel reset forgot unrelated owners");
      require(routing.setSource(128, 0) && routing.groupOwner(0, 128) == 0, "killed channel cannot be reused");

      require(!routing.setSource(128, -1), "missing source accepted");
      require(routing.groupOwner(0, 128) == Fluid2ChannelRouting::Mixed, "missing source treated as safe");
      routing.clear(128);
      require(!routing.setSource(128, 256), "out-of-range source accepted");
      require(routing.groupOwner(0, 128) == Fluid2ChannelRouting::Mixed, "out-of-range source treated as safe");

      // One output may contain several bend lanes for the same logical owner.
      // Both low/high halves are exercised for every supported group count.
      for (unsigned groups = 1; groups <= 128; ++groups) {
            routing.clear();
            for (int channel = 0; channel < 256; ++channel)
                  require(routing.setSource(channel, channel % int(groups)), "lane ownership rejected");
            for (unsigned group = 0; group < groups; ++group)
                  require(routing.groupOwner(group, groups) == int(group), "same-source lanes disagree");
            }
      routing.clear();
      for (unsigned group = 0; group < 128; ++group)
            require(routing.groupOwner(group, 128) == Fluid2ChannelRouting::Unknown, "score reset left an owner");
      std::cout << "Fluid2 routing: 256 channels, bend sources, modulo conflicts and ownership resets passed\n";
      }
