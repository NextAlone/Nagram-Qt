#pragma once

namespace Settings::Builder {
class SectionBuilder;
} // namespace Settings::Builder

namespace Nagram {

// The switch for the builds of the default branch, among the update
// options of the upstream settings.
void AddCiUpdatesButton(Settings::Builder::SectionBuilder &builder);

} // namespace Nagram
