// Copyright (C) 2026
// SPDX-License-Identifier: GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/secretaspect.h>

#include <utils/aspects.h>
#include <utils/result.h>

namespace DsComplete::Internal {

class DsCompleteSettings final : public Utils::AspectContainer
{
public:
    DsCompleteSettings();

    Utils::BoolAspect enabled{this};
    Utils::BoolAspect autoComplete{this};
    Core::SecretAspect apiKey{this};
    Utils::StringAspect baseUrl{this};
    Utils::StringAspect model{this};
    Utils::IntegerAspect maximumPromptCharacters{this};
    Utils::IntegerAspect maximumSuffixCharacters{this};
    Utils::IntegerAspect maximumTokens{this};
    Utils::IntegerAspect debounceMilliseconds{this};
};

DsCompleteSettings &settings();
void setupDsCompleteSettings();
bool isDsCompleteEnabled();

} // namespace DsComplete::Internal