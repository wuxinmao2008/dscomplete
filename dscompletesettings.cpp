// Copyright (C) 2026
// SPDX-License-Identifier: GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dscompletesettings.h"

#include "dscompleteconstants.h"
#include "dscompletetr.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/layoutbuilder.h>

using namespace Utils;

namespace DsComplete::Internal {

static void initializeEnabledAspect(BoolAspect &aspect)
{
    aspect.setSettingsKey(Constants::ENABLE);
    aspect.setDisplayName(Tr::tr("Enable DeepSeek Completion"));
    aspect.setLabelText(Tr::tr("Enable DeepSeek Completion"));
    aspect.setToolTip(Tr::tr("Allows DeepSeek completion requests for enabled projects."));
    aspect.setDefaultValue(false);
}

DsCompleteSettings &settings()
{
    static DsCompleteSettings theSettings;
    return theSettings;
}

DsCompleteSettings::DsCompleteSettings()
{
    setAutoApply(false);

    initializeEnabledAspect(enabled);

    autoComplete.setSettingsKey("DsComplete.AutoComplete");
    autoComplete.setDisplayName(Tr::tr("Automatic Completion"));
    autoComplete.setLabelText(Tr::tr("Request automatically"));
    autoComplete.setDefaultValue(false);

    apiKey.setSettingsKey("DsComplete.ApiKey");
    apiKey.setDisplayName(Tr::tr("API Key"));
    apiKey.setLabelText(Tr::tr("API key:"));

    baseUrl.setSettingsKey("DsComplete.BaseUrl");
    baseUrl.setDisplayName(Tr::tr("Base URL"));
    baseUrl.setLabelText(Tr::tr("Base URL:"));
    baseUrl.setDefaultValue("https://api.deepseek.com");
    baseUrl.setDisplayStyle(StringAspect::DisplayStyle::LineEditDisplay);

    model.setSettingsKey("DsComplete.Model");
    model.setDisplayName(Tr::tr("Model"));
    model.setLabelText(Tr::tr("Model:"));
    model.setDefaultValue("deepseek-chat");
    model.setDisplayStyle(StringAspect::DisplayStyle::LineEditDisplay);

    maximumPromptCharacters.setSettingsKey("DsComplete.MaximumPromptCharacters");
    maximumPromptCharacters.setLabelText(Tr::tr("Maximum prefix characters:"));
    maximumPromptCharacters.setDefaultValue(12000);
    maximumPromptCharacters.setRange(256, 100000);

    maximumSuffixCharacters.setSettingsKey("DsComplete.MaximumSuffixCharacters");
    maximumSuffixCharacters.setLabelText(Tr::tr("Maximum suffix characters:"));
    maximumSuffixCharacters.setDefaultValue(4000);
    maximumSuffixCharacters.setRange(0, 100000);

    maximumTokens.setSettingsKey("DsComplete.MaximumTokens");
    maximumTokens.setLabelText(Tr::tr("Maximum output tokens:"));
    maximumTokens.setDefaultValue(256);
    maximumTokens.setRange(1, 8192);

    debounceMilliseconds.setSettingsKey("DsComplete.DebounceMilliseconds");
    debounceMilliseconds.setLabelText(Tr::tr("Automatic request delay:"));
    debounceMilliseconds.setSuffix(Tr::tr(" ms"));
    debounceMilliseconds.setDefaultValue(600);
    debounceMilliseconds.setRange(100, 10000);

    readSettings();

    apiKey.setEnabler(&enabled);
    autoComplete.setEnabler(&enabled);
    baseUrl.setEnabler(&enabled);
    model.setEnabler(&enabled);
    maximumPromptCharacters.setEnabler(&enabled);
    maximumSuffixCharacters.setEnabler(&enabled);
    maximumTokens.setEnabler(&enabled);
    debounceMilliseconds.setEnabler(&enabled);

    setLayouter([this] {
        using namespace Layouting;
        return Column {
            Form {
                enabled, br,
                apiKey, br,
                baseUrl, br,
                model, br,
                autoComplete, br,
                maximumPromptCharacters, br,
                maximumSuffixCharacters, br,
                maximumTokens, br,
                debounceMilliseconds, br,
            },
            st,
        };
    });
}

class DsCompleteSettingsPage final : public Core::IOptionsPage
{
public:
    DsCompleteSettingsPage()
    {
        setId(Constants::SETTINGS_ID);
        setDisplayName(Tr::tr("DeepSeek Completion"));
        setCategory(Core::Constants::SETTINGS_CATEGORY_AI);
        setSettingsProvider([] { return &settings(); });
    }
};

bool isDsCompleteEnabled()
{
    return settings().enabled();
}

void setupDsCompleteSettings()
{
    static DsCompleteSettingsPage settingsPage;
}

} // namespace DsComplete::Internal