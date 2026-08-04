// Copyright (C) 2026
// SPDX-License-Identifier: GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dscompleteclient.h"
#include "dscompleteconstants.h"
#include "dscompletesettings.h"
#include "dscompletetr.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/statusbarmanager.h>

#include <extensionsystem/iplugin.h>

#include <texteditor/texteditor.h>
#include <texteditor/textsuggestion.h>

#include <QAction>
#include <QToolButton>

using namespace Core;
using namespace TextEditor;
using namespace Utils;

namespace DsComplete::Internal {

enum class Direction { Previous, Next };

static void cycleSuggestion(TextEditorWidget *editor, Direction direction)
{
    auto *suggestion = dynamic_cast<CyclicSuggestion *>(editor->currentSuggestion());
    if (!suggestion || suggestion->suggestions().size() < 2)
        return;

    const QList<TextSuggestion::Data> suggestions = suggestion->suggestions();
    int index = suggestion->currentSuggestion() + (direction == Direction::Next ? 1 : -1);
    if (index < 0)
        index = suggestions.size() - 1;
    else if (index >= suggestions.size())
        index = 0;
    editor->insertSuggestion(
        std::make_unique<CyclicSuggestion>(suggestions, editor->document(), index));
}

class DsCompletePlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "DsComplete.json")

public:
    void initialize() final
    {
        setupDsCompleteSettings();
        m_client = new DsCompleteClient(this);

        ActionBuilder requestAction(this, Constants::ACTION_REQUEST);
        requestAction.setText(Tr::tr("Request DeepSeek Suggestion"));
        requestAction.setToolTip(Tr::tr("Request a DeepSeek suggestion at the current cursor."));
        requestAction.addOnTriggered(this, [this] {
            if (TextEditorWidget *editor = TextEditorWidget::currentTextEditorWidget())
                m_client->requestCompletions(editor);
        });

        ActionBuilder nextAction(this, Constants::ACTION_NEXT);
        nextAction.setText(Tr::tr("Show Next DeepSeek Suggestion"));
        nextAction.addOnTriggered(this, [] {
            if (TextEditorWidget *editor = TextEditorWidget::currentTextEditorWidget())
                cycleSuggestion(editor, Direction::Next);
        });

        ActionBuilder previousAction(this, Constants::ACTION_PREVIOUS);
        previousAction.setText(Tr::tr("Show Previous DeepSeek Suggestion"));
        previousAction.addOnTriggered(this, [] {
            if (TextEditorWidget *editor = TextEditorWidget::currentTextEditorWidget())
                cycleSuggestion(editor, Direction::Previous);
        });

        ActionBuilder enableAction(this, Constants::ACTION_ENABLE);
        enableAction.setText(Tr::tr("Enable DeepSeek Completion"));
        enableAction.addOnTriggered(this, [] { setEnabled(true); });

        ActionBuilder disableAction(this, Constants::ACTION_DISABLE);
        disableAction.setText(Tr::tr("Disable DeepSeek Completion"));
        disableAction.addOnTriggered(this, [] { setEnabled(false); });

        ActionBuilder toggleAction(this, Constants::ACTION_TOGGLE);
        toggleAction.setText(Tr::tr("Toggle DeepSeek Completion"));
        toggleAction.setCheckable(true);
        toggleAction.setChecked(settings().enabled());
        toggleAction.addOnTriggered(this, [](bool checked) { setEnabled(checked); });

        QAction *toggle = toggleAction.contextAction();
        QAction *request = requestAction.contextAction();
        auto updateActions = [toggle, request] {
            const bool enabled = settings().enabled();
            toggle->setChecked(enabled);
            toggle->setToolTip(enabled ? Tr::tr("Disable DeepSeek Completion")
                                       : Tr::tr("Enable DeepSeek Completion"));
            request->setEnabled(enabled);
        };
        settings().enabled.addOnChanged(this, updateActions);
        updateActions();

        auto *button = new QToolButton;
        button->setDefaultAction(toggle);
        StatusBarManager::addStatusBarWidget(button, StatusBarManager::RightCorner);
    }

    ShutdownFlag aboutToShutdown() final
    {
        m_client->cancelAll();
        return SynchronousShutdown;
    }

private:
    static void setEnabled(bool enabled)
    {
        settings().enabled.setValue(enabled);
        settings().apply();
        settings().writeSettings();
    }

    DsCompleteClient *m_client = nullptr;
};

} // namespace DsComplete::Internal

#include "dscompleteplugin.moc"